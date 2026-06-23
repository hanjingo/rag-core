#include "server.h"

#include <hj/log/log.hpp>
#include <hj/encoding/fmt.hpp>
#include <hj/time/date_time.hpp>
#include <hj/algo/uuid.hpp>

#include "conf.h"
#include "db_mgr.h"
#include "auth.h"
#include "account_mgr.h"
#include "llm.h"

status_t api_handler::Login(ctx_t                         *ctx,
                            const ::GrpcLibrary::LoginReq *req,
                            ::GrpcLibrary::LoginResp      *resp)
{
    std::string account          = req->account();
    std::string encrypted_passwd = req->passwd();
    int64_t     user_id          = -1;
    int         privilege        = -1;
    resp->set_error_code(ERR_FAIL);
    LOG_DEBUG("Received Login request. account: {}, passwd: {}",
              account,
              encrypted_passwd);

    std::string sql =
        hj::fmt(SQL_SELECT_USER_BY_USERNAME_PASSWD, account, encrypted_passwd)
        + " LIMIT 1;";
    LOG_DEBUG("{}", sql);
    db_mgr::query_ret rows;
    if(db_mgr::instance().query(rows, DB_SQLITE, sql) != OK)
    {
        LOG_ERROR("Failed to authenticate account: {}, sql: {}", account, sql);
        resp->set_error_code(ACCOUNT_INVALID);
        return status_t::OK;
    }
    for(const auto row : rows)
    {
        user_id   = std::stoll(row[0]);
        privilege = std::stoi(row[3]);
        break;
    }

    hj::license::token_t token;
    if(OK
       != issuer::instance().issue(token,
                                   std::to_string(user_id),
                                   conf::instance().issuer_leeway(),
                                   {}))
    {
        resp->set_error_code(AUTH_ERR_ISSUE_FAIL);
        LOG_ERROR("Failed to issue license for account: {}", account);
        return status_t::OK;
    }

    resp->set_error_code(OK);
    resp->set_user_id(user_id);
    resp->set_privilege(privilege);
    resp->set_auth(token);
    resp->set_account(account);
    resp->set_last_login_time(
        hj::date_time::format(hj::date_time::now(), "%Y-%m-%d %H:%M:%S"));
    return status_t::OK;
}

status_t api_handler::Logout(ctx_t                          *ctx,
                             const ::GrpcLibrary::LogoutReq *req,
                             ::GrpcLibrary::LogoutResp      *resp)
{
    int64_t     user_id = -1;
    std::string auth;
    resp->set_error_code(ERR_FAIL);
    LOG_DEBUG("Received Logout request. user_id: {}, auth: {}", user_id, auth);

    bool success =
        (verifier::instance().verify(auth, std::to_string(user_id), {}) == OK);
    if(!success)
    {
        LOG_ERROR("Failed to verify auth for user_id: {}, auth: {}",
                  user_id,
                  auth);
        resp->set_error_code(ACCOUNT_INVALID);
        return status_t::OK;
    }

    resp->set_error_code(OK);
    return status_t::OK;
}

status_t api_handler::RegAccount(ctx_t                              *ctx,
                                 const ::GrpcLibrary::RegAccountReq *req,
                                 ::GrpcLibrary::RegAccountResp      *resp)
{
    std::string account          = req->account();
    std::string encrypted_passwd = req->passwd();
    resp->set_error_code(ERR_FAIL);
    LOG_DEBUG("Received RegAccount request. account: {}, passwd: {}",
              account,
              encrypted_passwd);

    const uint64_t id        = hj::uuid::gen_u64();
    const int      privilege = 0; // default privilege for new account
    auto           sql =
        hj::fmt(SQL_INSERT_USER, id, account, encrypted_passwd, privilege);
    LOG_DEBUG("{}", sql);
    if(db_mgr::instance().exec(DB_SQLITE, sql) != OK)
    {
        resp->set_error_code(ERR_SQLITE_EXEC_FAIL);
        LOG_ERROR("Failed to insert user with sql: {}", sql);
        return status_t::OK;
    }

    resp->set_user_id(id);
    resp->set_error_code(OK);
    return status_t::OK;
}

