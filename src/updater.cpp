#include "updater.h"

#include <hj/log/logger.hpp>
#include <hj/util/string_util.hpp>
#include <hj/db/sqlite.hpp>

#include "global.h"
#include "conf.h"
#include "db_mgr.h"

bool updater::check(const std::string &platform,
                    const std::string &arch,
                    const std::string &version)
{
    if(platform.empty() || arch.empty() || version.empty())
    {
        LOG_ERROR("platform, arch or version info is empty.");
        return false;
    }

    auto key = std::string("client_") + platform + "_" + arch;
    auto it  = _clients.find(key);
    if(it == _clients.end())
    {
        LOG_ERROR("No client config found for platform: {}, arch: {}",
                  platform,
                  arch);
        return false;
    }

    auto arr = hj::string_util::split(version, ".");
    if(arr.empty())
    {
        LOG_ERROR("Parse version:{} failed", version);
        return false;
    }

    uint8_t min_versions[3] = {it->second.version_major,
                               it->second.version_minor,
                               it->second.version_patch};
    for(size_t i = 0; i < arr.size() && i < 3; ++i)
    {
        try
        {
            auto part = std::stoi(arr[i]);
            if(min_versions[i] > part)
            {
                LOG_INFO("Client version {} is less than min compatible "
                         "version {}.{}.{}. "
                         "Force update required.",
                         version,
                         min_versions[0],
                         min_versions[1],
                         min_versions[2]);
                return false;
            }
        }
        catch(const std::exception &e)
        {
            LOG_ERROR("Failed to parse version part: {}. Error: {}",
                      arr[i],
                      e.what());
            return false;
        }
    }

    LOG_DEBUG("updater::check exit. Client version {} is compatible with min "
              "version {}.{}.{}",
              version,
              min_versions[0],
              min_versions[1],
              min_versions[2]);
    return true;
}

void updater::init(bool force)
{
    if(_inited.load() && !force)
        return;

    _inited.store(true);
    _version = VERSION;

    if(ENV_OS == "windows")
        _platform = 1;
    else if(ENV_OS == "linux")
        _platform = 2;
    else if(ENV_OS == "macos")
        _platform = 3;
    else
        _platform = 0;

    if(ENV_ARCH == "x86")
        _arch = 1;
    else if(ENV_ARCH == "x64")
        _arch = 2;
    else if(ENV_ARCH == "arm64")
        _arch = 3;
    else
        _arch = 0;

    _release_time = COMPILE_TIME;
    LOG_DEBUG(
        "Version info loaded. rag-core.version: {}, rag-core.platform: {}, "
        "rag-core.arch: {}, rag-core.release_time: {}",
        _version,
        _platform,
        _arch,
        _release_time);

    _clients = conf::instance().clients();
    for(auto item : _clients)
    {
        LOG_DEBUG("init client config with platform:{}, arch:{}, "
                  "rollout_percent:{}, min_version:{}.{}.{}",
                  item.second.platform,
                  item.second.arch,
                  item.second.rollout_percent,
                  item.second.version_major,
                  item.second.version_minor,
                  item.second.version_patch);
    }

    // migrate database schema if needed
    migrate();
}

void updater::migrate()
{
    // init sqlite database schema if not exists
    auto sqlite_confs = conf::instance().sqlites();
    for(auto conf : sqlite_confs)
    {
        LOG_INFO("SQLite config - id: {}, path: {}, capa: {}, min_sz: {}, "
                 "sql_path: {}",
                 conf.id,
                 conf.path,
                 conf.capa,
                 conf.min_sz,
                 conf.sql_path);

        // Check if the schema_version table exists
        hj::sqlite sqlite;
        if(!sqlite.open(conf.path))
        {
            LOG_ERROR("Failed to open SQLite database at path: {}. Error: {}",
                      conf.path,
                      sqlite.get_last_error());
            continue;
        }
        int               current_version = 0;
        db_mgr::query_ret rows;
        if(sqlite.query(rows, SQL_SELECT_SCHEMA_VERSION).value() == OK)
        {
            if(!rows.empty())
                current_version = rows.get_or<int64_t>(0, 0, 0);
        }

        if(current_version < SCHEMA_VERSION)
        {
            // schema_version table does not exist, create it and set version to 1
            std::ifstream sql_file(conf.sql_path);
            if(!sql_file.is_open())
            {
                LOG_ERROR("Failed to open SQL file at path: {}", conf.sql_path);
                continue;
            }

            std::stringstream buf;
            buf << sql_file.rdbuf();

            bool success    = true;
            auto statements = hj::string_util::split(buf.str(), ";");
            sqlite.begin();
            for(const auto &stmt : statements)
            {
                if(stmt.find_first_not_of(" \t\n\r") == std::string::npos)
                    continue;

                auto res = sqlite.exec(stmt + ";");
                if(res.ec.value() == OK)
                    continue;

                LOG_ERROR("Failed to execute SQL statement: {}. Error: {}",
                          stmt,
                          sqlite.get_last_error());
                success = false;
                break;
            }

            if(!success)
            {
                sqlite.rollback();
                LOG_ERROR(
                    "Database migration failed for path: {}. Rolled back.",
                    conf.path);
                continue;
            }

            // check if the schema_version table exists and update the version
            db_mgr::query_ret tmp_rows;
            if(sqlite.query(tmp_rows, SQL_SELECT_SCHEMA_VERSION).value() != OK
               || tmp_rows.empty()
               || tmp_rows.get_or<int64_t>(0, 0, 0) < SCHEMA_VERSION)
            {
                sqlite.rollback();
                LOG_ERROR("Failed to query schema_version after migration. "
                          "Rows count: {}, Database path: {}",
                          tmp_rows.rows(),
                          conf.path);
                continue;
            }

            // ok, commit the transaction
            current_version = tmp_rows.get_or<int64_t>(0, 0, 0);
            sqlite.commit();
            LOG_INFO("Database migrated to version {} by sql:{}",
                     current_version,
                     conf.sql_path);
        }
        LOG_INFO("Current database: {} schema version: {}",
                 conf.path,
                 current_version);
    }
}