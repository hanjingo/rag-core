#include "server_reactor.h"

#include <hj/log/log.hpp>
#include <hj/time/date_time.hpp>
#include <hj/algo/uuid.hpp>
#include <hj/db/sqlite.hpp>

#include "conf.h"
#include "db_mgr.h"
#include "auth.h"
#include "account_mgr.h"
#include "llm.h"
#include "asr.h"
#include "router.h"
#include "watch_dog.h"
#include "sync.h"
#include "reactor_mgr.h"
#include "audio_buffer.h"

QueryReactor::QueryReactor(grpc::CallbackServerContext   *ctx,
                           const ::GrpcLibrary::QueryReq *req)
    : _ctx(ctx)
    , _w_queue{conf::instance().sync_write_queue_size()}
    , _ctx_params{hj::llama::context::default_params()}
{
    _session_id = req->id();
    _user_id    = req->user_id();
    _auth       = req->auth();
    _content    = req->content();
    _model      = req->model();

    _sampling_repetition_penalty = req->sampling().repetition_penalty();
    _sampling_temperature        = req->sampling().temperature();
    _sampling_top_p              = req->sampling().top_p();
    _sampling_top_p_min_keep     = req->sampling().top_p_min_keep();
    _sampling_top_k              = req->sampling().top_k();
    _sampling_min_p              = req->sampling().min_p();
    _sampling_min_p_min_keep     = req->sampling().min_p_min_keep();

    _ctx_window_sz  = req->ctx().window_size();
    _ctx_stop_words = req->ctx().stop_words();
    _ctx_window_sz  = (_ctx_window_sz > 128)
                          ? _ctx_window_sz
                          : conf::instance().llm_ctx_window_sz();
    _prompt         = req->ctx().prompt();

    _ctx_params.n_ctx           = req->ctx().n_ctx();
    _ctx_params.n_batch         = req->ctx().n_batch();
    _ctx_params.n_ubatch        = req->ctx().n_ubatch();
    _ctx_params.n_seq_max       = req->ctx().n_seq_max();
    _ctx_params.n_threads       = req->ctx().n_threads();
    _ctx_params.n_threads_batch = req->ctx().n_threads_batch();

    _ctx_params.rope_freq_base   = req->ctx().rope_freq_base();
    _ctx_params.rope_freq_scale  = req->ctx().rope_freq_scale();
    _ctx_params.yarn_ext_factor  = req->ctx().yarn_ext_factor();
    _ctx_params.yarn_attn_factor = req->ctx().yarn_attn_factor();
    _ctx_params.yarn_beta_fast   = req->ctx().yarn_beta_fast();
    _ctx_params.yarn_beta_slow   = req->ctx().yarn_beta_slow();
    _ctx_params.yarn_orig_ctx    = req->ctx().yarn_orig_ctx();
    _ctx_params.defrag_thold     = req->ctx().defrag_thold();

    _ctx_params.embeddings  = req->ctx().embeddings();
    _ctx_params.offload_kqv = req->ctx().offload_kqv();
    _ctx_params.no_perf     = req->ctx().no_perf();
    _ctx_params.op_offload  = req->ctx().op_offload();
    _ctx_params.swa_full    = req->ctx().swa_full();
    _ctx_params.kv_unified  = req->ctx().kv_unified();

    query_reactor_mgr::instance().register_query(_session_id, this);
    thread_pool::instance().enqueue([this]() { this->_process(); });
}

QueryReactor::~QueryReactor()
{
    query_reactor_mgr::instance().unregister_query(_session_id);
}

void QueryReactor::OnWriteDone(bool ok)
{
    _is_writing.store(false);

    if(!ok)
    {
        LOG_ERROR("Write failed or client disconnected for session_id: {}",
                  _session_id);
        _is_cancelled.store(true);
        Finish(grpc::Status(grpc::StatusCode::ABORTED, "Client disconnected"));
        return;
    }

    _push();
}

