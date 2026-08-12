#include "server.h"

#include <hj/log/logger.hpp>
#include <hj/time/date_time.hpp>
#include <hj/algo/uuid.hpp>
#include <hj/db/sqlite.hpp>
#include <hj/encoding/fmt.hpp>

#include "conf.h"
#include "db_mgr.h"
#include "auth.h"
#include "account_mgr.h"
#include "llm.h"
#include "router.h"
#include "watch_dog.h"
#include "reactor_mgr.h"
#include "updater.h"
#include "mq.h"
#include "err.h"

reactor_t *api_handler::Heartbeat(ctx_t                       *ctx,
                                  const ::GrpcLibraryV1::Ping *req,
                                  ::GrpcLibraryV1::Pong       *resp)
{
    int64_t timestamp = req->timestamp();
    // LOG_DEBUG("Received Heartbeat request. timestamp: {}", timestamp);
    resp->set_timestamp(timestamp);
    auto *reactor = ctx->DefaultReactor();
    reactor->Finish(status_t::OK);
    return reactor;
}

reactor_t *api_handler::Login(ctx_t                           *ctx,
                              const ::GrpcLibraryV1::LoginReq *req,
                              ::GrpcLibraryV1::LoginResp      *resp)
{
    std::string account          = req->account();
    std::string encrypted_passwd = req->passwd();
    std::string platform         = req->platform();
    std::string arch             = req->arch();
    std::string version          = req->client_version();
    int64_t     user_id          = -1;
    auto       *reactor          = ctx->DefaultReactor();

    GrpcLibraryV1::UpdateInfo info;
    auto ok = updater::instance()->check(platform, arch, version);
    info.set_force_update(!ok);

    resp->set_error_code(ERR_FAIL);
    resp->mutable_update_info()->CopyFrom(info);
    LOG_DEBUG("Received Login request. account: {}, platform: {}, "
              "arch: {}, version: {}",
              account,
              platform,
              arch,
              version);

    db_mgr::query_ret ret;
    if(db_mgr::instance().query(ret,
                                SQL_SELECT_USER_BY_USERNAME_PASSWD,
                                account,
                                encrypted_passwd)
           != OK
       || ret.empty())
    {
        LOG_ERROR("Failed to authenticate account: {}", account);
        resp->set_error_code(ACCOUNT_INVALID);

        reactor->Finish(status_t::OK);
        return reactor;
    }
    for(int n_row = 0; n_row < ret.rows(); ++n_row)
    {
        auto row = ret[n_row];
        user_id  = std::get<int64_t>(row[0]);
        break;
    }

    auto                 expired_days = conf::instance().issuer_expired_days();
    hj::license::token_t token;
    if(OK
       != issuer::instance().issue(token,
                                   std::to_string(user_id),
                                   expired_days,
                                   {}))
    {
        LOG_ERROR("Failed to issue license for account: {}", account);
        resp->set_error_code(AUTH_ERR_ISSUE_FAIL);

        reactor->Finish(status_t::OK);
        return reactor;
    }

    LOG_DEBUG("Issued license for user_id: {}, token: {}, expired_days: {}",
              user_id,
              token,
              expired_days);
    resp->set_error_code(OK);
    resp->set_user_id(user_id);
    resp->set_auth(token);
    resp->set_account(account);
    resp->set_last_login_time(
        hj::date_time::format(hj::date_time::now(), TIME_FORMAT));

    reactor->Finish(status_t::OK);
    return reactor;
}

reactor_t *api_handler::Logout(ctx_t                            *ctx,
                               const ::GrpcLibraryV1::LogoutReq *req,
                               ::GrpcLibraryV1::LogoutResp      *resp)
{
    int64_t     user_id = req->user_id();
    std::string auth    = req->auth();
    auto       *reactor = ctx->DefaultReactor();
    resp->set_error_code(ERR_FAIL);
    LOG_DEBUG("Received Logout request. user_id: {}", user_id);

    if(verifier::instance().verify(auth, std::to_string(user_id), {}) != OK)
    {
        LOG_ERROR("Failed to verify auth for user_id: {}", user_id);
        resp->set_error_code(ACCOUNT_INVALID);

        reactor->Finish(status_t::OK);
        return reactor;
    }

    resp->set_error_code(OK);

    reactor->Finish(status_t::OK);
    return reactor;
}

