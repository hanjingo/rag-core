#include "watch_dog.h"

#include <hj/util/string_util.hpp>
#include <hj/log/logger.hpp>

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

void watch_dog::_on_pub_msg(const std::string &msg)
{
    LOG_DEBUG("watch_dog::_on_pub_msg: msg: {}", msg);
    std::string payload;
    std::string topic;
    auto        parts = hj::string_util::split(msg, TOPIC_SEPARATOR);
    if(parts.size() < 2)
    {
        LOG_WARN("Received pub message with unexpected format: {}", msg);
        return;
    }

    topic   = parts[0];
    payload = parts[1];
    LOG_DEBUG("parse pub msg with topic:{}, payload:{}", topic, payload);

    if(topic == TOPIC_PLUGIN_PUB)
    {
        ::GrpcLibraryV1::Plugin info;
        if(!info.ParseFromString(payload))
        {
            LOG_WARN("Failed to parse Plugin message from payload: {}",
                     payload);
            return;
        }

        LOG_DEBUG("Received plugin info: hash: {}, name: {}, desc: {}, "
                  "publisher: {}, version: {}, timestamp: {}, platform: {}",
                  info.hash(),
                  info.name(),
                  info.desc(),
                  info.publisher(),
                  info.version(),
                  info.timestamp(),
                  info.platform());
        auto sql = hj::sqlite::mprintf(SQL_INSERT_PLUGIN_INFO,
                                       info.hash().c_str(),
                                       info.platform(),
                                       info.name().c_str(),
                                       info.desc().c_str(),
                                       info.publisher().c_str(),
                                       info.version().c_str(),
                                       info.timestamp().c_str());
        LOG_DEBUG("{}", sql);
        if(db_mgr::instance().exec(DB_SQLITE, sql) != OK)
        {
            LOG_ERROR("Failed to insert plugin info for sql: {}", sql);
            return;
        }
    }
}