#include "server.h"

#include "conf.h"
#include "db_mgr.h"
#include <hj/log/log.hpp>

status_t api_handler::Query(ctx_t                         *ctx,
                            const ::GrpcLibrary::QueryReq *req,
                            ::GrpcLibrary::QueryResp      *resp)
{
    resp->set_content("Hello, world");
    return status_t::OK;
}

status_t api_handler::GetHistory(ctx_t                              *ctx,
                                 const ::GrpcLibrary::GetHistoryReq *req,
                                 ::GrpcLibrary::GetHistoryResp      *resp)
{
    int user_id = req->id();

#ifdef DEBUG
    user_id = 1;
#endif

    // SELECT timestamp, content FROM session WHERE user_id = 1;
    std::string sql = "SELECT timestamp, content FROM session WHERE user_id = "
                      + std::to_string(user_id) + ";";
    LOG_DEBUG("{}", sql);
    db_mgr::query_ret rows;
    if(db_mgr::instance().query(rows, "sqlite", sql) != OK)
    {
        LOG_ERROR("Failed to query history for user_id: {}, sql: {}",
                  user_id,
                  sql);
        LOG_FLUSH();
        return status_t::OK;
    }
    for(const auto &row : rows)
    {
        auto *item = resp->add_history();
        item->set_datetime(row[0]);
        item->set_content(row[1]);

        LOG_DEBUG("datetime:{}, content:{}", row[0], row[1]);
    }

    LOG_FLUSH();
    return status_t::OK;
}