void QueryReactor::OnDone()
{
    // store the answer to DB
    auto msg_id = static_cast<int64_t>(hj::uuid::gen_u64());
    auto now    = hj::date_time::now().ms_since_epoch();
    auto sql    = hj::sqlite::mprintf(SQL_INSERT_MESSAGE,
                                      msg_id,
                                      _session_id,
                                      ROLE_ASSISTANT,
                                      _answer.c_str(),
                                      NONE_MSG_ID,
                                      now);
    if(db_mgr::instance().exec(DB_SQLITE, sql) != OK)
    {
        LOG_ERROR("Failed to insert assistant message for session_id: {}",
                  _session_id);
        _send("", true, ERR_SQLITE_EXEC_FAIL);
        return;
    }

    delete this;
}

void QueryReactor::Stop()
{
    LOG_INFO("Query {} was stopped by user", _session_id);
    _is_cancelled.store(true);

    // clear the queue
}

void QueryReactor::_process()
{
    int64_t msg_id = static_cast<int64_t>(hj::uuid::gen_u64());
    auto    now    = hj::date_time::now().ms_since_epoch();
    auto    sql    = hj::sqlite::mprintf(SQL_INSERT_MESSAGE,
                                         msg_id,
                                         _session_id,
                                         ROLE_USER,
                                         _content.c_str(),
                                         NONE_MSG_ID,
                                         now);
    if(db_mgr::instance().exec(DB_SQLITE, sql) != OK)
    {
        _send("", true, ERR_SQLITE_EXEC_FAIL);
        return;
    }

    auto tokens = llm_mgr::instance().tokenize(_model, _content, true, true);
    auto params = llm_mgr::instance().create_ctx_params();
    auto smpl_params = llm_mgr::instance().create_sampler_params();

    // create sampler
    hj::llama::sampler sampler{};
    sampler.init_top_k(_sampling_top_k);
    sampler.init_top_p(_sampling_top_p, _sampling_top_p_min_keep);
    sampler.init_min_p(_sampling_min_p, _sampling_min_p_min_keep);
    sampler.init_temp(_sampling_temperature);
    sampler.init_dist(_sampling_seed);

    auto      max_repeates = conf::instance().llm_max_repeats();
    watch_dog dog{max_repeates};
    auto      ec = llm_mgr::instance().loop_query(
        _model,
        tokens,
        params,
        sampler,
        [&](std::string &output) -> bool {
            // 1. if disconnect, stop
            // 2. if write too much repeat words (bug), stop
            // 3. if client stop answer, stop
            // 4. ...
            if(_is_cancelled.load())
            {
                LOG_DEBUG("is_cancelled.load() is true, stop query");
                // send final words
                _send(output, true, OK);
                Finish(grpc::Status::OK);
                return false;
            }

            if(output.empty())
                return true;

            if(!dog.watch(output))
            {
                LOG_DEBUG("dog.watch() is false, stop query");
                _send(output, true, OK);
                return false;
            }

            _answer += output;
            _send(output, false, OK);
            return true;
        });

    if(_is_cancelled.load())
    {
        LOG_INFO("Query {} was cancelled", _session_id);
        return;
    }

    // send final words
    LOG_INFO("Query {} finished with error code {}", _session_id, ec);
    _send("", true, ec);
    Finish(grpc::Status::OK);
}

void QueryReactor::_send(const std::string &text,
                         bool               is_finished,
                         int                error_code)
{
    ::GrpcLibrary::QueryResp resp;
    resp.set_error_code(error_code);
    resp.set_id(_session_id);
    resp.set_content(text);
    resp.set_is_finished(is_finished);

    _w_queue.enqueue(resp);
    _push();
}

void QueryReactor::_push()
{
    if(_is_writing.load())
        return;

    ::GrpcLibrary::QueryResp resp;
    while(_w_queue.try_dequeue(resp))
    {
        _is_writing.store(true);
        StartWrite(&resp);
        LOG_DEBUG("QueryReactor::_push: session_id: {}, user_id: {}, auth: {}, "
                  "content: {}, model: {}, is_finished: {}, error_code: {}",
                  _session_id,
                  _user_id,
                  _auth,
                  resp.content(),
                  _model,
                  resp.is_finished(),
                  resp.error_code());
    }
}

// ------------------------------------------ ReocgnizeReactor -------------------------------
RecognizeReactor::RecognizeReactor(grpc::CallbackServerContext *ctx)
    : _ctx(ctx)
    , _session_id(0)
    , _w_queue{conf::instance().sync_write_queue_size()}
    , _is_registered(false)
    , _is_cancelled(false)
    , _is_writing(false)
    , _is_processing(false)
    , _audio_buffer(conf::instance().asr_audio_buffer_size())
{
    LOG_DEBUG("RecognizeReactor entry");

    // start reading the first request
    StartRead(&_req);
}

