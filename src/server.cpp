#include "server.h"

#include <hj/log/log.hpp>
#include <hj/encoding/fmt.hpp>
#include <hj/time/date_time.hpp>

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
    int         user_id          = -1;
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
    if(db_mgr::instance().query(rows, "sqlite", sql) != OK)
    {
        LOG_ERROR("Failed to authenticate account: {}, sql: {}", account, sql);
        resp->set_error_code(ACCOUNT_INVALID);
        return status_t::OK;
    }
    for(const auto row : rows)
    {
        user_id   = std::stoi(row[0]);
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
    int         user_id = -1;
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
    std::string account;
    std::string encrypted_passwd;
    bool        success = false;
    resp->set_error_code(ERR_FAIL);
    LOG_DEBUG("Received RegAccount request. account: {}, passwd: {}",
              account,
              encrypted_passwd);

    auto sql = hj::fmt(SQL_INSERT_USER, account, encrypted_passwd);
    LOG_DEBUG("{}", sql);
    if(db_mgr::instance().exec("sqlite", sql) != OK)
    {
        resp->set_error_code(ERR_SQLITE_EXEC_FAIL);
        LOG_ERROR("Failed to insert user with sql: {}", sql);
        return status_t::OK;
    }

    auto id = db_mgr::instance().last_insert_id("sqlite", "user");
    resp->set_user_id(id);
    resp->set_error_code(OK);
    return status_t::OK;
}

status_t api_handler::Query(ctx_t                         *ctx,
                            const ::GrpcLibrary::QueryReq *req,
                            ::GrpcLibrary::QueryResp      *resp)
{
    int64_t     id      = req->id();
    int32_t     user_id = req->user_id();
    std::string auth    = req->auth();
    std::string content = req->content();
    std::string model   = req->model();
    resp->set_error_code(ERR_FAIL);
    resp->set_id(id);
    LOG_DEBUG(
        "Query request id: {}, user_id: {}, auth: {}, content: {}, model: {}",
        id,
        user_id,
        auth,
        content,
        model);

    // TODO check privilege && token count

    LOG_DEBUG("Received Query request. id: {}, user_id: {}, auth: {}, content: "
              "{}, model: {}",
              id,
              user_id,
              auth,
              content,
              model);
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
    resp->set_error_code(ec);
    return status_t::OK;
}

