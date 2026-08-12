#include "db_mgr.h"

#include <hj/log/logger.hpp>
#include <hj/db/db_conn.hpp>
#include <hj/db/db_conn_pool.hpp>
#include <hj/db/sqlite.hpp>

#include "conf.h"
#include "global.h"

void db_mgr::init()
{
    _pools.clear();
    // init sqlite
    auto sqlite_confs = conf::instance().sqlites();
    for(auto conf : sqlite_confs)
    {
        auto pool = hj::db_conn_pool<hj::db_conn>::create(
            conf.capa,
            conf.min_sz,
            [path = conf.path]() -> std::shared_ptr<hj::db_conn> {
                auto conn = std::make_shared<hj::sqlite>();
                if(!conn->open(path))
                    return nullptr;

                return std::static_pointer_cast<hj::db_conn>(conn);
            },
            [](std::shared_ptr<hj::db_conn> conn) -> bool {
                if(!conn)
                    return false;

                auto valid = static_cast<hj::sqlite *>(conn.get())->is_open();
                return valid;
            });
        add(conf.id, pool);
        LOG_DEBUG(
            "Initialized db_mgr with sqlite id:%1, path:%2, capa:%3, min_sz:%4",
            conf.id,
            conf.path,
            conf.capa,
            conf.min_sz);
    }

    return;
}

const std::vector<std::string> &db_mgr::supported_db_types()
{
    static std::vector<std::string> ret{"sqlite"};
    return ret;
}

int db_mgr::add(const uint32_t db_id, pool_ptr_t pool)
{
    if(_pools.size() <= db_id)
        _pools.resize(db_id + 1);

    _pools[db_id] = std::move(pool);
    return OK;
}