status_t api_handler::Query(ctx_t                         *ctx,
                            const ::GrpcLibrary::QueryReq *req,
                            ::GrpcLibrary::QueryResp      *resp)
{
    int64_t     session_id = req->id();
    int64_t     user_id    = req->user_id();
    std::string auth       = req->auth();
    std::string content    = req->content();
    std::string model      = req->model();
    resp->set_error_code(ERR_FAIL);
    resp->set_id(session_id);
    LOG_DEBUG("Query request session_id: {}, user_id: {}, auth: {}, content: "
              "{}, model: {}",
              session_id,
              user_id,
              auth,
              content,
              model);

    // TODO check privilege && token count

    // save query record
    auto msg_id = hj::uuid::gen_u64();
    auto now = hj::date_time::format(hj::date_time::now(), "%Y-%m-%d %H:%M:%S");
    auto sql = hj::fmt(SQL_INSERT_MESSAGE,
                       msg_id,
                       session_id,
                       ROLE_USER,
                       content,
                       NONE_MSG_ID,
                       now);
    if(db_mgr::instance().exec(DB_SQLITE, sql) != OK)
    {
        resp->set_error_code(ERR_SQLITE_EXEC_FAIL);
        LOG_ERROR("Failed to insert message for sql: {}", sql);
        return status_t::OK;
    }

    // answer query
    auto tokens = llm_mgr::instance().tokenize(model, content, true, true);
    auto params = llm_mgr::instance().create_ctx_params(
        conf::instance().llm_ctx_window_sz());
    auto ec = llm_mgr::instance()
                  .loop_query(model, tokens, params, [&](std::string &output) {
                      LOG_DEBUG("llm generated output: {}", output);
                      if(output.empty())
                          return true;

                      resp->set_content(output);
                      return false;
                  });

    // save answer record
    msg_id = hj::uuid::gen_u64();
    now    = hj::date_time::format(hj::date_time::now(), "%Y-%m-%d %H:%M:%S");
    sql    = hj::fmt(SQL_INSERT_MESSAGE,
                     msg_id,
                     session_id,
                     ROLE_ASSISTANT,
                     resp->content(),
                     NONE_MSG_ID,
                     now);
    if(db_mgr::instance().exec(DB_SQLITE, sql) != OK)
    {
        resp->set_error_code(ERR_SQLITE_EXEC_FAIL);
        LOG_ERROR("Failed to insert message for sql: {}", sql);
        return status_t::OK;
    }

    resp->set_error_code(ec);
    return status_t::OK;
}

status_t
api_handler::GetMessageInfo(ctx_t                                  *ctx,
                            const ::GrpcLibrary::GetMessageInfoReq *req,
                            ::GrpcLibrary::GetMessageInfoResp      *resp)
{
    int64_t     id         = req->id();
    int64_t     session_id = req->session_id();
    int32_t     limit      = req->limit();
    int64_t     user_id    = req->user_id();
    std::string auth       = req->auth();

    if(limit < 0 || limit > conf::instance().sqlite_msg_limit())
        limit = conf::instance().sqlite_msg_limit();
    resp->set_error_code(ERR_FAIL);
    LOG_DEBUG(
        "Received GetMessage request. id: {}, session_id: {}, user_id: {}",
        ", auth: {}, limit: {}",
        id,
        session_id,
        user_id,
        auth,
        limit);

    std::string sql;
    if(id != -1)
        sql = hj::fmt(SQL_SELECT_MESSAGE_BY_ID, id);
    else
        sql = hj::fmt(SQL_SELECT_MESSAGE_BY_SESSION_ID, session_id);
    sql += hj::fmt(" LIMIT {};", std::to_string(limit));

    LOG_DEBUG("{}", sql);
    db_mgr::query_ret rows;
    if(db_mgr::instance().query(rows, DB_SQLITE, sql) != OK)
    {
        LOG_ERROR("Failed to query message for sql: {}", sql);
        resp->set_error_code(ERR_SQLITE_EXEC_FAIL);
        return status_t::OK;
    }
    for(const auto row : rows)
    {
        LOG_DEBUG("I AM HERE1");
        auto item = resp->add_messages();
        item->set_id(row[0].empty() ? -1 : std::stoll(row[0]));
        item->set_session_id(row[1].empty() ? -1 : std::stoll(row[1]));
        item->set_role(row[2]);
        item->set_content(row[3]);
        item->set_prev_message_id(row[4].empty() ? -1 : std::stoll(row[4]));
        item->set_timestamp(row[5]);

        LOG_DEBUG("GetMessage id: {}, session_id: {}, role: {}, content: {}, "
                  "prev_message_id: {}, timestamp: {}",
                  item->id(),
                  item->session_id(),
                  item->role(),
                  item->content(),
                  item->prev_message_id(),
                  item->timestamp());
    }

    resp->set_error_code(OK);
    return status_t::OK;
}