reactor_t *api_handler::RegAccount(ctx_t                                *ctx,
                                   const ::GrpcLibraryV1::RegAccountReq *req,
                                   ::GrpcLibraryV1::RegAccountResp      *resp)
{
    std::string account          = req->account();
    std::string encrypted_passwd = req->passwd();
    auto       *reactor          = ctx->DefaultReactor();
    resp->set_error_code(ERR_FAIL);
    LOG_DEBUG("Received RegAccount request. account: {}", account);

    const int64_t id  = static_cast<int64_t>(hj::uuid::gen_u64());
    auto          sql = hj::sqlite::mprintf(SQL_INSERT_USER,
                                            id,
                                            account.c_str(),
                                            encrypted_passwd.c_str());
    // LOG_DEBUG("{}", sql);
    if(db_mgr::instance().exec(SQL_INSERT_USER, id, account, encrypted_passwd)
       != OK)
    {
        resp->set_error_code(ERR_SQLITE_EXEC_FAIL);
        LOG_ERROR("Failed to insert user with id: {}, account: {}, "
                  "encrypted_passwd: {}",
                  id,
                  account,
                  encrypted_passwd);

        reactor->Finish(status_t::OK);
        return reactor;
    }

    resp->set_user_id(id);
    resp->set_error_code(OK);

    reactor->Finish(status_t::OK);
    return reactor;
}

reactor_t *api_handler::StopAnswer(ctx_t                                *ctx,
                                   const ::GrpcLibraryV1::StopAnswerReq *req,
                                   ::GrpcLibraryV1::StopAnswerResp      *resp)
{
    auto session_id = req->session_id();
    auto user_id    = req->user_id();
    auto auth       = req->auth();
    LOG_DEBUG("Received StopAnswer request. session_id: {}, user_id: {}",
              session_id,
              user_id);

    auto *reactor = ctx->DefaultReactor();
    resp->set_error_code(OK);

    bool stopped = query_reactor_mgr::instance().stop_query(session_id);
    if(!stopped)
    {
        LOG_WARN("No active query found for session_id: {}", session_id);
        resp->set_error_code(ERR_STOP_FAIL);
    } else
    {
        LOG_DEBUG("Successfully stopped query for session_id: {}", session_id);
        resp->set_error_code(OK);
    }

    LOG_DEBUG("StopAnswer request processed for session_id: {}, user_id: {}",
              session_id,
              user_id);
    resp->set_session_id(session_id);
    reactor->Finish(status_t::OK);
    return reactor;
}

grpc::ServerWriteReactor<::GrpcLibraryV1::QueryResp> *
api_handler::Query(grpc::CallbackServerContext     *ctx,
                   const ::GrpcLibraryV1::QueryReq *req)
{
    return new QueryReactor(ctx, req);
}

grpc::ServerBidiReactor<GrpcLibraryV1::RecognizeReq,
                        GrpcLibraryV1::RecognizeResp> *
api_handler::Recognize(grpc::CallbackServerContext *context)
{
    LOG_INFO("Received Recognize bidirectional stream request from peer: {}",
             context->peer());

    return new RecognizeReactor(context);
}

reactor_t *
api_handler::StopRecognize(ctx_t                                   *ctx,
                           const ::GrpcLibraryV1::StopRecognizeReq *req,
                           ::GrpcLibraryV1::StopRecognizeResp      *resp)
{
    auto session_id = req->session_id();
    auto user_id    = req->user_id();
    auto auth       = req->auth();
    LOG_DEBUG("Received StopRecognize request. session_id: {}, user_id: {}",
              session_id,
              user_id);

    auto *reactor = ctx->DefaultReactor();
    resp->set_error_code(OK);
    resp->set_session_id(session_id);

    // bool stopped = recognize_reactor_mgr::instance().stop_recognize(session_id);
    // if(!stopped)
    // {
    //     LOG_WARN("No active recognize found for session_id: {}", session_id);
    //     resp->set_error_code(ERR_STOP_FAIL);
    // } else
    // {
    //     LOG_DEBUG("Successfully stopped recognize for session_id: {}",
    //               session_id);
    //     resp->set_error_code(OK);
    // }

    reactor->Finish(status_t::OK);
    return reactor;
}

