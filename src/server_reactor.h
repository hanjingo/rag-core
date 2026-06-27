#ifndef SERVER_REACTOR_H
#define SERVER_REACTOR_H

#include <thread>
#include <queue>
#include <mutex>
#include <condition_variable>

#include <hj/net/grpc.hpp>

#include "sync.h"
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

  private:
    void _process();

    void _send(const std::string &text, bool is_finished, int error_code);

    void _push();

  private:
    grpc::CallbackServerContext          *_ctx;
    std::atomic<bool>                     _is_cancelled{false};
    hj::channel<::GrpcLibrary::QueryResp> _w_queue;

    // params
    int64_t     _session_id;
    int64_t     _user_id;
    std::string _auth;
    std::string _content;
    std::string _model;

    // sampling params
    float _sampling_repetition_penalty;
    float _sampling_temperature;
    float _sampling_top_p;
    float _sampling_top_k;
    float _sampling_min_p;

    // context params
    int32_t     _ctx_window_sz;
    std::string _ctx_stop_words;

    // resp
    std::string _answer;
};

#endif // SERVER_REACTOR_H