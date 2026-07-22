#include "updater.h"

#include <hj/log/logger.hpp>
#include <hj/util/string_util.hpp>

#include "global.h"
#include "conf.h"

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
}