status_t api_handler::GetSession(ctx_t                              *ctx,
                                 const ::GrpcLibrary::GetSessionReq *req,
                                 ::GrpcLibrary::GetSessionResp      *resp)
{
    int64_t     id      = req->id();
    int64_t     user_id = req->user_id();
    std::string auth    = req->auth();
    int         limit   = req->limit();
    limit               = (limit < 0 || limit > 100) ? 100 : limit;
    resp->set_error_code(ERR_FAIL);
    LOG_DEBUG(
        "Received GetSession request. id: {}, user_id: {}, auth: {}, limit: {}",
        id,
        user_id,
        auth,
        limit);

    std::string sql;
    if(id > 0)
        sql = hj::fmt(SQL_SELECT_SESSION_BY_ID, id) + " LIMIT 1;";
    else
        sql = hj::fmt(SQL_SELECT_SESSION_BY_USER_ID, user_id)
              + hj::fmt(" LIMIT {};", std::to_string(limit));

    LOG_DEBUG("{}", sql);
    db_mgr::query_ret rows;
    if(db_mgr::instance().query(rows, DB_SQLITE, sql) != OK)
    {
        LOG_ERROR("Failed to query history for sql: {}", sql);
        resp->set_error_code(ERR_SQLITE_EXEC_FAIL);
        return status_t::OK;
    }
    for(const auto row : rows)
    {
        auto item = resp->add_sessions();
        item->set_id(std::stoll(row[0]));
        item->set_user_id(std::stoll(row[1]));
        item->set_title(row[2]);
        item->set_timestamp(row[3]);

        LOG_DEBUG("GetSession id: {}, user_id: {}, title: {}, timestamp: {}",
                  item->id(),
                  item->user_id(),
                  item->title(),
                  item->timestamp());
    }

    resp->set_error_code(OK);
    return status_t::OK;
}

status_t api_handler::NewSession(ctx_t                              *ctx,
                                 const ::GrpcLibrary::NewSessionReq *req,
                                 ::GrpcLibrary::NewSessionResp      *resp)
{
    resp->set_error_code(ERR_FAIL);
    int64_t     user_id = req->user_id();
    std::string auth    = req->auth();
    std::string title   = req->title();
    std::string content = req->content();
    std::string model   = req->model();
    std::string answer;
    std::string timestamp =
        hj::date_time::format(hj::date_time::now(), "%Y-%m-%d %H:%M:%S");
    LOG_DEBUG("Received NewSession request. user_id: {}, auth: {}, title: {}, "
              "content: {}, model: {}",
              user_id,
              auth,
              title,
              content,
              model);

    int64_t id  = hj::uuid::gen_u64();
    auto    sql = hj::fmt(SQL_INSERT_SESSION, id, user_id, title, timestamp);
    LOG_DEBUG("{}", sql);
    if(db_mgr::instance().exec(DB_SQLITE, sql) != OK)
    {
        resp->set_error_code(ERR_SQLITE_EXEC_FAIL);
        LOG_ERROR("Failed to insert session for sql: {}", sql);
        return status_t::OK;
    }

    auto session = resp->mutable_session();
    session->set_id(id);
    session->set_user_id(user_id);
    session->set_title(title);
    session->set_timestamp(timestamp);

    LOG_DEBUG("Session created without content or model, return directly");
    resp->set_error_code(OK);
    return status_t::OK;
}

