// reactor_mgr.h
#ifndef REACTOR_MGR_H
#define REACTOR_MGR_H

#include <unordered_map>
#include <mutex>
#include <memory>
#include <atomic>

#include "server_reactor.h"

class query_reactor_mgr
{
  public:
    static query_reactor_mgr &instance()
    {
        static query_reactor_mgr manager;
        return manager;
    }

    void register_query(int64_t session_id, QueryReactor *reactor)
    {
        std::lock_guard<std::mutex> lock(_mu);
        _active_queries[session_id] = reactor;
    }

    void unregister_query(int64_t session_id)
    {
        std::lock_guard<std::mutex> lock(_mu);
        _active_queries.erase(session_id);
    }

    bool stop_query(int64_t session_id)
    {
        std::lock_guard<std::mutex> lock(_mu);
        auto                        it = _active_queries.find(session_id);
        if(it != _active_queries.end() && it->second != nullptr)
        {
            it->second->Stop();
            return true;
        }
        return false;
    }

  private:
    std::unordered_map<int64_t, QueryReactor *> _active_queries;
    std::mutex                                  _mu;
};

class recognize_reactor_mgr
{
  public:
    static recognize_reactor_mgr &instance()
    {
        static recognize_reactor_mgr manager;
        return manager;
    }

    void register_recognize(int64_t session_id, RecognizeReactor *reactor)
    {
        std::lock_guard<std::mutex> lock(_mu);
        _active_recognizes[session_id] = reactor;
    }

    void unregister_recognize(int64_t session_id)
    {
        std::lock_guard<std::mutex> lock(_mu);
        auto                        it = _active_recognizes.find(session_id);
        if(it != _active_recognizes.end())
        {
            _active_recognizes.erase(it);
            LOG_DEBUG("Unregistered RecognizeReactor for session_id: {}",
                      session_id);
        }
    }

    bool stop_recognize(int64_t session_id)
    {
        std::lock_guard<std::mutex> lock(_mu);
        auto                        it = _active_recognizes.find(session_id);
        if(it != _active_recognizes.end() && it->second != nullptr)
        {
            it->second->Stop();
            return true;
        }
        LOG_WARN("No active RecognizeReactor found for session_id: {}",
                 session_id);
        return false;
    }

    RecognizeReactor *get_recognize(int64_t session_id)
    {
        std::lock_guard<std::mutex> lock(_mu);
        auto                        it = _active_recognizes.find(session_id);
        if(it != _active_recognizes.end())
            return it->second;

        LOG_WARN("No active RecognizeReactor found for session_id: {}",
                 session_id);
        return nullptr;
    }

  private:
    std::unordered_map<int64_t, RecognizeReactor *> _active_recognizes;
    std::mutex                                      _mu;
};

#endif // REACTOR_MGR_H