reactor_t *
api_handler::GetChatMessage(ctx_t                                    *ctx,
                            const ::GrpcLibraryV1::GetChatMessageReq *req,
                            ::GrpcLibraryV1::GetChatMessageResp      *resp)
{
    int64_t     id         = req->id();
    int64_t     session_id = req->session_id();
    int32_t     limit      = req->limit();
    int64_t     user_id    = req->user_id();
    std::string auth       = req->auth();
    auto       *reactor    = ctx->DefaultReactor();

    if(limit < 0 || limit > conf::instance().param_query_limit())
        limit = conf::instance().param_query_limit();
    resp->set_error_code(ERR_FAIL);
    LOG_DEBUG(
        "Received GetChatMessage request. id: {}, session_id: {}, user_id: "
        "{}, limit: {}",
        id,
        session_id,
        user_id,
        limit);

    db_mgr::query_ret rows;
    if(id != -1)
    {
        if(db_mgr::instance().query(rows, SQL_SELECT_MESSAGE_BY_ID, id) != OK)
        {
            LOG_ERROR("Failed to query message for id: {}, limit: {}",
                      id,
                      limit);
            resp->set_error_code(ERR_SQLITE_EXEC_FAIL);

            reactor->Finish(status_t::OK);
            return reactor;
        }
    } else
    {
        if(db_mgr::instance().query(rows,
                                    SQL_SELECT_MESSAGE_BY_SESSION_ID,
                                    session_id,
                                    limit)
           != OK)
        {
            LOG_ERROR("Failed to query message for session_id: {}, limit: {}",
                      session_id,
                      limit);
            resp->set_error_code(ERR_SQLITE_EXEC_FAIL);

            reactor->Finish(status_t::OK);
            return reactor;
        }
    }
    for(int n_row = 0; n_row < rows.rows(); ++n_row)
    {
        auto id         = rows.get_or<int64_t>(n_row, 0, -1);
        auto session_id = rows.get_or<int64_t>(n_row, 1, -1);
        auto role       = rows.get_or<std::string>(n_row, 2, "");
        auto content    = rows.get_or<std::string>(n_row, 3, "");
        auto msg_id     = rows.get_or<int64_t>(n_row, 4, -1);
        auto ms         = rows.get_or<int64_t>(n_row, 5, 0);

        auto item = resp->add_messages();
        item->set_id(id);
        item->set_session_id(session_id);
        item->set_role(role);
        item->set_content(content);
        item->set_prev_message_id(msg_id);
        item->set_timestamp(
            hj::date_time::format(hj::date_time::from_ms_since_epoch(ms),
                                  TIME_FORMAT));
        LOG_DEBUG(
            "GetChatMessage id: {}, session_id: {}, role: {}, content: {}, "
            "prev_message_id: {}, timestamp: {}",
            item->id(),
            item->session_id(),
            item->role(),
            item->content(),
            item->prev_message_id(),
            item->timestamp());
    }

    resp->set_error_code(OK);
    reactor->Finish(status_t::OK);
    return reactor;
}

reactor_t *api_handler::GetSession(ctx_t                                *ctx,
                                   const ::GrpcLibraryV1::GetSessionReq *req,
                                   ::GrpcLibraryV1::GetSessionResp      *resp)
{
    int64_t     id      = req->id();
    int64_t     user_id = req->user_id();
    std::string auth    = req->auth();
    int         limit   = req->limit();
    limit               = (limit < 0 || limit > 100) ? 100 : limit;
    auto *reactor       = ctx->DefaultReactor();
    resp->set_error_code(ERR_FAIL);
    LOG_DEBUG("Received GetSession request. id: {}, user_id: {}, limit: {}",
              id,
              user_id,
              limit);

    std::string sql;
    if(id > 0)
        sql = hj::sqlite::mprintf(SQL_SELECT_SESSION_BY_ID, id) + " LIMIT 1;";
    else
        sql = hj::sqlite::mprintf(SQL_SELECT_SESSION_BY_USER_ID, user_id)
              + hj::sqlite::mprintf(" LIMIT %d;", limit);

    db_mgr::query_ret rows;
    if(id > 0)
    {
        if(db_mgr::instance().query(rows, SQL_SELECT_SESSION_BY_ID, id, 1)
           != OK)
        {
            LOG_ERROR("Failed to query history for id: {}", id);
            resp->set_error_code(ERR_SQLITE_EXEC_FAIL);

            // return status_t::OK;
            reactor->Finish(status_t::OK);
            return reactor;
        }
    } else
    {
        if(db_mgr::instance().query(rows,
                                    SQL_SELECT_SESSION_BY_USER_ID,
                                    user_id,
                                    limit)
           != OK)
        {
            LOG_ERROR("Failed to query history for user_id: {}", user_id);
            resp->set_error_code(ERR_SQLITE_EXEC_FAIL);

            // return status_t::OK;
            reactor->Finish(status_t::OK);
            return reactor;
        }
    }

    for(int n_row = 0; n_row < rows.rows(); ++n_row)
    {
        auto id      = rows.get_or<int64_t>(n_row, 0, -1);
        auto user_id = rows.get_or<int64_t>(n_row, 1, -1);
        auto title   = rows.get_or<std::string>(n_row, 2, "");
        auto ms      = rows.get_or<int64_t>(n_row, 3, 0);

        auto item = resp->add_sessions();
        item->set_id(id);
        item->set_user_id(user_id);
        item->set_title(title);
        item->set_timestamp(
            hj::date_time::format(hj::date_time::from_ms_since_epoch(ms),
                                  TIME_FORMAT));

        LOG_DEBUG("GetSession id: {}, user_id: {}, title: {}, timestamp: {}",
                  item->id(),
                  item->user_id(),
                  item->title(),
                  item->timestamp());
    }

    resp->set_error_code(OK);
    // return status_t::OK;
    reactor->Finish(status_t::OK);
    return reactor;
}