status_t
api_handler::ModifySessionTitle(ctx_t                                      *ctx,
                                const ::GrpcLibrary::ModifySessionTitleReq *req,
                                ::GrpcLibrary::ModifySessionTitleResp *resp)
{
    int64_t     id      = req->id();
    int64_t     user_id = req->user_id();
    std::string auth    = req->auth();
    std::string title   = req->title();
    resp->set_error_code(ERR_FAIL);
    resp->set_id(id);
    resp->set_title(title);
    LOG_DEBUG(
        "Received ModifySessionTitle request. id: {}, user_id: {}, auth: {}, "
        "title: {}",
        id,
        user_id,
        auth,
        title);

    // TODO check privilege

    auto sql = hj::fmt(SQL_UPDATE_SESSION_TITLE_BY_ID, title, id);
    LOG_DEBUG("{}", sql);
    if(db_mgr::instance().exec(DB_SQLITE, sql) != OK)
    {
        resp->set_error_code(ERR_SQLITE_EXEC_FAIL);
        LOG_ERROR("Failed to update session for sql: {}", sql);
        return status_t::OK;
    }

    resp->set_error_code(OK);
    return status_t::OK;
}

status_t api_handler::DelSession(ctx_t                              *ctx,
                                 const ::GrpcLibrary::DelSessionReq *req,
                                 ::GrpcLibrary::DelSessionResp      *resp)
{
    auto        ids     = req->ids();
    int64_t     user_id = req->user_id();
    std::string auth    = req->auth();
    resp->set_error_code(ERR_FAIL);
    LOG_DEBUG(
        "Received DelSession request. ids.size(): {}, user_id: {}, auth: {}",
        ids.size(),
        user_id,
        auth);

    // TODO check privilege

    for(auto id : ids)
    {
        auto sql = hj::fmt(SQL_DELETE_SESSION_BY_ID, id);
        LOG_DEBUG("{}", sql);
        if(db_mgr::instance().exec(DB_SQLITE, sql) != OK)
        {
            resp->set_error_code(ERR_SQLITE_EXEC_FAIL);
            LOG_ERROR("Failed to delete session for sql: {}", sql);
            return status_t::OK;
        }

        // delete all relative message
        sql = hj::fmt(SQL_DELETE_MESSAGE_BY_SESSION_ID, id);
        LOG_DEBUG("{}", sql);
        if(db_mgr::instance().exec(DB_SQLITE, sql) != OK)
        {
            resp->set_error_code(ERR_SQLITE_EXEC_FAIL);
            LOG_ERROR("Failed to delete messages for sql: {}", sql);
            return status_t::OK;
        }

        resp->add_ids(id);
    }

    resp->set_error_code(OK);
    return status_t::OK;
}

status_t api_handler::GetModelInfo(ctx_t                                *ctx,
                                   const ::GrpcLibrary::GetModelInfoReq *req,
                                   ::GrpcLibrary::GetModelInfoResp      *resp)
{
    resp->set_error_code(ERR_FAIL);
    std::string hash    = req->hash();
    int64_t     user_id = req->user_id();
    std::string auth    = req->auth();
    int         limit   = req->limit();
    limit               = (limit < 0 || limit > 100) ? 100 : limit;
    LOG_DEBUG("Received GetModelInfo request. hash: {}, user_id: {}, auth: {}, "
              "limit: {}",
              hash,
              user_id,
              auth,
              limit);

    std::string sql;
    if(hash.empty())
        sql = SQL_SELECT_MODEL + hj::fmt(" LIMIT {};", std::to_string(limit));
    else
        sql = hj::fmt(SQL_SELECT_MODEL_BY_HASH, hash) + " LIMIT 1;";

    LOG_DEBUG("{}", sql);
    db_mgr::query_ret rows;
    if(db_mgr::instance().query(rows, DB_SQLITE, sql) != OK)
    {
        resp->set_error_code(ERR_SQLITE_EXEC_FAIL);
        LOG_ERROR("Failed to query model info for sql: {}", sql);
        return status_t::OK;
    }
    for(const auto row : rows)
    {
        auto item = resp->add_models();
        item->set_hash(row[0]);
        item->set_name(row[1]);
        item->set_publisher(row[2]);
        item->set_timestamp(row[3]);
        item->set_addr(row[4]);
        item->set_capabilities(row[5]);
        item->set_context_size(std::stoll(row[6]));
        item->set_cost(std::stod(row[7]));
    }

    resp->set_error_code(OK);
    return status_t::OK;
}

