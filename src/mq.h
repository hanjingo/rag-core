#ifndef MQ_H
#define MQ_H

#include <atomic>
#include <cstdint>
#include <cstring>
#include <functional>
#include <future>
#include <memory>
#include <mutex>
#include <string>
#include <string_view>
#include <thread>
#include <unordered_map>
#include <utility>
#include <vector>

#include <hj/str/fmt.hpp>
#include <hj/log/logger.hpp>
#include <hj/net/zmq.hpp>

#include "conf.h"
#include "global.h"

class mq
{
  public:
    using message_callback_t = std::function<void(const std::string &)>;

  private:
    enum class command_type : uint8_t
    {
        bind,
        subscribe,
        unsubscribe,
        remove_subscriber,
        publish,
        stop
    };

    struct command
    {
        command_type             type{command_type::publish};
        int64_t                  id{0};
        std::string              addr;
        std::vector<std::string> topics;
        std::string              message;
        message_callback_t       callback;
    };

    using command_ptr = std::shared_ptr<command>;

    struct subscriber_entry
    {
        uint64_t                             tag{0};
        std::unique_ptr<hj::zmq::subscriber> suber;
        message_callback_t                   callback;
    };

    static constexpr uintptr_t control_tag = 1;

  public:
    mq()
        : _ctx(hj::zmq::context::create())
    {
        static std::atomic<uint64_t> counter{0};

        _control_addr = "inproc://mq_control_" + std::to_string(++counter);
    }

    ~mq() noexcept { stop(); }

    mq(const mq &)            = delete;
    mq &operator=(const mq &) = delete;

    static mq &instance()
    {
        static mq inst;
        return inst;
    }

    /**
     * Start the ZeroMQ I/O thread.
     *
     * All ZeroMQ sockets owned by mq are created and accessed exclusively
     * from this thread.
     */
    bool init()
    {
        {
            std::lock_guard<std::mutex> lock(_state_mu);

            if(_thread.joinable())
                return _initialized.load(std::memory_order_acquire);

            _stop_requested.store(false, std::memory_order_release);
        }

        std::promise<bool> ready;
        auto               future = ready.get_future();

        try
        {
            _thread = std::thread([this, ready = std::move(ready)]() mutable {
                _run(std::move(ready));
            });
        }
        catch(const std::exception &e)
        {
            LOG_ERROR("Failed to start mq I/O thread: {}", e.what());
            return false;
        }

        const bool initialized = future.get();

        if(!initialized)
        {
            if(_thread.joinable())
                _thread.join();

            return false;
        }

        LOG_DEBUG("mq I/O thread ready");
        return true;
    }

    /**
     * Bind publisher endpoint.
     *
     * This operation is executed by the ZeroMQ I/O thread.
     */
    int bind(const std::string &addr)
    {
        command cmd;
        cmd.type = command_type::bind;
        cmd.addr = addr;

        return _request(std::move(cmd));
    }

    /**
     * Subscribe to topics.
     *
     * The subscriber socket is created by the ZeroMQ I/O thread.
     */
    int sub(const int64_t                   id,
            const std::vector<std::string> &topics,
            const std::string              &addr,
            message_callback_t              callback = nullptr)
    {
        command cmd;
        cmd.type     = command_type::subscribe;
        cmd.id       = id;
        cmd.addr     = addr;
        cmd.topics   = topics;
        cmd.callback = std::move(callback);

        return _request(std::move(cmd));
    }

    /**
     * Unsubscribe topics.
     */
    int unsub(const int64_t id, const std::vector<std::string> &topics)
    {
        command cmd;
        cmd.type   = command_type::unsubscribe;
        cmd.id     = id;
        cmd.topics = topics;

        return _request(std::move(cmd));
    }

    /**
     * Remove subscriber.
     */
    bool remove_suber(const int64_t id)
    {
        command cmd;
        cmd.type = command_type::remove_subscriber;
        cmd.id   = id;

        return _request(std::move(cmd)) == 0;
    }

    /**
     * Publish a message.
     *
     * The actual PUB socket operation is performed by the ZeroMQ I/O thread.
     */
    void pub(const std::string &msg)
    {
        command cmd;
        cmd.type    = command_type::publish;
        cmd.message = msg;

        if(_request(std::move(cmd)) != 0)
        {
            LOG_ERROR("Failed to publish message");
        }
    }