reactor_t *api_handler::NewSession(ctx_t                                *ctx,
                                   const ::GrpcLibraryV1::NewSessionReq *req,
                                   ::GrpcLibraryV1::NewSessionResp      *resp)
{
    resp->set_error_code(ERR_FAIL);
    int64_t     user_id = req->user_id();
    std::string auth    = req->auth();
    std::string title   = req->title();
    std::string content = req->content();
    std::string model   = req->model();
    auto       *reactor = ctx->DefaultReactor();
    std::string answer;
    long long   ms = hj::date_time::now().ms_since_epoch();
    LOG_DEBUG("Received NewSession request. user_id: {}, title: {}, "
              "content: {}, model: {}",
              user_id,
              title,
              content,
              model);

    int64_t id = static_cast<int64_t>(hj::uuid::gen_u64());
    if(db_mgr::instance().exec(SQL_INSERT_SESSION, id, user_id, title, ms)
       != OK)
    {
        resp->set_error_code(ERR_SQLITE_EXEC_FAIL);
        LOG_ERROR("Failed to insert session with id: {}, user_id: {}, "
                  "title: {}, ms: {}",
                  id,
                  user_id,
                  title,
                  ms);

        reactor->Finish(status_t::OK);
        return reactor;
    }

    auto session = resp->mutable_session();
    session->set_id(id);
    session->set_user_id(user_id);
    session->set_title(title);
    session->set_timestamp(
        hj::date_time::format(hj::date_time::from_ms_since_epoch(ms),
                              TIME_FORMAT));

    LOG_DEBUG("Session created without content or model, return directly");
    resp->set_error_code(OK);

    reactor->Finish(status_t::OK);
    return reactor;
}

reactor_t *api_handler::ModifySessionTitle(
    ctx_t                                        *ctx,
    const ::GrpcLibraryV1::ModifySessionTitleReq *req,
    ::GrpcLibraryV1::ModifySessionTitleResp      *resp)
{
    int64_t     id      = req->id();
    int64_t     user_id = req->user_id();
    std::string auth    = req->auth();
    std::string title   = req->title();
    auto       *reactor = ctx->DefaultReactor();

    resp->set_error_code(ERR_FAIL);
    resp->set_id(id);
    resp->set_title(title);
    LOG_DEBUG(
        "Received ModifySessionTitle request. id: {}, user_id: {}, title: {}",
        id,
        user_id,
        title);

    if(db_mgr::instance().exec(SQL_UPDATE_SESSION_TITLE_BY_ID,
                               title.c_str(),
                               id)
       != OK)
    {
        resp->set_error_code(ERR_SQLITE_EXEC_FAIL);
        LOG_ERROR("Failed to update session for id: {}, title: {}", id, title);

        reactor->Finish(status_t::OK);
        return reactor;
    }

    resp->set_error_code(OK);

    reactor->Finish(status_t::OK);
    return reactor;
}