status_t api_handler::GetSession(ctx_t                              *ctx,
                                 const ::GrpcLibrary::GetSessionReq *req,
                                 ::GrpcLibrary::GetSessionResp      *resp)
{
    int64_t     id      = req->id();
    int         user_id = req->user_id();
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
    if(db_mgr::instance().query(rows, "sqlite", sql) != OK)
    {
        LOG_ERROR("Failed to query history for sql: {}", sql);
        resp->set_error_code(ERR_SQLITE_EXEC_FAIL);
        return status_t::OK;
    }
    for(const auto row : rows)
    {
        auto item = resp->add_sessions();
        item->set_id(std::stoll(row[0]));
        item->set_user_id(std::stoi(row[1]));
        item->set_title(row[2]);
        item->set_content(row[3]);
        item->set_timestamp(row[4]);

        LOG_DEBUG("GetSession id: {}, user_id: {}, title: {}, content: {}, "
                  "timestamp: {}",
                  item->id(),
                  item->user_id(),
                  item->title(),
                  item->content(),
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
    int         user_id = req->user_id();
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

    auto sql = hj::fmt(SQL_INSERT_SESSION, user_id, title, content, timestamp);
    LOG_DEBUG("{}", sql);
    if(db_mgr::instance().exec("sqlite", sql) != OK)
    {
        resp->set_error_code(ERR_SQLITE_EXEC_FAIL);
        LOG_ERROR("Failed to insert session for sql: {}", sql);
        return status_t::OK;
    }

    auto id      = db_mgr::instance().last_insert_id("sqlite", "session");
    auto session = resp->mutable_session();
    session->set_id(id);
    session->set_user_id(user_id);
    session->set_title(title);
    session->set_timestamp(timestamp);

    // if content not null, ask llm to generate answer and update session
    if(content.size() > 0 && model.size() > 0)
    {
        LOG_DEBUG("Received NewSession request with content not null, ask llm");
        auto tokens = llm_mgr::instance().tokenize(model, content, true, true);
        auto params = llm_mgr::instance().create_ctx_params(
            conf::instance().llm_ctx_window_sz());

        auto ec =
            llm_mgr::instance().loop_query(model,
                                           tokens,
                                           params,
                                           [&](std::string &pieces) -> bool {
                                               // TODO check tokens' enough or not!

                                               answer += pieces;
                                               return LLM_CONTINUE;
                                           });
        session->set_content(answer);
        resp->set_error_code(ec);
        LOG_DEBUG("Session generated ec:{}, answer:{}", ec, answer);
        return status_t::OK;
    }

    //LOG_DEBUG("Session created without content or model, return directly");
    resp->set_error_code(OK);
    return status_t::OK;
}

status_t
api_handler::ModifySessionTitle(ctx_t                                      *ctx,
                                const ::GrpcLibrary::ModifySessionTitleReq *req,
                                ::GrpcLibrary::ModifySessionTitleResp *resp)
{
    int64_t     id      = req->user_id();
    int         user_id = req->user_id();
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
    if(db_mgr::instance().exec("sqlite", sql) != OK)
    {
        resp->set_error_code(ERR_SQLITE_EXEC_FAIL);
        LOG_ERROR("Failed to update session for sql: {}", sql);
        return status_t::OK;
    }

    resp->set_error_code(OK);
    return status_t::OK;
}

status_t api_handler::GetSkillInfo(ctx_t                                *ctx,
                                   const ::GrpcLibrary::GetSkillInfoReq *req,
                                   ::GrpcLibrary::GetSkillInfoResp      *resp)
{
    int64_t id    = req->id();
    int     limit = req->limit();
    limit         = limit < 0 || limit > 50 ? 50 : limit;
    resp->set_error_code(ERR_FAIL);
    LOG_DEBUG("Received GetSkillInfo request. id: {}, limit: {}", id, limit);

    std::string sql;
    if(id < 0)
        sql = SQL_SELECT_SKILL_INFO
              + hj::fmt(" LIMIT {};", std::to_string(limit));
    else
        sql = hj::fmt(SQL_SELECT_SKILL_INFO_BY_ID, id) + " LIMIT 1;";

    LOG_DEBUG("{}", sql);
    db_mgr::query_ret rows;
    if(db_mgr::instance().query(rows, "sqlite", sql) != OK)
    {
        resp->set_error_code(ERR_SQLITE_EXEC_FAIL);
        LOG_ERROR("Failed to query skill info for sql: {}", sql);
        return status_t::OK;
    }
    for(const auto row : rows)
    {
        auto item = resp->add_skills();
        item->set_id(std::stoll(row[0]));
        // platform
        item->set_name(row[2]);
        item->set_desc(row[3]);
        item->set_publisher(row[4]);
        item->set_version(row[5]);
        item->set_timestamp(row[6]);
        item->set_hash(row[7]);

        LOG_DEBUG("GetSkillInfo id: {}, name: {}, desc: {}, publisher: {}, "
                  "version: {}, timestamp: {}, hash: {}",
                  item->id(),
                  item->name(),
                  item->desc(),
                  item->publisher(),
                  item->version(),
                  item->timestamp(),
                  item->hash());
    }

    resp->set_error_code(OK);
    return status_t::OK;
}

status_t api_handler::Download(ctx_t                            *ctx,
                               const ::GrpcLibrary::DownloadReq *req,
                               ::GrpcLibrary::DownloadResp      *resp)
{
    std::string hash    = req->hash();
    int         user_id = req->user_id();
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
    if(db_mgr::instance().query(rows, "sqlite", sql) != OK)
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