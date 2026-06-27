#ifndef SYNC_H
#define SYNC_H

#include <hj/sync/channel.hpp>
#include <hj/sync/thread_pool.hpp>

#include "conf.h"

class thread_pool
{
  public:
    thread_pool()
        : _pool{conf::instance().sync_thread_pool_size()}
    {
    }
    ~thread_pool() { _pool.clear(); }

    static thread_pool &instance()
    {
        static thread_pool inst;
        return inst;
    }

    template <class F, class... Args>
    auto enqueue(F &&f, Args &&...args)
        -> std::future<typename std::invoke_result<F, Args...>::type>
    {
        return _pool.enqueue(std::forward<F>(f), std::forward<Args>(args)...);
    }

  private:
    hj::thread_pool _pool;
};

#endif // SYNC_H