#include "dbconnpool.h"

namespace Data 
{
    DbClientPool::DbClientPool(size_t maxSize, const std::string& connectionString)
        : maxSize(maxSize), connStr(connectionString) {}

    DbClientPool::~DbClientPool() 
    {
        std::lock_guard<std::mutex> lock(mtx);
        while (!pool.empty()) pool.pop();
    }

    std::shared_ptr<DbClient> DbClientPool::Acquire() 
    {
        std::unique_lock<std::mutex> lock(mtx);
        cv.wait(lock, [this] { return !pool.empty() || currentSize < maxSize; });

        if (!pool.empty()) 
        {
            auto client = pool.front();
            pool.pop();
            return client;
        }
        auto client = std::make_shared<DbClient>(connStr);
        if (client->Connect())
        {
            ++currentSize;
            return client;
        }

        return nullptr;
    }

    void DbClientPool::Release(std::shared_ptr<DbClient> client) 
    {
        std::lock_guard<std::mutex> lock(mtx);
        pool.push(client);
        cv.notify_one();
    }

}
