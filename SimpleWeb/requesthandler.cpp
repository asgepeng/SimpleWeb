#include "requesthandler.h"

RequestHandler::RequestHandler(size_t threadCount, size_t maxQueueSize) : stopFlag(false), maxQueueSize(maxQueueSize), shutdownCalled(false)
{
    for (size_t i = 0; i < threadCount; ++i)
    {
        workers.emplace_back([this]() { this->workerLoop(); });
    }
}
RequestHandler::~RequestHandler()
{
    Shutdown();
}
bool RequestHandler::Enqueue(Task task)
{
    {
        std::unique_lock<std::mutex> lock(queueMutex);
        if (stopFlag || tasks.size() >= maxQueueSize)
            return false;

        tasks.push(std::move(task));
    }
    cv.notify_one();
    return true;
}

void RequestHandler::Shutdown()
{
    if (shutdownCalled.exchange(true)) return; // hanya sekali

    {
        std::lock_guard<std::mutex> lock(queueMutex);
        stopFlag = true;
    }
    cv.notify_all();

    for (auto& t : workers) {
        if (t.joinable())
            t.join();
    }

    workers.clear();
}

void RequestHandler::workerLoop()
{
    while (true) {
        Task task;
        {
            std::unique_lock<std::mutex> lock(queueMutex);
            cv.wait(lock, [this]() { return stopFlag || !tasks.empty(); });

            if (stopFlag && tasks.empty())
                return;

            task = std::move(tasks.front());
            tasks.pop();
        }

        task();
    }
}
