#ifndef MQ_H
#define MQ_H

#include <atomic>
#include <functional>
#include <memory>
#include <mutex>
#include <string>
#include <unordered_map>

#include <hj/log/logger.hpp>
#include <hj/net/zmq.hpp>
#include <hj/sync/channel.hpp>
#include <hj/encoding/fmt.hpp>

#include "conf.h"
#include "global.h"
#include "sync.h"

class mq
{
  public:
    using message_callback_t = std::function<void(const std::string &)>;

    mq()
        : _ctx(hj::zmq::context::create())
        , _poller()
        , _puber(_ctx)
        , _is_polling(false)
        , _ch(1024)
    {
    }

    ~mq() { _is_stop.store(true); }

    static mq &instance()
    {
        static mq inst;
        return inst;
    }

    void init()
    {
        if(_is_inited.exchange(true))
            return;

        if(0 != bind(conf::instance().watch_dog_pub_addr()))
        {
            LOG_ERROR("Failed to bind publisher to address: {}",
                      conf::instance().watch_dog_pub_addr());
            return;
        }

        thread_pool::instance()->enqueue([this]() {
            while(!_is_stop.load())
            {
                {
                    std::lock_guard<std::mutex> lock(_mu);
                    if(_poller.size() == 0)
                    {
                        std::this_thread::sleep_for(
                            std::chrono::milliseconds(100));
                        continue;
                    }
                }

                poll(100);
            }
        });
    }

    int bind(const std::string &addr) { return _puber.bind(addr); }

    int sub(const int64_t                   id,
            const std::vector<std::string> &topics,
            const std::string              &addr,
            message_callback_t              callback = nullptr)
    {
        std::lock_guard<std::mutex> lock(_mu);
        auto                        it = _subscribers.find(id);
        if(it == _subscribers.end())
        {
            // create subscriber
            auto suber = std::make_shared<hj::zmq::subscriber>(_ctx);
            if(suber->connect(addr) != 0)
            {
                LOG_ERROR(
                    "Failed to create and connect subscriber to address: {}",
                    addr);
                return -1;
            }

            // create wrapper entry
            auto entry      = std::make_shared<subscriber_entry>();
            entry->suber    = suber;
            entry->callback = callback;
            it              = _subscribers.emplace(id, entry).first;

            // add event
            _poller.add(*suber, entry.get(), ZMQ_POLLIN | ZMQ_POLLERR);
        }

        for(auto topic : topics)
        {
            if(0 != it->second->suber->sub(topic))
            {
                LOG_ERROR("Failed to subscribe to topic: {}", topic);
                return -1;
            }

            LOG_DEBUG("Successfully subscribed topic: {}", topic);
        }

        LOG_DEBUG("Successfully subscribed topics: {}",
                  hj::format("{}", topics));
        return 0;
    }

    int unsub(const int64_t id, const std::vector<std::string> &topics)
    {
        std::lock_guard<std::mutex> lock(_mu);
        auto                        itr = _subscribers.find(id);
        if(itr == _subscribers.end())
            return -1;

        for(auto topic : topics)
        {
            if(0 != itr->second->suber->unsub(topic))
            {
                LOG_ERROR("Failed to unsubscribe to topic: {}", topic);
                return -1;
            }

            LOG_DEBUG("Successfully unsubscribed topic: {}", topic);
        }

        LOG_DEBUG("Successfully unsubscribed topics: {}",
                  hj::format("{}", topics));
        return 0;
    }

    bool remove_suber(const int64_t id)
    {
        std::lock_guard<std::mutex> lock(_mu);

        auto it = _subscribers.find(id);
        if(it != _subscribers.end())
        {
            if(it->second && it->second->suber)
                _poller.remove(*(it->second->suber));

            _subscribers.erase(it);
            return true;
        }

        return false;
    }

    void pub(const std::string &msg)
    {
        _ch << msg;
        std::string buf;
        while(_ch.try_dequeue(buf))
        {
            if(_puber.pub(buf) < 0)
            {
                LOG_ERROR("Failed to publish message: {}", buf);
                return;
            }

            LOG_DEBUG("Successfully published message: {}", buf);
        }
    }

    void poll(long timeout_ms = -1)
    {
        std::vector<subscriber_entry *> entries_to_process;
        {
            std::lock_guard<std::mutex> lock(_mu);
            int                         rc = _poller.poll(timeout_ms);
            if(rc <= 0)
                return;

            for(size_t i = 0; i < _poller.size(); ++i)
            {
                const auto &item = _poller.items()[i];
                if(item.revents & ZMQ_POLLIN)
                {
                    auto *entry =
                        static_cast<subscriber_entry *>(_poller.user_data(i));
                    if(entry)
                        entries_to_process.push_back(entry);
                }
            }
        }

        for(auto *entry : entries_to_process)
        {
            if(!entry || !entry->suber)
                continue;

            std::string msg;
            while(entry->suber->recv(msg, ZMQ_DONTWAIT) >= 0)
            {
                LOG_DEBUG("Received broadcast message: {}", msg);
                if(entry->callback)
                    entry->callback(msg);
            }
        }
    }

  private:
    struct subscriber_entry
    {
        std::shared_ptr<hj::zmq::subscriber> suber;
        message_callback_t                   callback;
    };

    void _process(subscriber_entry *entry)
    {
        if(!entry || !entry->suber)
            return;

        std::string msg;
        while(entry->suber->recv(msg, ZMQ_DONTWAIT) >= 0)
        {
            LOG_DEBUG("Received broadcast message: {}", msg);
            if(entry->callback)
                entry->callback(msg);
        }
    }

  private:
    hj::zmq::context::ptr _ctx;
    hj::zmq::poller       _poller;
    hj::zmq::publisher    _puber;

    std::atomic<bool>        _is_inited{false};
    std::atomic<bool>        _is_polling{false};
    std::atomic<bool>        _is_stop{false};
    hj::channel<std::string> _ch;

    std::mutex                                                     _mu;
    std::unordered_map<int64_t, std::shared_ptr<subscriber_entry>> _subscribers;
};

#endif // MQ_H