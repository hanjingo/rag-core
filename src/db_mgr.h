#ifndef DB_MGR_H
#define DB_MGR_H

#include <memory>
#include <vector>
#include <string>

#include "global.h"
#include "sqlite.h"

class db_mgr
{
  public:
    using query_ret = std::vector<std::vector<std::string>>;

  public:
    db_mgr()
        : _dbs{}
    {
    }
    ~db_mgr() { _dbs.clear(); }

    static db_mgr &instance()
    {
        static db_mgr inst;
        return inst;
    }

    void                            init();
    const std::vector<std::string> &supported_db_types();

    int add(std::unique_ptr<db> &&db);
    int exec(const std::string &db_id, const std::string &sql);
    // for example: outs=[["name", "age"], [...], ...]
    int
    query(query_ret &outs, const std::string &db_id, const std::string &sql);
    int64_t last_insert_id(const std::string &db_id, const std::string &table);

  private:
    std::vector<std::unique_ptr<db>> _dbs;
};

#endif