status_t api_handler::NewModelInfo(ctx_t                                *ctx,
                                   const ::GrpcLibrary::NewModelInfoReq *req,
                                   ::GrpcLibrary::NewModelInfoResp      *resp)
{
    int64_t     user_id = req->user_id();
    std::string auth    = req->auth();
    auto        models  = req->models();
    resp->set_error_code(ERR_FAIL);
    LOG_DEBUG("Received NewModelInfo request. user_id: {}, auth: {}, models "
              "count: {}",
              user_id,
              auth,
              models.size());

    // TODO check privilege

    for(const auto &model : models)
    {
        std::string hash         = model.hash();
        std::string name         = model.name();
        std::string publisher    = model.publisher();
        std::string timestamp    = model.timestamp();
        std::string addr         = model.addr();
        std::string capabilities = model.capabilities();
        int64_t     context_size = model.context_size();
        int         cost         = model.cost();
        auto        sql          = hj::fmt(SQL_INSERT_MODEL,
                                           hash,
                                           name,
                                           publisher,
                                           timestamp,
                                           addr,
                                           capabilities,
                                           context_size,
                                           cost);
        LOG_DEBUG("{}", sql);
        if(db_mgr::instance().exec(DB_SQLITE, sql) != OK)
        {
            LOG_ERROR("Failed to insert model for sql: {}", sql);
            continue;
        }

        resp->add_hashs(hash);
    }

    resp->set_error_code(OK);
    return status_t::OK;
}

status_t api_handler::GetSkillInfo(ctx_t                                *ctx,
                                   const ::GrpcLibrary::GetSkillInfoReq *req,
                                   ::GrpcLibrary::GetSkillInfoResp      *resp)
{
    std::string hash  = req->hash();
    int         limit = req->limit();
    limit             = limit < 0 || limit > 50 ? 50 : limit;
    resp->set_error_code(ERR_FAIL);
    LOG_DEBUG("Received GetSkillInfo request. hash: {}, limit: {}",
              hash,
              limit);

    std::string sql;
    if(hash.empty())
        sql = SQL_SELECT_SKILL_INFO
              + hj::fmt(" LIMIT {};", std::to_string(limit));
    else
        sql = hj::fmt(SQL_SELECT_SKILL_INFO_BY_HASH, hash) + " LIMIT 1;";

    LOG_DEBUG("{}", sql);
    db_mgr::query_ret rows;
    if(db_mgr::instance().query(rows, DB_SQLITE, sql) != OK)
    {
        resp->set_error_code(ERR_SQLITE_EXEC_FAIL);
        LOG_ERROR("Failed to query skill info for sql: {}", sql);
        return status_t::OK;
    }
    for(const auto row : rows)
    {
        auto item = resp->add_skills();
        item->set_hash(row[0]);
        item->set_platform(std::stoi(row[1]));
        item->set_name(row[2]);
        item->set_desc(row[3]);
        item->set_publisher(row[4]);
        item->set_version(row[5]);
        item->set_timestamp(row[6]);

        LOG_DEBUG("GetSkillInfo hash: {}, platform: {}, name: {}, desc: {}",
                  ", publisher: {}, version: {}, timestamp: {}",
                  item->hash(),
                  item->platform(),
                  item->name(),
                  item->desc(),
                  item->publisher(),
                  item->version(),
                  item->timestamp());
    }

    resp->set_error_code(OK);
    return status_t::OK;
}

status_t api_handler::Download(ctx_t                            *ctx,
                               const ::GrpcLibrary::DownloadReq *req,
                               ::GrpcLibrary::DownloadResp      *resp)
{
    std::string hash    = req->hash();
    int64_t     user_id = req->user_id();
    std::string auth    = req->auth();
    resp->set_error_code(ERR_FAIL);
    resp->set_hash(hash);
    LOG_DEBUG("Received Download request. hash: {}, user_id: {}, auth: {}",
              hash,
              user_id,
              auth);

    // TODO check privilege

    auto sql = hj::fmt(SQL_SELECT_FILE_BY_HASH, hash) + " LIMIT 1;";
    LOG_DEBUG("{}", sql);
    db_mgr::query_ret rows;
    if(db_mgr::instance().query(rows, DB_SQLITE, sql) != OK)
    {
        LOG_ERROR("Failed to query history for sql: {}", sql);
        return status_t::OK;
    }
    for(const auto row : rows)
    {
        resp->set_addr(row[0]);
        resp->set_size_kb(std::stoll(row[2]));
        break;
    }

    resp->set_error_code(OK);
    return status_t::OK;
}