RecognizeReactor::~RecognizeReactor()
{
    if(_is_registered.load() && _session_id != 0)
    {
        recognize_reactor_mgr::instance().unregister_recognize(_session_id);
    }
    LOG_DEBUG("RecognizeReactor exit for session_id: {}", _session_id);
}

void RecognizeReactor::_ensure_registered(int64_t session_id)
{
    if(_is_registered.load())
        return;

    std::lock_guard<std::mutex> lock(_mu);
    if(_is_registered.load())
        return;

    _session_id = session_id;
    recognize_reactor_mgr::instance().register_recognize(_session_id, this);
    _is_registered.store(true);
    LOG_DEBUG("Registered RecognizeReactor with session_id: {}", _session_id);
}

void RecognizeReactor::OnReadDone(bool ok)
{
    LOG_DEBUG("OnReadDone called with ok={}, session_id: {}", ok, _session_id);

    if(!ok)
    {
        LOG_DEBUG("Read done with !ok, finishing");
        Finish(grpc::Status::OK);
        return;
    }

    if(_req.session_id() != 0 && !_is_registered.load())
    {
        _ensure_registered(_req.session_id());
    }

    _process(_req);

    StartRead(&_req);
}

void RecognizeReactor::OnWriteDone(bool ok)
{
    LOG_DEBUG("OnWriteDone called with ok={}, session_id: {}", ok, _session_id);
    _is_writing.store(false);

    if(!ok)
    {
        LOG_WARN("Write failed for session_id: {}", _session_id);
        _is_cancelled.store(true);

        ::GrpcLibrary::RecognizeResp resp;
        while(_w_queue.try_dequeue(resp))
        {
        }

        Finish(grpc::Status(grpc::StatusCode::ABORTED, "Client disconnected"));
        return;
    }

    _flush();
}

void RecognizeReactor::OnDone()
{
    LOG_DEBUG("OnDone called for session_id: {}", _session_id);
    delete this;
}

void RecognizeReactor::OnCancel()
{
    LOG_INFO("OnCancel called for session_id: {}", _session_id);
    _is_cancelled.store(true);
    _is_writing.store(false);

    ::GrpcLibrary::RecognizeResp resp;
    while(_w_queue.try_dequeue(resp))
    {
    }
}

void RecognizeReactor::Stop()
{
    LOG_INFO("RecognizeReactor {} was stopped by user", _session_id);
    _is_cancelled.store(true);
    _is_writing.store(false);

    ::GrpcLibrary::RecognizeResp resp;
    while(_w_queue.try_dequeue(resp))
    {
    }
}