reactor_t *api_handler::DelSession(ctx_t                                *ctx,
                                   const ::GrpcLibraryV1::DelSessionReq *req,
                                   ::GrpcLibraryV1::DelSessionResp      *resp)
{
    auto        ids     = req->ids();
    int64_t     user_id = req->user_id();
    std::string auth    = req->auth();
    auto       *reactor = ctx->DefaultReactor();
    resp->set_error_code(ERR_FAIL);
    LOG_DEBUG("Received DelSession request. ids.size(): {}, user_id: {}",
              ids.size(),
              user_id);

    for(auto id : ids)
    {
        if(db_mgr::instance().exec(SQL_DELETE_SESSION_BY_ID, id) != OK)
        {
            resp->set_error_code(ERR_SQLITE_EXEC_FAIL);
            LOG_ERROR("Failed to delete session for id: {}", id);

            reactor->Finish(status_t::OK);
            return reactor;
        }

        // delete all relative message
        if(db_mgr::instance().exec(SQL_DELETE_MESSAGE_BY_SESSION_ID, id) != OK)
        {
            resp->set_error_code(ERR_SQLITE_EXEC_FAIL);
            LOG_ERROR("Failed to delete messages for session id: {}", id);

            reactor->Finish(status_t::OK);
            return reactor;
        }

        resp->add_ids(id);
    }

    resp->set_error_code(OK);

    reactor->Finish(status_t::OK);
    return reactor;
}

reactor_t *
api_handler::GetPluginInfo(ctx_t                                   *ctx,
                           const ::GrpcLibraryV1::GetPluginInfoReq *req,
                           ::GrpcLibraryV1::GetPluginInfoResp      *resp)
{
    // NOTE: wo should use ORM or prepared statement to avoid SQL injection,
    // but for simplicity, we use string concatenation here.
    std::string hash      = req->hash();
    std::string publisher = req->publisher();
    int         limit     = req->limit();
    limit                 = (limit < 0 || limit > 50) ? 50 : limit;
    auto *reactor         = ctx->DefaultReactor();
    resp->set_error_code(ERR_FAIL);
    LOG_DEBUG(
        "Received GetPluginInfo request. hash: {}, publisher: {}, limit: {}",
        hash,
        publisher,
        limit);

    // std::string sql = SQL_SELECT_PLUGIN_INFO;
    // if(!hash.empty())
    //     sql += hj::sqlite::mprintf(" AND hash = '%s'", hash.c_str());

    // if(!publisher.empty())
    //     sql += hj::sqlite::mprintf(" AND publisher = '%s'", publisher.c_str());

    // sql += hj::sqlite::mprintf(" LIMIT %d;", limit);

    // LOG_DEBUG("{}", sql);
    // db_mgr::query_ret rows;
    // if(db_mgr::instance().query(rows, DB_SQLITE, sql) != OK)
    // {
    //     resp->set_error_code(ERR_SQLITE_EXEC_FAIL);
    //     LOG_ERROR("Failed to query plugin info for sql: {}", sql);

    //     reactor->Finish(status_t::OK);
    //     return reactor;
    // }
    // for(const auto row : rows)
    // {
    //     auto item = resp->add_plugins();
    //     item->set_hash(row[0]);
    //     item->set_platform(row[1].empty() ? 0 : std::stoi(row[1]));
    //     item->set_name(row[2]);
    //     item->set_desc(row[3]);
    //     item->set_publisher(row[4]);
    //     item->set_version(row[5]);
    //     long long ms = row[6].empty() ? 0 : std::stoll(row[6]);
    //     item->set_timestamp(
    //         hj::date_time::format(hj::date_time::from_ms_since_epoch(ms),
    //                               TIME_FORMAT));

    //     LOG_DEBUG("GetPluginInfo hash: {}, platform: {}, name: {}, desc: {}, "
    //               "publisher: {}, version: {}, timestamp: {}",
    //               item->hash(),
    //               item->platform(),
    //               item->name(),
    //               item->desc(),
    //               item->publisher(),
    //               item->version(),
    //               item->timestamp());
    // }

    db_mgr::query_ret rows;
    if(db_mgr::instance().query(rows, SQL_SELECT_PLUGIN_INFO, limit) != OK)
    {
        resp->set_error_code(ERR_SQLITE_EXEC_FAIL);
        LOG_ERROR("Failed to query plugin info");

        reactor->Finish(status_t::OK);
        return reactor;
    }
    for(int n_row = 0; n_row < rows.rows(); ++n_row)
    {
        auto item = resp->add_plugins();
        item->set_hash(rows.get_or<std::string>(n_row, 0, ""));
        item->set_platform(rows.get_or<int64_t>(n_row, 1, 0));
        item->set_name(rows.get_or<std::string>(n_row, 2, ""));
        item->set_desc(rows.get_or<std::string>(n_row, 3, ""));
        item->set_publisher(rows.get_or<std::string>(n_row, 4, ""));
        item->set_version(rows.get_or<std::string>(n_row, 5, ""));
        long long ms = rows.get_or<int64_t>(n_row, 6, 0);
        item->set_timestamp(
            hj::date_time::format(hj::date_time::from_ms_since_epoch(ms),
                                  TIME_FORMAT));

        LOG_DEBUG("GetPluginInfo hash: {}, platform: {}, name: {}, desc: {}, "
                  "publisher: {}, version: {}, timestamp: {}",
                  item->hash(),
                  item->platform(),
                  item->name(),
                  item->desc(),
                  item->publisher(),
                  item->version(),
                  item->timestamp());
    }

    resp->set_error_code(OK);

    reactor->Finish(status_t::OK);
    return reactor;
}

