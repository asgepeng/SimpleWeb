#pragma once
#include <thread>
#include <vector>
#include <queue>
#include <mutex>
#include <condition_variable>
#include <functional>
#include <atomic>

class RequestHandler 
{
public:
    using Task = std::function<void()>;

    RequestHandler(size_t threadCount = std::thread::hardware_concurrency(), size_t maxQueueSize = 1024);
    ~RequestHandler();
    bool Enqueue(Task task);
    void Shutdown();
private:
    void workerLoop();

    std::vector<std::thread> workers;
    std::queue<Task> tasks;
    std::mutex queueMutex;
    std::condition_variable cv;
    std::atomic<bool> stopFlag;
    std::atomic<bool> shutdownCalled;
    size_t maxQueueSize;
};
