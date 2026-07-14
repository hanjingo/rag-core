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

class api_handler final : public GrpcLibrary::GrpcService::CallbackService
{
  public:
    static api_handler &instance()
    {
        static api_handler handler;
        return handler;
    }

    reactor_t *Heartbeat(ctx_t                     *ctx,
                         const ::GrpcLibrary::Ping *req,
                         ::GrpcLibrary::Pong       *resp) override;

    reactor_t *Login(ctx_t                         *ctx,
                     const ::GrpcLibrary::LoginReq *req,
                     ::GrpcLibrary::LoginResp      *resp) override;

    reactor_t *Logout(ctx_t                          *ctx,
                      const ::GrpcLibrary::LogoutReq *req,
                      ::GrpcLibrary::LogoutResp      *resp) override;

    reactor_t *RegAccount(ctx_t                              *ctx,
                          const ::GrpcLibrary::RegAccountReq *req,
                          ::GrpcLibrary::RegAccountResp      *resp) override;

    reactor_t *GetMessageInfo(ctx_t                                  *ctx,
                              const ::GrpcLibrary::GetMessageInfoReq *req,
                              ::GrpcLibrary::GetMessageInfoResp *resp) override;

    reactor_t *GetSession(ctx_t                              *ctx,
                          const ::GrpcLibrary::GetSessionReq *req,
                          ::GrpcLibrary::GetSessionResp      *resp) override;

    reactor_t *NewSession(ctx_t                              *ctx,
                          const ::GrpcLibrary::NewSessionReq *req,
                          ::GrpcLibrary::NewSessionResp      *resp) override;

    reactor_t *
    ModifySessionTitle(ctx_t                                      *ctx,
                       const ::GrpcLibrary::ModifySessionTitleReq *req,
                       ::GrpcLibrary::ModifySessionTitleResp *resp) override;

    reactor_t *DelSession(ctx_t                              *ctx,
                          const ::GrpcLibrary::DelSessionReq *req,
                          ::GrpcLibrary::DelSessionResp      *resp) override;

    reactor_t *GetSkillInfo(ctx_t                                *ctx,
                            const ::GrpcLibrary::GetSkillInfoReq *req,
                            ::GrpcLibrary::GetSkillInfoResp *resp) override;

    reactor_t *Download(ctx_t                            *ctx,
                        const ::GrpcLibrary::DownloadReq *req,
                        ::GrpcLibrary::DownloadResp      *resp) override;

    reactor_t *Upload(ctx_t                          *ctx,
                      const ::GrpcLibrary::UploadReq *req,
                      ::GrpcLibrary::UploadResp      *resp) override;

    reactor_t *StopAnswer(ctx_t                              *ctx,
                          const ::GrpcLibrary::StopAnswerReq *req,
                          ::GrpcLibrary::StopAnswerResp      *resp) override;

    grpc::ServerWriteReactor<::GrpcLibrary::QueryResp> *
    Query(grpc::CallbackServerContext   *ctx,
          const ::GrpcLibrary::QueryReq *req) override;

    grpc::ServerBidiReactor<GrpcLibrary::RecognizeReq,
                            GrpcLibrary::RecognizeResp> *
    Recognize(grpc::CallbackServerContext *context) override;

    reactor_t *StopRecognize(ctx_t                                 *ctx,
                             const ::GrpcLibrary::StopRecognizeReq *req,
                             ::GrpcLibrary::StopRecognizeResp *resp) override;

    grpc::ServerBidiReactor<GrpcLibrary::EmbeddingReq,
                            GrpcLibrary::EmbeddingResp> *
    Embedding(grpc::CallbackServerContext *context) override;

    reactor_t *StopEmbedding(ctx_t                                 *ctx,
                             const ::GrpcLibrary::StopEmbeddingReq *req,
                             ::GrpcLibrary::StopEmbeddingResp *resp) override;
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