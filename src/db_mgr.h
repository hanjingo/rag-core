#ifndef DB_MGR_H
#define DB_MGR_H

#include <memory>
#include <vector>
#include <string>
#include <utility>

#include <hj/db/db_conn.hpp>
#include <hj/db/db_conn_pool.hpp>

#include "err.h"

class db_mgr
{
  public:
    using conn_ptr_t = std::shared_ptr<hj::db_conn>;
    using pool_ptr_t = std::shared_ptr<hj::db_conn_pool<hj::db_conn>>;
    using query_ret  = hj::db_conn::ret_t;

  public:
    db_mgr()
        : _pools{}
    {
    }
    ~db_mgr() { _pools.clear(); }

    static db_mgr &instance()
    {
        static db_mgr inst;
        return inst;
    }

    void                            init();
    const std::vector<std::string> &supported_db_types();

    int add(const uint32_t db_id, pool_ptr_t pool);

    template <typename... Args>
    int exec(const std::string &sql, Args &&...args)
    {
        const uint32_t db_id = DB_SQLITE1;
        if(_pools.size() <= db_id)
            return ERR_DB_NOT_EXIST;

        auto conn = _pools[db_id]->acquire();
        if(!conn)
            return ERR_DB_CONN_POOL_EMPTY;

        auto ret = conn->exec(sql, std::forward<Args>(args)...);
        return ret.ec.value();
    }

    template <typename... Args>
    int query(query_ret &outs, const std::string &sql, Args &&...args)
    {
        const uint32_t db_id = DB_SQLITE1;
        if(_pools.size() <= db_id)
            return ERR_DB_NOT_EXIST;

        auto conn = _pools[db_id]->acquire();
        if(!conn)
            return ERR_DB_CONN_POOL_EMPTY;

        auto ret = conn->query(outs, sql, std::forward<Args>(args)...);
        return ret.value();
    }

  private:
    std::vector<pool_ptr_t> _pools;
};

#endif