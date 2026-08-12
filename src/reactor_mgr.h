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

    void register_recognize(int64_t session_id, RecognizeAudioReactor *reactor)
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
            LOG_DEBUG("Unregistered RecognizeAudioReactor for session_id: {}",
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
        LOG_WARN("No active RecognizeAudioReactor found for session_id: {}",
                 session_id);
        return false;
    }

    RecognizeAudioReactor *get_recognize(int64_t session_id)
    {
        std::lock_guard<std::mutex> lock(_mu);
        auto                        it = _active_recognizes.find(session_id);
        if(it != _active_recognizes.end())
            return it->second;

        LOG_WARN("No active RecognizeAudioReactor found for session_id: {}",
                 session_id);
        return nullptr;
    }

  private:
    std::unordered_map<int64_t, RecognizeAudioReactor *> _active_recognizes;
    std::mutex                                           _mu;
};

class embedding_reactor_mgr
{
  public:
    static embedding_reactor_mgr &instance()
    {
        static embedding_reactor_mgr manager;
        return manager;
    }

    void register_embedding(int64_t task_id, EmbeddingReactor *reactor)
    {
        std::lock_guard<std::mutex> lock(_mu);
        _active_embeddings[task_id] = reactor;
        LOG_DEBUG("Registered EmbeddingReactor for task_id: {}", task_id);
    }

    void unregister_embedding(int64_t task_id)
    {
        std::lock_guard<std::mutex> lock(_mu);
        auto                        it = _active_embeddings.find(task_id);
        if(it != _active_embeddings.end())
        {
            _active_embeddings.erase(it);
            LOG_DEBUG("Unregistered EmbeddingReactor for task_id: {}", task_id);
        }
    }

    bool stop_embedding(int64_t task_id)
    {
        std::lock_guard<std::mutex> lock(_mu);
        auto                        it = _active_embeddings.find(task_id);
        if(it != _active_embeddings.end() && it->second != nullptr)
        {
            it->second->Stop();
            return true;
        }
        LOG_WARN("No active EmbeddingReactor found to stop for task_id: {}",
                 task_id);
        return false;
    }

    EmbeddingReactor *get_embedding(int64_t task_id)
    {
        std::lock_guard<std::mutex> lock(_mu);
        auto                        it = _active_embeddings.find(task_id);
        if(it != _active_embeddings.end())
            return it->second;

        LOG_WARN("No active EmbeddingReactor found for task_id: {}", task_id);
        return nullptr;
    }

  private:
    std::unordered_map<int64_t, EmbeddingReactor *> _active_embeddings;
    std::mutex                                      _mu;
};

class subscribe_reactor_mgr
{
  public:
    static subscribe_reactor_mgr &instance()
    {
        static subscribe_reactor_mgr manager;
        return manager;
    }

    SubscribeReactor *get_active_suber(int64_t user_id)
    {
        std::lock_guard<std::mutex> lock(_mu);
        auto                        it = _active_subers.find(user_id);
        if(it != _active_subers.end())
            return it->second;

        return nullptr;
    }

    void register_suber(int64_t user_id, SubscribeReactor *reactor)
    {
        std::lock_guard<std::mutex> lock(_mu);
        _active_subers[user_id] = reactor;
    }

    void unregister_suber(int64_t user_id)
    {
        std::lock_guard<std::mutex> lock(_mu);
        _active_subers.erase(user_id);
    }

    bool stop_suber(int64_t user_id)
    {
        std::lock_guard<std::mutex> lock(_mu);
        auto                        it = _active_subers.find(user_id);
        if(it != _active_subers.end() && it->second != nullptr)
        {
            it->second->Stop();
            return true;
        }
        return false;
    }

  private:
    std::unordered_map<int64_t, SubscribeReactor *> _active_subers;
    std::mutex                                      _mu;
};

#endif // REACTOR_MGR_H