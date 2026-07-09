#ifndef SERVER_REACTOR_H
#define SERVER_REACTOR_H

#include <thread>
#include <queue>
#include <mutex>
#include <condition_variable>

#include <hj/net/grpc.hpp>
#include <hj/ai/asr.hpp>
#include <hj/encoding/json.hpp>
#include <hj/net/http/http_request.hpp>

#include "sync.h"
#include "audio_buffer.h"
#include "api.grpc.pb.h"

class QueryReactor : public grpc::ServerWriteReactor<::GrpcLibrary::QueryResp>
{
  public:
    QueryReactor(grpc::CallbackServerContext   *ctx,
                 const ::GrpcLibrary::QueryReq *req);
    QueryReactor::~QueryReactor();

    void OnWriteDone(bool ok) override;

    void OnDone() override;

    void OnCancel() override { _is_cancelled.store(true); }

    void Stop();

  private:
    void _process();
    void _processRemote();

    void _send(const std::string &text, bool is_finished, int error_code);

    void _flush();

  private:
    grpc::CallbackServerContext          *_ctx;
    std::atomic<bool>                     _is_cancelled{false};
    std::atomic<bool>                     _is_writing{false};
    hj::channel<::GrpcLibrary::QueryResp> _w_queue;

    // base params
    int64_t     _session_id;
    int64_t     _user_id;
    std::string _auth;
    std::string _content;
    std::string _model;
    std::string _pipeline;

    // sampling params
    hj::llama::sampler::params _smpl_params;

    // context params
    std::string                 _prompt;
    int32_t                     _ctx_window_sz;
    std::string                 _ctx_stop_words;
    hj::llama::context_params_t _ctx_params;

    // remote api params
    std::string      _api_key;
    hj::http_request _api_req;

    // resp
    std::string _answer;
};

class RecognizeReactor
    : public grpc::ServerBidiReactor<::GrpcLibrary::RecognizeReq,
                                     ::GrpcLibrary::RecognizeResp>
{
  public:
    RecognizeReactor(grpc::CallbackServerContext *ctx);
    ~RecognizeReactor();

    void OnReadDone(bool ok) override;
    void OnWriteDone(bool ok) override;
    void OnDone() override;
    void OnCancel() override;
    void Stop();

    inline int64_t GetSessionId() const { return _session_id; }

  private:
    void _process(const ::GrpcLibrary::RecognizeReq &req);
    void _ensure_registered(int64_t session_id);

    void _send(const int          error_code,
               const int64_t      session_id,
               const std::string &text,
               const bool         is_finished,
               const double       confidence = 0.0);
    void _flush();

  private:
    grpc::CallbackServerContext *_ctx;
    ::GrpcLibrary::RecognizeReq  _req;
    ::GrpcLibrary::RecognizeResp _resp;

    hj::channel<::GrpcLibrary::RecognizeResp> _w_queue;

    int64_t                _session_id;
    std::atomic<bool>      _is_cancelled{false};
    std::atomic<bool>      _is_registered{false};
    std::atomic<bool>      _is_writing{false};
    std::atomic<bool>      _is_processing{false};
    std::mutex             _mu;
    std::string            _ctx_id;
    hj::asr::full_params_t _params;
    audio_buffer           _audio_buffer;
};

#endif // SERVER_REACTOR_H