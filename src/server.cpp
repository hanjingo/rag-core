#include "server.h"

#include "conf.h"
#include "db_mgr.h"
#include <hj/log/log.hpp>
#include <hj/encoding/fmt.hpp>
#include <hj/time/date_time.hpp>

status_t api_handler::Query(ctx_t                         *ctx,
                            const ::GrpcLibrary::QueryReq *req,
                            ::GrpcLibrary::QueryResp      *resp)
{
    int64_t     id      = req->id();
    int32_t     user_id = req->user_id();
    std::string auth    = req->auth();
    std::string content = req->content();
    resp->set_error_code(OK);
    resp->set_id(id);

    // TODO check privilege

#ifdef DEBUG
    resp->set_content("Hello, world");
#endif

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
    resp->set_error_code(OK);

#ifdef DEBUG
    user_id = 1;
#endif

    std::string sql;
    if(id > 0)
        sql = hj::fmt(SQL_SELECT_BY_ID, id);
    else
        sql = hj::fmt(SQL_SELECT_BY_USER_ID, user_id);
    if(limit > 0)
        sql += hj::fmt(" LIMIT {}", limit);

    LOG_DEBUG("{}", sql);
    db_mgr::query_ret rows;
    if(db_mgr::instance().query(rows, "sqlite", sql) != OK)
    {
        LOG_ERROR("Failed to query history for sql: {}", sql);
        LOG_FLUSH();
        return status_t::OK;
    }
    for(const auto &row : rows)
    {
        auto item = resp->add_sessions();
        item->set_id(std::stoll(row[0]));
        item->set_user_id(std::stoi(row[1]));
        item->set_title(row[2]);
        item->set_content(row[3]);
        item->set_timestamp(row[4]);
        item->set_vector_index(row[5]);

        LOG_DEBUG("GetSession id: {}, user_id: {}, title: {}, content: "
                  "{}, timestamp: {}, "
                  "vector_index: {}",
                  item->id(),
                  item->user_id(),
                  item->title(),
                  item->content(),
                  item->timestamp(),
                  item->vector_index());
    }

    LOG_FLUSH();
    return status_t::OK;
}

status_t api_handler::NewSession(ctx_t                              *ctx,
                                 const ::GrpcLibrary::NewSessionReq *req,
                                 ::GrpcLibrary::NewSessionResp      *resp)
{
    resp->set_error_code(OK);
    int         user_id = req->user_id();
    std::string auth    = req->auth();
    std::string title   = req->title();
    std::string content = "";
    std::string timestamp =
        hj::date_time::format(hj::date_time::now(), "%Y-%m-%d %H:%M:%S");
    std::string vector_index = "";

#ifdef DEBUG
    user_id = 1;
#endif

    auto sql = hj::fmt(SQL_INSERT_SESSION, user_id, title, "", timestamp, "");
    LOG_DEBUG("{}", sql);
    if(db_mgr::instance().exec("sqlite", sql) != OK)
    {
        resp->set_error_code(ERR_SQLITE_EXEC_FAIL);
        LOG_ERROR("Failed to insert session for sql: {}", sql);
        LOG_FLUSH();
        return status_t::OK;
    }

    auto id      = db_mgr::instance().last_insert_id("sqlite", "session");
    auto session = resp->mutable_session();
    session->set_id(id);
    session->set_user_id(user_id);
    session->set_title(title);
    session->set_content(content);
    session->set_timestamp(timestamp);
    session->set_vector_index(vector_index);

    LOG_FLUSH();
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
    resp->set_error_code(OK);
    resp->set_id(id);
    resp->set_title(title);

    // TODO check privilege

    auto sql = hj::fmt(SQL_UPDATE_SESSION_TITLE_BY_ID, title, id);
    LOG_DEBUG("{}", sql);
    if(db_mgr::instance().exec("sqlite", sql) != OK)
    {
        resp->set_error_code(ERR_SQLITE_EXEC_FAIL);
        LOG_ERROR("Failed to update session for sql: {}", sql);
        LOG_FLUSH();
        return status_t::OK;
    }

    LOG_FLUSH();
    return status_t::OK;
}