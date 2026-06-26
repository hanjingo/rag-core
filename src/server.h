#ifndef SERVER_H
#define SERVER_H

#include <hj/net/grpc.hpp>
#include "api.grpc.pb.h"

#include "db_mgr.h"

using status_t = ::grpc::Status;
using ctx_t    = ::grpc::ServerContext;

class api_handler final : public GrpcLibrary::GrpcService::Service
{
  public:
    static api_handler &instance()
    {
        static api_handler handler;
        return handler;
    }
    status_t Login(ctx_t                         *ctx,
                   const ::GrpcLibrary::LoginReq *req,
                   ::GrpcLibrary::LoginResp      *resp) override;
    status_t Logout(ctx_t                          *ctx,
                    const ::GrpcLibrary::LogoutReq *req,
                    ::GrpcLibrary::LogoutResp      *resp) override;
    status_t RegAccount(ctx_t                              *ctx,
                        const ::GrpcLibrary::RegAccountReq *req,
                        ::GrpcLibrary::RegAccountResp      *resp) override;
    status_t
    Query(ctx_t                                        *ctx,
          const ::GrpcLibrary::QueryReq                *req,
          grpc::ServerWriter<::GrpcLibrary::QueryResp> *writer) override;

    status_t GetMessageInfo(ctx_t                                  *ctx,
                            const ::GrpcLibrary::GetMessageInfoReq *req,
                            ::GrpcLibrary::GetMessageInfoResp *resp) override;

    status_t GetSession(ctx_t                              *ctx,
                        const ::GrpcLibrary::GetSessionReq *req,
                        ::GrpcLibrary::GetSessionResp      *resp) override;

    status_t NewSession(ctx_t                              *ctx,
                        const ::GrpcLibrary::NewSessionReq *req,
                        ::GrpcLibrary::NewSessionResp      *resp) override;

    status_t
    ModifySessionTitle(ctx_t                                      *ctx,
                       const ::GrpcLibrary::ModifySessionTitleReq *req,
                       ::GrpcLibrary::ModifySessionTitleResp *resp) override;

    status_t DelSession(ctx_t                              *ctx,
                        const ::GrpcLibrary::DelSessionReq *req,
                        ::GrpcLibrary::DelSessionResp      *resp) override;

    status_t GetSkillInfo(ctx_t                                *ctx,
                          const ::GrpcLibrary::GetSkillInfoReq *req,
                          ::GrpcLibrary::GetSkillInfoResp      *resp) override;

    status_t Download(ctx_t                            *ctx,
                      const ::GrpcLibrary::DownloadReq *req,
                      ::GrpcLibrary::DownloadResp      *resp) override;
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