#ifndef SERVER_H
#define SERVER_H

#include <hj/net/grpc.hpp>
#include <hj/sync/safe_map.hpp>

#include "api.grpc.pb.h"

#include "db_mgr.h"
#include "server_reactor.h"

using status_t  = ::grpc::Status;
using reactor_t = grpc::ServerUnaryReactor;
using ctx_t     = ::grpc::CallbackServerContext;

class api_handler final : public GrpcLibraryV1::GrpcService::CallbackService
{
  public:
    static api_handler &instance()
    {
        static api_handler handler;
        return handler;
    }

    reactor_t *Heartbeat(ctx_t                       *ctx,
                         const ::GrpcLibraryV1::Ping *req,
                         ::GrpcLibraryV1::Pong       *resp) override;

    reactor_t *Login(ctx_t                           *ctx,
                     const ::GrpcLibraryV1::LoginReq *req,
                     ::GrpcLibraryV1::LoginResp      *resp) override;

    reactor_t *Logout(ctx_t                            *ctx,
                      const ::GrpcLibraryV1::LogoutReq *req,
                      ::GrpcLibraryV1::LogoutResp      *resp) override;

    reactor_t *RegAccount(ctx_t                                *ctx,
                          const ::GrpcLibraryV1::RegAccountReq *req,
                          ::GrpcLibraryV1::RegAccountResp      *resp) override;

    reactor_t *
    GetMessageInfo(ctx_t                                    *ctx,
                   const ::GrpcLibraryV1::GetMessageInfoReq *req,
                   ::GrpcLibraryV1::GetMessageInfoResp      *resp) override;

    reactor_t *GetSession(ctx_t                                *ctx,
                          const ::GrpcLibraryV1::GetSessionReq *req,
                          ::GrpcLibraryV1::GetSessionResp      *resp) override;

    reactor_t *NewSession(ctx_t                                *ctx,
                          const ::GrpcLibraryV1::NewSessionReq *req,
                          ::GrpcLibraryV1::NewSessionResp      *resp) override;

    reactor_t *
    ModifySessionTitle(ctx_t                                        *ctx,
                       const ::GrpcLibraryV1::ModifySessionTitleReq *req,
                       ::GrpcLibraryV1::ModifySessionTitleResp *resp) override;

    reactor_t *DelSession(ctx_t                                *ctx,
                          const ::GrpcLibraryV1::DelSessionReq *req,
                          ::GrpcLibraryV1::DelSessionResp      *resp) override;

    reactor_t *GetPluginInfo(ctx_t                                   *ctx,
                             const ::GrpcLibraryV1::GetPluginInfoReq *req,
                             ::GrpcLibraryV1::GetPluginInfoResp *resp) override;

    reactor_t *Download(ctx_t                              *ctx,
                        const ::GrpcLibraryV1::DownloadReq *req,
                        ::GrpcLibraryV1::DownloadResp      *resp) override;

    reactor_t *Upload(ctx_t                            *ctx,
                      const ::GrpcLibraryV1::UploadReq *req,
                      ::GrpcLibraryV1::UploadResp      *resp) override;

    reactor_t *StopAnswer(ctx_t                                *ctx,
                          const ::GrpcLibraryV1::StopAnswerReq *req,
                          ::GrpcLibraryV1::StopAnswerResp      *resp) override;

    grpc::ServerWriteReactor<::GrpcLibraryV1::QueryResp> *
    Query(grpc::CallbackServerContext     *ctx,
          const ::GrpcLibraryV1::QueryReq *req) override;

    grpc::ServerBidiReactor<GrpcLibraryV1::RecognizeReq,
                            GrpcLibraryV1::RecognizeResp> *
    Recognize(grpc::CallbackServerContext *context) override;

    reactor_t *StopRecognize(ctx_t                                   *ctx,
                             const ::GrpcLibraryV1::StopRecognizeReq *req,
                             ::GrpcLibraryV1::StopRecognizeResp *resp) override;

    grpc::ServerBidiReactor<GrpcLibraryV1::EmbeddingReq,
                            GrpcLibraryV1::EmbeddingResp> *
    Embedding(grpc::CallbackServerContext *context) override;

    reactor_t *StopEmbedding(ctx_t                                   *ctx,
                             const ::GrpcLibraryV1::StopEmbeddingReq *req,
                             ::GrpcLibraryV1::StopEmbeddingResp *resp) override;
};

class server
{
  public:
    server(const std::string &address)
        : _srv{}
        , _address(address) {};
    virtual ~server() { _srv.stop(); };

    inline bool start()
    {
        return _srv.start(_address, &api_handler::instance());
    };

    inline void stop() { _srv.stop(); }

    inline bool is_running() { return _srv.is_running(); }

    inline std::string address() { return _address; }

  private:
    hj::grpc_server _srv;
    std::string     _address;
};

#endif