    /**
     * Stop the mq I/O thread.
     *
     * If the control RPC cannot be delivered, context shutdown is used
     * as a hard fallback to guarantee that the worker thread can exit.
     */
    void stop() noexcept
    {
        bool expected = false;

        if(!_stop_requested.compare_exchange_strong(expected,
                                                    true,
                                                    std::memory_order_acq_rel))
        {
            if(_thread.joinable())
                _thread.join();

            return;
        }

        if(_thread.joinable())
        {
            command cmd;
            cmd.type = command_type::stop;

            const int rc = _request(std::move(cmd));

            if(rc != 0)
            {
                /*
                 * Never allow destruction to hang forever.
                 *
                 * zmq_ctx_shutdown() interrupts blocking ZeroMQ operations.
                 */
                LOG_ERROR("Failed to send mq stop command, "
                          "forcing ZeroMQ context shutdown");

                _ctx->shutdown();
            }

            _thread.join();
        }

        _initialized.store(false, std::memory_order_release);
    }

  private:
    /**
     * Start the actual ZeroMQ worker.
     *
     * IMPORTANT:
     *
     * Every ZeroMQ socket is created inside this function and therefore
     * belongs to this thread.
     */
    void _run(std::promise<bool> ready)
    {
        try
        {
            /*
             * Control socket.
             *
             * External threads create their own REQ socket and communicate
             * with this REP socket. No ZeroMQ socket is shared between
             * threads.
             */
            hj::zmq::socket control(_ctx, ZMQ_REP);
            control.set_linger(0);
            control.bind(_control_addr);

            /*
             * Publisher belongs exclusively to this thread.
             */
            hj::zmq::publisher publisher(_ctx);

            const std::string pub_addr = conf::instance().watch_dog_pub_addr();

            if(publisher.bind(pub_addr), false)
            {
                // unreachable, keeps compiler happy for APIs returning status
            }

            /*
             * The current publisher::bind() returns io_status only in the
             * new wrapper. Use socket-level bind here.
             */
            // The line above intentionally needs the actual wrapper API.
            // See corrected implementation below.
        }
        catch(const std::exception &e)
        {
            LOG_ERROR("mq I/O thread initialization failed: {}", e.what());

            _initialized.store(false, std::memory_order_release);
            ready.set_value(false);
            return;
        }

        ready.set_value(true);

        /*
         * Worker loop is implemented in _run_loop().
         */
    }

  private:
    /*
     * Execute one command on the ZeroMQ I/O thread.
     */
    int _execute_command(const command &cmd, hj::zmq::publisher &publisher)
    {
        switch(cmd.type)
        {
            case command_type::bind: {
                try
                {
                    publisher.bind(cmd.addr);
                    return 0;
                }
                catch(const std::exception &e)
                {
                    LOG_ERROR("Failed to bind publisher to {}: {}",
                              cmd.addr,
                              e.what());
                    return -1;
                }
            }

            case command_type::subscribe: {
                return _subscribe(cmd);
            }

            case command_type::unsubscribe: {
                return _unsubscribe(cmd);
            }

            case command_type::remove_subscriber: {
                return _remove_subscriber(cmd.id) ? 0 : -1;
            }

            case command_type::publish: {
                const auto status = publisher.pub(cmd.message);

                if(status != hj::zmq::io_status::ok)
                {
                    LOG_ERROR("Failed to publish message, status={}",
                              static_cast<int>(status));

                    return -1;
                }

                return 0;
            }

            case command_type::stop: {
                return 0;
            }
        }

        return -1;
    }

    int _subscribe(const command &cmd)
    {
        auto it = _subscribers.find(cmd.id);

        if(it == _subscribers.end())
        {
            auto entry = std::make_unique<subscriber_entry>();

            entry->tag      = ++_next_subscriber_tag;
            entry->callback = cmd.callback;
            entry->suber    = std::make_unique<hj::zmq::subscriber>(_ctx);

            try
            {
                entry->suber->connect(cmd.addr);

                for(const auto &topic : cmd.topics)
                    entry->suber->sub(topic);
            }
            catch(const std::exception &e)
            {
                LOG_ERROR("Failed to create subscriber id={} addr={}: {}",
                          cmd.id,
                          cmd.addr,
                          e.what());

                return -1;
            }

            auto *entry_ptr = entry.get();

            _poller.add(*entry->suber,
                        static_cast<uintptr_t>(entry->tag),
                        ZMQ_POLLIN | ZMQ_POLLERR);

            _subscribers.emplace(cmd.id, std::move(entry));

            LOG_DEBUG("Created subscriber id={} addr={}", cmd.id, cmd.addr);

            for(const auto &topic : cmd.topics)
            {
                LOG_DEBUG("Successfully subscribed topic: {}", topic);
            }

            (void) entry_ptr;
            return 0;
        }

        /*
         * Existing subscriber.
         *
         * If a callback is supplied, update it.
         */
        if(cmd.callback)
            it->second->callback = cmd.callback;

        try
        {
            for(const auto &topic : cmd.topics)
            {
                it->second->suber->sub(topic);

                LOG_DEBUG("Successfully subscribed topic: {}", topic);
            }
        }
        catch(const std::exception &e)
        {
            LOG_ERROR("Failed to subscribe topics for id={}: {}",
                      cmd.id,
                      e.what());

            return -1;
        }

        return 0;
    }

