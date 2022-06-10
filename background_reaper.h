#pragma once

#include <atomic>
#include <chrono>
#include <condition_variable>
#include <mutex>
#include <thread>
#include <type_traits>

#include "boost/interprocess/sync/null_mutex.hpp"
#include "boost/lockfree/queue.hpp"

/// 后台线程回收资源
template <typename ResourceType, typename ReaperFunction>
class BackgroundReaper
{
    using value_type = ResourceType;
    using pointer = ResourceType *;

public:
    BackgroundReaper(ReaperFunction &&reaper)
        : reaper_(std::move(reaper)), resources_(10)
    {
        static_assert(std::is_same<decltype(reaper_(pointer{})), void>::value, "");

        reaper_thread_ = std::thread([this] {
            ReaperLoop();
        });
    }

    ~BackgroundReaper()
    {
        stopping_ = true;
        if (reaper_thread_.joinable())
        {
            reaper_thread_.join();
        }
    }

    /// 添加待回收的指针
    void Reap(pointer ptr)
    {
        assert(!stopping_);
        while (!resources_.push(ptr))
            ;
    }

    /// 通知立即回收正在等待的指针
    void Notify()
    {
        assert(!stopping_);
        waiter_.notify_one();
    }

private:
    void ReaperLoop()
    {
        while (!stopping_)
        {
            std::unique_lock<decltype(mutex_)> cv_lock(mutex_);
            waiter_.wait_for(cv_lock, std::chrono::milliseconds(100), [&] {
                return !resources_.empty();
            });

            resources_.consume_all(reaper_);
        }
    }

private:
    ReaperFunction reaper_;
    std::thread reaper_thread_;
    std::atomic<bool> stopping_ = {false};
    std::mutex mutex_;
    std::condition_variable waiter_;
    boost::lockfree::queue<pointer> resources_;
};