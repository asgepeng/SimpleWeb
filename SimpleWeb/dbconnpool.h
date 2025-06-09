#pragma once
#include "db.h"
#include <queue>
#include <mutex>
#include <condition_variable>
#include <memory>

namespace Data {
    class DbClientPool 
    {
    public:
        DbClientPool(size_t maxSize, const std::string& connectionString);
        ~DbClientPool();

        std::shared_ptr<DbClient> Acquire();
        void Release(std::shared_ptr<DbClient> client);

    private:
        std::queue<std::shared_ptr<DbClient>> pool;
        std::mutex mtx;
        std::condition_variable cv;
        std::string connStr;
        size_t maxSize;
        size_t currentSize = 0;
    };
}