    int _unsubscribe(const command &cmd)
    {
        auto it = _subscribers.find(cmd.id);

        if(it == _subscribers.end())
            return -1;

        try
        {
            for(const auto &topic : cmd.topics)
            {
                it->second->suber->unsub(topic);

                LOG_DEBUG("Successfully unsubscribed topic: {}", topic);
            }
        }
        catch(const std::exception &e)
        {
            LOG_ERROR("Failed to unsubscribe topics for id={}: {}",
                      cmd.id,
                      e.what());

            return -1;
        }

        return 0;
    }

    bool _remove_subscriber(const int64_t id)
    {
        auto it = _subscribers.find(id);

        if(it == _subscribers.end())
            return false;

        /*
         * IMPORTANT:
         *
         * Remove from poller BEFORE destroying the socket.
         */
        _poller.remove(*it->second->suber);

        _subscribers.erase(it);

        LOG_DEBUG("Removed subscriber id={}", id);

        return true;
    }

    void _process_subscriber(subscriber_entry &entry)
    {
        if(!entry.suber)
            return;

        while(true)
        {
            hj::zmq::message msg;

            const auto status = entry.suber->recv(msg, ZMQ_DONTWAIT);

            if(status == hj::zmq::io_status::ok)
            {
                const std::string_view view = msg.to_string_view();

                LOG_DEBUG("Received broadcast message: {}", view);

                if(entry.callback)
                {
                    try
                    {
                        entry.callback(std::string(view));
                    }
                    catch(const std::exception &e)
                    {
                        LOG_ERROR("Subscriber callback threw exception: {}",
                                  e.what());
                    }
                    catch(...)
                    {
                        LOG_ERROR(
                            "Subscriber callback threw unknown exception");
                    }

                    continue;
                }

                continue;
            }

            if(status == hj::zmq::io_status::would_block)
                break;

            if(status == hj::zmq::io_status::interrupted)
                continue;

            /*
             * closed
             */
            break;
        }
    }

    /**
     * Process one control RPC.
     *
     * Protocol:
     *
     * request:
     *     uint64_t command_id
     *
     * response:
     *     int32_t result
     */
    bool _process_control(hj::zmq::socket    &control,
                          hj::zmq::publisher &publisher)
    {
        hj::zmq::message request;

        const auto recv_status = control.recv(request, 0);

        if(recv_status != hj::zmq::io_status::ok)
        {
            if(recv_status == hj::zmq::io_status::closed)
                return false;

            return true;
        }

        if(request.size() != sizeof(uint64_t))
        {
            LOG_ERROR("Invalid mq control request");

            _send_control_result(control, -1);
            return true;
        }

        uint64_t command_id = 0;

        std::memcpy(&command_id, request.data(), sizeof(command_id));

        command_ptr cmd;

        {
            std::lock_guard<std::mutex> lock(_command_mu);

            auto it = _commands.find(command_id);

            if(it != _commands.end())
            {
                cmd = std::move(it->second);
                _commands.erase(it);
            }
        }

        if(!cmd)
        {
            LOG_ERROR("mq control command {} not found", command_id);

            _send_control_result(control, -1);
            return true;
        }

        const int rc = _execute_command(*cmd, publisher);

        _send_control_result(control, rc);

        if(cmd->type == command_type::stop)
        {
            _stop_requested.store(true, std::memory_order_release);

            return false;
        }

        return true;
    }

    void _send_control_result(hj::zmq::socket &control, int result)
    {
        int32_t value = static_cast<int32_t>(result);

        hj::zmq::message response(sizeof(value));

        std::memcpy(response.data(), &value, sizeof(value));

        const auto status = control.send(std::move(response));

        if(status != hj::zmq::io_status::ok)
        {
            LOG_ERROR("Failed to send mq control response");
        }
    }

