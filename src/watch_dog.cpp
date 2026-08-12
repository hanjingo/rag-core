#include "watch_dog.h"

#include <hj/util/string_util.hpp>
#include <hj/log/logger.hpp>
#include <hj/encoding/json.hpp>

#include "global.h"
#include "mq.h"
#include "db_mgr.h"
#include "api.grpc.pb.h"

bool watch_dog::watch(const std::string &output)
{
    // TODO
    return true;
}

bool watch_dog::watch_pub(const std::vector<std::string> &topics,
                          const std::string              &addr)
{
    mq::instance().sub(
        0,
        topics,
        addr,
        std::bind(&watch_dog::_on_pub_msg, this, std::placeholders::_1));
    return true;
}

void watch_dog::_on_pub_msg(const std::string &msgs)
{
    LOG_DEBUG("watch_dog::_on_pub_msg with msgs: {}", msgs);

    try
    {
        auto arr = hj::json::parse(msgs);
        if(!arr.is_array())
        {
            LOG_INFO("Invalid pub msg format: {}", msgs);
            return;
        }
        for(auto item : arr)
        {
            if(!item.contains("topic") || !item["topic"].is_string())
                continue;

            std::string topic = item["topic"].get<std::string>();
            if(topic == TOPIC_PLUGIN_PUB)
            {
                std::string hash      = item.value("hash", "");
                std::string name      = item.value("name", "");
                std::string desc      = item.value("desc", "");
                std::string publisher = item.value("publisher", "");
                std::string version   = item.value("version", "");
                std::string timestamp = item.value("timestamp", "");
                int         platform  = item.value("platform", 0);
                LOG_DEBUG(
                    "Received plugin info: hash: {}, name: {}, desc: {}, "
                    "publisher: {}, version: {}, timestamp: {}, platform: {}",
                    hash,
                    name,
                    desc,
                    publisher,
                    version,
                    timestamp,
                    platform);
                if(db_mgr::instance().exec(SQL_INSERT_PLUGIN_INFO,
                                           hash,
                                           name,
                                           desc,
                                           publisher,
                                           version,
                                           timestamp,
                                           platform)
                   != OK)
                {
                    LOG_ERROR("Failed to insert plugin info with hash:{}",
                              hash);
                    return;
                }
            } else
            {
                LOG_DEBUG("unrecognized msg!");
            }
        }
    }
    catch(const hj::json::exception &e)
    {
        LOG_ERROR("JSON parse or operational exception: {}, raw msgs: {}",
                  e.what(),
                  msgs);
    }
}