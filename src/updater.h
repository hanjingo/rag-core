#ifndef UPDATER_H
#define UPDATER_H

#include <atomic>
#include <string>
#include <unordered_map>

#include <hj/os/env.h>

#include "conf.h"

class updater
{
  public:
    updater() {}
    ~updater() = default;

    static updater *instance()
    {
        static updater inst;
        return &inst;
    }

    std::string version() { return _version; }
    uint8_t     platform() { return _platform; }
    uint8_t     arch() { return _arch; }
    std::string release_time() { return _release_time; }

    void init(bool force = false);
    bool check(const std::string &platform,
               const std::string &arch,
               const std::string &version);

  private:
    std::atomic<bool> _inited{false};

    std::string _version  = VERSION;
    uint8_t     _platform = 0;
    uint8_t     _arch     = 0;
    std::string _release_time;

    std::unordered_map<std::string, conf::client_config> _clients;
};

#endif // UPDATER_H