reactor_t *api_handler::Download(ctx_t                              *ctx,
                                 const ::GrpcLibraryV1::DownloadReq *req,
                                 ::GrpcLibraryV1::DownloadResp      *resp)
{
    std::string hash    = req->hash();
    int64_t     user_id = req->user_id();
    std::string auth    = req->auth();
    auto       *reactor = ctx->DefaultReactor();
    resp->set_error_code(ERR_FAIL);
    resp->set_hash(hash);
    LOG_DEBUG("Received Download request. hash: {}, user_id: {}",
              hash,
              user_id);

    db_mgr::query_ret rows;
    if(db_mgr::instance().query(rows, SQL_SELECT_FILE_BY_HASH, hash) != OK)
    {
        LOG_ERROR("Failed to query file for hash: {}", hash);

        reactor->Finish(status_t::OK);
        return reactor;
    }
    for(int n_row = 0; n_row < rows.rows(); ++n_row)
    {
        resp->set_addr(rows.get_or<std::string>(n_row, 0, ""));
        resp->set_size_kb(rows.get_or<long long>(n_row, 2, 0));
        break;
    }

    resp->set_error_code(OK);

    reactor->Finish(status_t::OK);
    return reactor;
}

reactor_t *api_handler::Upload(ctx_t                            *ctx,
                               const ::GrpcLibraryV1::UploadReq *req,
                               ::GrpcLibraryV1::UploadResp      *resp)
{
    std::string hash    = req->hash();
    int64_t     user_id = req->user_id();
    std::string auth    = req->auth();
    std::string addr    = req->addr();
    int64_t     size_kb = req->size_kb();
    auto       *reactor = ctx->DefaultReactor();
    resp->set_error_code(ERR_FAIL);
    resp->set_hash(hash);
    LOG_DEBUG("Received Upload request. hash: {}, user_id: {}", hash, user_id);

    db_mgr::query_ret rows;
    if(db_mgr::instance().exec(SQL_INSERT_FILE, hash, addr, user_id, size_kb)
       != OK)
    {
        resp->set_error_code(ERR_SQLITE_EXEC_FAIL);
        LOG_ERROR("Failed to insert file for hash: {}, addr: {}, user_id: {}, "
                  "size_kb: {}",
                  hash,
                  addr,
                  user_id,
                  size_kb);

        reactor->Finish(status_t::OK);
        return reactor;
    }

    resp->set_error_code(OK);
    resp->set_hash(hash);
    LOG_DEBUG("send upload resp with error_code:{}, hash:{}",
              resp->error_code(),
              resp->hash());

    reactor->Finish(status_t::OK);
    return reactor;
}

grpc::ServerBidiReactor<GrpcLibraryV1::EmbeddingReq,
                        GrpcLibraryV1::EmbeddingResp> *
api_handler::Embedding(grpc::CallbackServerContext *context)
{
    LOG_INFO("Received Embedding bidirectional stream request from peer: {}",
             context->peer());

    return new EmbeddingReactor(context);
}

