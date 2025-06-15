#pragma once
#include <vector>
#include <queue>
#include <memory>
#include <mutex>

class BufferPool {
public:
    BufferPool(size_t bufferSize, size_t initialCount = 32);

    std::shared_ptr<std::vector<char>> Rent();
    void Return(std::shared_ptr<std::vector<char>> buffer);

private:
    size_t bufferSize_;
    std::mutex mutex_;
    std::queue<std::shared_ptr<std::vector<char>>> pool_;
};