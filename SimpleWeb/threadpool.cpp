#include "threadpool.h"

ThreadPool::ThreadPool(size_t numThreads) : stopFlag(false)
{
    for (size_t i = 0; i < numThreads; ++i)
    {
        workers.emplace_back([this]() {
            while (true)
            {
                std::function<void()> task;
                {   // lock scope
                    std::unique_lock<std::mutex> lock(queueMutex);
                    condition.wait(lock, [this]() { return stopFlag || !tasks.empty(); });

                    if (stopFlag && tasks.empty())
                        return; // exit thread

                    task = std::move(tasks.front());
                    tasks.pop();
                }

                task(); // run task outside lock
            }
            });
    }
}
ThreadPool::~ThreadPool()
{
    {
        std::unique_lock<std::mutex> lock(queueMutex);
        stopFlag = true;
    }
    condition.notify_all();

    for (auto& thread : workers)
    {
        if (thread.joinable())
            thread.join();
    }
}
