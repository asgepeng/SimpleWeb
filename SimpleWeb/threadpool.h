#pragma once
#include <vector>
#include <thread>
#include <queue>
#include <functional>
#include <mutex>
#include <condition_variable>
#include <atomic>

class ThreadPool {
public:
    ThreadPool(size_t numThreads);
    ~ThreadPool();
    template<class F, class... Args>
    void enqueue(F&& f, Args&&... args);

private:
    std::vector<std::thread> workers;
    std::queue<std::function<void()>> tasks;

    std::mutex queueMutex;
    std::condition_variable condition;
    std::atomic<bool> stopFlag;
};

template<class F, class ...Args>
inline void ThreadPool::enqueue(F&& f, Args && ...args)
{
    {
        std::unique_lock<std::mutex> lock(queueMutex);

        // don't allow enqueue after stop
        if (stopFlag)
            throw std::runtime_error("enqueue on stopped ThreadPool");

        tasks.emplace([f = std::forward<F>(f), ... args = std::forward<Args>(args)]() mutable {
            f(std::forward<Args>(args)...);
            });
    }
    condition.notify_one();
}