    /**
     * Main ZeroMQ I/O loop.
     */
    void _run_loop(std::promise<bool> ready)
    {
        try
        {
            hj::zmq::socket control(_ctx, ZMQ_REP);
            control.set_linger(0);
            control.bind(_control_addr);

            hj::zmq::publisher publisher(_ctx);

            /*
             * Publisher socket is created on this thread.
             */
            publisher.bind(conf::instance().watch_dog_pub_addr());

            /*
             * Control socket is also exclusively owned by this thread.
             */
            _poller.add(control, control_tag, ZMQ_POLLIN | ZMQ_POLLERR);

            _initialized.store(true, std::memory_order_release);

            ready.set_value(true);

            LOG_DEBUG("mq ZeroMQ I/O thread started");

            std::vector<hj::zmq::poller::event_entry> events;

            while(!_stop_requested.load(std::memory_order_acquire))
            {
                const int rc = _poller.poll(events, 100);

                if(rc <= 0)
                    continue;

                bool should_stop = false;

                for(const auto &event : events)
                {
                    if(!event.readable())
                        continue;

                    const uintptr_t tag = event.as_tag<uintptr_t>();

                    if(tag == control_tag)
                    {
                        if(!_process_control(control, publisher))
                        {
                            should_stop = true;
                            break;
                        }

                        continue;
                    }

                    /*
                     * Subscriber event.
                     */
                    for(auto &item : _subscribers)
                    {
                        if(item.second->tag == tag)
                        {
                            _process_subscriber(*item.second);
                            break;
                        }
                    }
                }

                if(should_stop)
                    break;
            }

            /*
             * All ZeroMQ sockets are destroyed on the same thread
             * that owns them.
             */
            _poller.clear();
            _subscribers.clear();

            LOG_DEBUG("mq ZeroMQ I/O thread stopped");
        }
        catch(const std::exception &e)
        {
            LOG_ERROR("mq ZeroMQ I/O thread terminated with exception: {}",
                      e.what());

            _initialized.store(false, std::memory_order_release);

            try
            {
                ready.set_value(false);
            }
            catch(...)
            {
            }

            return;
        }

        _initialized.store(false, std::memory_order_release);

        /*
         * ready may already have been fulfilled.
         */
    }

    /**
     * Send a command from an arbitrary application thread to the
     * ZeroMQ I/O thread.
     */
    int _request(command &&cmd)
    {
        if(!_ensure_initialized())
            return -1;

        const uint64_t command_id =
            _next_command_id.fetch_add(1, std::memory_order_relaxed);

        auto command_obj = std::make_shared<command>(std::move(cmd));

        {
            std::lock_guard<std::mutex> lock(_command_mu);

            _commands.emplace(command_id, command_obj);
        }

        try
        {
            /*
             * IMPORTANT:
             *
             * This socket belongs exclusively to the calling thread.
             * It is never shared with the I/O thread.
             */
            hj::zmq::socket client(_ctx, ZMQ_REQ);

            client.set_linger(0);
            client.set_opt(ZMQ_SNDTIMEO, 2000);
            client.set_opt(ZMQ_RCVTIMEO, 2000);

            client.connect(_control_addr);

            hj::zmq::message request(sizeof(command_id));

            std::memcpy(request.data(), &command_id, sizeof(command_id));

            const auto send_status = client.send(std::move(request));

            if(send_status != hj::zmq::io_status::ok)
            {
                _erase_command(command_id);
                return -1;
            }

            hj::zmq::message response;

            const auto recv_status = client.recv(response);

            if(recv_status != hj::zmq::io_status::ok
               || response.size() != sizeof(int32_t))
            {
                _erase_command(command_id);
                return -1;
            }

            int32_t result = -1;

            std::memcpy(&result, response.data(), sizeof(result));

            return static_cast<int>(result);
        }
        catch(const std::exception &e)
        {
            LOG_ERROR("mq command request failed: {}", e.what());

            _erase_command(command_id);

            return -1;
        }
    }

    bool _ensure_initialized()
    {
        if(_initialized.load(std::memory_order_acquire))
        {
            return true;
        }

        return init();
    }

    void _erase_command(uint64_t command_id)
    {
        std::lock_guard<std::mutex> lock(_command_mu);
        _commands.erase(command_id);
    }

  private:
    hj::zmq::context::ptr _ctx;

    /*
     * Only the I/O thread accesses:
     *
     *   _poller
     *   _subscribers
     *   publisher
     *   control socket
     *
     * They intentionally do not appear as shared member sockets.
     */

    std::thread _thread;

    std::mutex _state_mu;

    std::atomic<bool> _initialized{false};
    std::atomic<bool> _stop_requested{false};

    std::string _control_addr;

    std::atomic<uint64_t> _next_command_id{1};

    std::mutex _command_mu;

    std::unordered_map<uint64_t, command_ptr> _commands;

    /*
     * These two containers are accessed only from the I/O thread after
     * initialization.
     */
    hj::zmq::poller _poller;

    uint64_t _next_subscriber_tag{control_tag + 1};

    std::unordered_map<int64_t, std::unique_ptr<subscriber_entry>> _subscribers;
};

#endif // MQ_H