void RecognizeReactor::_process(const ::GrpcLibrary::RecognizeReq &req)
{
    // check if registered
    if(!_is_registered.load() && req.session_id() != 0)
    {
        _ensure_registered(req.session_id());
    }

    // Check if cancelled
    if(_is_cancelled.load())
    {
        LOG_DEBUG("Process cancelled for session_id: {}", _session_id);
        return;
    }

    LOG_DEBUG("RecognizeReactor::_process: ctx_id: {}, has_audio_chunk: {}, "
              "has_param: {}, _session_id: {}",
              req.ctx_id(),
              req.has_audio_chunk(),
              req.has_param(),
              req.session_id());
    int         err         = ERR_FAIL;
    std::string text        = "";
    int64_t     session_id  = req.session_id();
    bool        is_finished = true;
    double      confidence  = 0.0;

    // ctx_id
    auto ctx_id = req.ctx_id();
    if(req.has_param())
    {
        LOG_DEBUG("RecognizeReactor::_process param: ctx_id: {}", ctx_id);
        // full param
        auto tmp                     = req.param();
        _params                      = hj::asr::context::default_full_params();
        _params.n_threads            = tmp.n_threads();
        _params.n_max_text_ctx       = tmp.n_max_text_ctx();
        _params.offset_ms            = tmp.offset_ms();
        _params.duration_ms          = tmp.duration_ms();
        _params.translate            = tmp.translate();
        _params.detect_language      = tmp.detect_language();
        _params.language             = "auto";
        _params.no_context           = tmp.no_ctx();
        _params.no_timestamps        = tmp.no_timestamps();
        _params.single_segment       = tmp.single_segment();
        _params.print_special        = tmp.print_special();
        _params.print_progress       = tmp.print_progress();
        _params.print_realtime       = tmp.print_realtime();
        _params.print_timestamps     = tmp.print_timestamps();
        _params.carry_initial_prompt = tmp.carry_initial_prompt();
        _params.initial_prompt       = "";
        _params.suppress_regex       = "";
        _params.suppress_blank       = tmp.suppress_blank();
        _params.suppress_nst         = tmp.suppress_nst();
        _params.temperature          = tmp.temperature();
        _params.temperature_inc      = tmp.temperature_inc();
        _params.max_initial_ts       = tmp.max_initial_ts();
        _params.length_penalty       = tmp.length_penalty();
        _params.entropy_thold        = tmp.entropy_thold();
        _params.logprob_thold        = tmp.logprob_thold();
        _params.no_speech_thold      = tmp.no_speech_thold();

        err         = OK;
        is_finished = false;
    }

    if(req.has_audio_chunk())
    {
        // fcm
        auto               chunk = req.audio_chunk();
        std::vector<float> data;
        hj::asr::context::convert(data, chunk);
        // append to audio buffer
        _audio_buffer.push(data.data(), data.size());
        LOG_DEBUG("RecognizeReactor::_process audio chunk: ctx_id: {}, "
                  "pushed {} samples to audio buffer, session_id: {}",
                  ctx_id,
                  data.size(),
                  session_id);

        // process the audio buffer in a separate thread to avoid blocking the reactor
        thread_pool::instance().enqueue([this, ctx_id, session_id]() {
            if(_is_cancelled.load() || _is_processing.load())
                return;

            _is_processing.store(true);
            // min size of audio at 16kHz
            size_t required_sz = conf::instance().asr_audio_min_chunk_size();
            std::vector<float> data(required_sz);
            while(!_is_cancelled.load() && _is_processing.load())
            {
                auto actual = this->_audio_buffer.pop(data.data(), required_sz);
                if(actual == 0)
                {
                    _is_processing.store(false);
                    return;
                }

                if(actual < required_sz)
                {
                    LOG_WARN(
                        "RecognizeReactor::_process: not enough audio data "
                        "after pop, "
                        "actual size: {}, required size: {}, session_id: {}",
                        actual,
                        required_sz,
                        session_id);
                }
                std::string segment;
                auto        ec = asr_mgr::instance().translate(segment,
                                                               ctx_id,
                                                               data,
                                                               this->_params);
                LOG_DEBUG("_process with ec:{}, ctx_id:{}, segment: {}",
                          ec,
                          ctx_id,
                          segment);
                if(ec != OK)
                {
                    LOG_ERROR("asr_mgr::instance().translate failed with error "
                              "code: {}",
                              ec);
                    _send(ec, session_id, "", true, 0.0);
                    _is_processing.store(false);
                    return;
                }
                _send(OK, session_id, segment, false, 0.0);
            }
        });

        return;
    }

    // param only, no audio chunk, send back the param ack
    _send(err, session_id, text, is_finished, confidence);
}

void RecognizeReactor::_send(const int          error_code,
                             const int64_t      session_id,
                             const std::string &text,
                             const bool         is_finished,
                             const double       confidence)
{
    if(_is_cancelled.load())
        return;

    ::GrpcLibrary::RecognizeResp resp;
    resp.set_error_code(error_code);
    resp.set_session_id(session_id);
    resp.set_transcript(text);
    resp.set_is_finished(is_finished);
    resp.set_confidence(confidence);

    _w_queue.enqueue(resp);
    _flush();
}

void RecognizeReactor::_flush()
{
    if(_is_writing.load())
        return;

    if(_is_cancelled.load())
        return;

    ::GrpcLibrary::RecognizeResp resp;
    if(!_w_queue.try_dequeue(resp))
        return;

    _is_writing.store(true);
    StartWrite(&resp);
    LOG_DEBUG("RecognizeReactor::_flush: session_id: {}, transcript: {}, "
              "is_finished: {}, error_code: {}",
              resp.session_id(),
              resp.transcript(),
              resp.is_finished(),
              resp.error_code());
}