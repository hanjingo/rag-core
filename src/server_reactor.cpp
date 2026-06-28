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
#include "router.h"
#include "watch_dog.h"
#include "sync.h"
#include "reactor_mgr.h"

QueryReactor::QueryReactor(grpc::CallbackServerContext   *ctx,
                           const ::GrpcLibrary::QueryReq *req)
    : _ctx(ctx)
    , _w_queue{conf::instance().sync_write_queue_size()}
{
    _session_id = req->id();
    _user_id    = req->user_id();
    _auth       = req->auth();
    _content    = req->content();
    _model      = req->model();

    _sampling_repetition_penalty = req->sampling().repetition_penalty();
    _sampling_temperature        = req->sampling().temperature();
    _sampling_top_p              = req->sampling().top_p();
    _sampling_top_k              = req->sampling().top_k();
    _sampling_min_p              = req->sampling().min_p();

    _ctx_window_sz  = req->ctx().window_size();
    _ctx_stop_words = req->ctx().stop_words();
    _ctx_window_sz  = (_ctx_window_sz > 128)
                          ? _ctx_window_sz
                          : conf::instance().llm_ctx_window_sz();

    query_reactor_mgr::instance().register_query(_session_id, this);
    thread_pool::instance().enqueue([this]() { this->_process(); });
}

QueryReactor::~QueryReactor()
{
    query_reactor_mgr::instance().unregister_query(_session_id);
}

void QueryReactor::OnWriteDone(bool ok)
{
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
    // text context, 0 = from model
    params.n_ctx = _ctx_window_sz;
    // // logical maximum batch size that can be submitted to llama_decode
    // params.n_batch;
    // //  physical maximum batch size
    // params.n_ubatch;
    // // max number of sequences (i.e. distinct states for recurrent models)
    // params.n_seq_max;
    // // number of threads to use for generation
    // params.n_threads;
    // // number of threads to use for batch processing
    // params.n_threads_batch;

    auto      max_repeates = conf::instance().llm_max_repeats();
    watch_dog dog{max_repeates};
    LOG_DEBUG("QueryReactor::_process: session_id: {}, user_id: {}, auth: {}, "
              "content: {}, model: {}, tokens size: {}, max_repeats: {}",
              _session_id,
              _user_id,
              _auth,
              _content,
              _model,
              tokens.size(),
              max_repeates);
    auto ec = llm_mgr::instance().loop_query(
        _model,
        tokens,
        params,
        [&](std::string &output) -> bool {
            // 1. if disconnect, stop
            // 2. if write too much repeat words (bug), stop
            // 3. if client stop answer, stop
            // 4. ...
            if(_is_cancelled.load())
            {
                LOG_DEBUG("is_cancelled.load() is true, stop query");
                // send final words
                _send("", true, QUERY_CANCELLED);
                Finish(grpc::Status(grpc::StatusCode::CANCELLED,
                                    "Cancelled by user"));
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

            LOG_DEBUG("QueryReactor::_process: session_id: {}, user_id: {}, "
                      "content: {}, model: {}, "
                      "output size: {}, answer size: {}",
                      _session_id,
                      _user_id,
                      _content,
                      _model,
                      output.size(),
                      _answer.size());
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
    if(_is_cancelled.load())
    {
        // Finish(grpc::Status::CANCELLED);
        return;
    }

    ::GrpcLibrary::QueryResp resp;
    while(_w_queue.try_dequeue(resp))
    {
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