reactor_t *
api_handler::StopEmbedding(ctx_t                                   *ctx,
                           const ::GrpcLibraryV1::StopEmbeddingReq *req,
                           ::GrpcLibraryV1::StopEmbeddingResp      *resp)
{
    auto task_id = req->task_id();
    auto user_id = req->user_id();
    auto auth    = req->auth();
    LOG_DEBUG("Received StopEmbedding request. task_id: {}, user_id: {}",
              task_id,
              user_id);

    auto *reactor = ctx->DefaultReactor();
    resp->set_task_id(task_id);

    bool stopped = embedding_reactor_mgr::instance().stop_embedding(task_id);
    if(!stopped)
    {
        LOG_WARN("No active EmbeddingReactor found for task_id: {}", task_id);
        resp->set_error_code(ERR_STOP_FAIL);
    } else
    {
        LOG_DEBUG("Successfully stopped EmbeddingReactor for task_id: {}",
                  task_id);
        resp->set_error_code(OK);
    }

    LOG_DEBUG("StopEmbedding request processed for task_id: {}, user_id: {}",
              task_id,
              user_id);

    reactor->Finish(status_t::OK);
    return reactor;
}

reactor_t *api_handler::Publish(ctx_t                             *ctx,
                                const ::GrpcLibraryV1::PublishReq *req,
                                ::GrpcLibraryV1::PublishResp      *resp)
{
    int64_t     user_id = req->user_id();
    std::string auth    = req->auth();

    std::vector<std::string> msgs;
    msgs.reserve(req->msgs_size());
    for(const auto &msg : req->msgs())
        msgs.push_back(msg);

    auto *reactor = ctx->DefaultReactor();
    resp->set_error_code(ERR_FAIL);
    LOG_DEBUG("Received Publish request. msgs.size(): {}, user_id: {}",
              msgs.size(),
              user_id);

    for(int i = 0; i < msgs.size(); ++i)
    {
        auto &msg = msgs[i];
        mq::instance().pub(msg);
        LOG_DEBUG("Publish msg[{}]: {}", i, msg);
    }

    resp->set_error_code(OK);
    reactor->Finish(status_t::OK);
    return reactor;
}

grpc::ServerWriteReactor<::GrpcLibraryV1::PubMessage> *
api_handler::Subscribe(grpc::CallbackServerContext         *ctx,
                       const ::GrpcLibraryV1::SubscribeReq *req)
{
    auto user_id = req->user_id();
    auto auth    = req->auth();

    std::vector<std::string> topics;
    topics.reserve(req->topics_size());
    for(const auto &topic : req->topics())
        topics.push_back(topic);

    auto suber = subscribe_reactor_mgr::instance().get_active_suber(user_id);
    if(!suber)
        suber = new SubscribeReactor(ctx, req);

    if(!suber->Sub(topics))
    {
        LOG_ERROR("Failed to subscribe topics for user_id: {}, topics: {}",
                  user_id,
                  hj::format("{}", topics));
        delete suber;
        return nullptr;
    }

    return suber;
}

reactor_t *api_handler::UnSubscribe(ctx_t                                 *ctx,
                                    const ::GrpcLibraryV1::UnSubscribeReq *req,
                                    ::GrpcLibraryV1::UnSubscribeResp      *resp)
{
    int64_t user_id = req->user_id();
    auto    auth    = req->auth();

    auto *reactor = ctx->DefaultReactor();
    resp->set_error_code(ERR_FAIL);
    LOG_DEBUG("Received UnSubscribe request. user_id: {}", user_id);

    auto suber = subscribe_reactor_mgr::instance().get_active_suber(user_id);
    if(!suber)
    {
        LOG_ERROR("No active suber found for user_id: {}", user_id);
        resp->set_error_code(SUB_ERR_NO_ACTIVE_SUBER);

        reactor->Finish(status_t::OK);
        return reactor;
    }

    std::vector<std::string> topics;
    topics.reserve(req->topics_size());
    for(const auto &topic : req->topics())
        topics.push_back(topic);

    if(!suber->UnSub(topics))
    {
        LOG_ERROR("Failed to unsubscribe topics for user_id: {}, topics: {}",
                  user_id,
                  hj::format("{}", topics));
        resp->set_error_code(SUB_ERR_UNSUB_FAIL);

        reactor->Finish(status_t::OK);
        return reactor;
    }

    resp->set_error_code(OK);
    for(auto topic : topics)
        resp->add_topics(topic);

    reactor->Finish(status_t::OK);
    return reactor;
}