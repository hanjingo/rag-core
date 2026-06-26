#include "db_mgr.h"

#include "conf.h"
#include <hj/log/log.hpp>

void db_mgr::init()
{
    _dbs.clear();

    auto id   = conf::instance().sqlite_id();
    auto path = conf::instance().sqlite_path();
    auto capa = conf::instance().sqlite_pool();
    add(std::make_unique<sqlite>(id.c_str(), path.c_str(), capa));
    LOG_DEBUG("Initialized db_mgr with sqlite db, id:{}, path:{}, capa:{}",
              id,
              path,
              capa);
    return;
}

const std::vector<std::string> &db_mgr::supported_db_types()
{
    static std::vector<std::string> ret{"sqlite"};
    return ret;
}

int db_mgr::add(std::unique_ptr<db> &&elem)
{
    for(const auto &e : _dbs)
        if(e->id() == elem->id())
            return ERR_DB_EXISTED;

    _dbs.emplace_back(std::move(elem));
    return OK;
}

int db_mgr::exec(const std::string &db_id, const std::string &sql)
{
    for(const auto &e : _dbs)
    {
        if(e->id() != db_id)
            continue;

        return e->exec(sql.c_str());
    }

    return ERR_DB_NOT_EXIST;
}

int db_mgr::query(query_ret         &outs,
                  const std::string &db_id,
                  const std::string &sql)
{
    for(const auto &e : _dbs)
    {
        if(e->id() != db_id)
            continue;

        return e->query(outs, sql.c_str());
    }

    return ERR_DB_NOT_EXIST;
}

int64_t db_mgr::last_insert_id(const std::string &db_id,
                               const std::string &table)
{
    for(const auto &e : _dbs)
    {
        if(e->id() != db_id)
            continue;

        return e->last_insert_id(table.c_str());
    }

    return -1;
}