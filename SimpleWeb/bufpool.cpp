#include "bufpool.h"

BufferPool::BufferPool(size_t bufferSize, size_t initialCount)
    : bufferSize_(bufferSize)
{
    for (size_t i = 0; i < initialCount; ++i) {
        pool_.emplace(std::make_shared<std::vector<char>>(bufferSize_));
    }
}

std::shared_ptr<std::vector<char>> BufferPool::Rent() {
    std::lock_guard<std::mutex> lock(mutex_);
    if (pool_.empty()) {
        return std::make_shared<std::vector<char>>(bufferSize_);
    }
    auto buffer = pool_.front();
    pool_.pop();
    return buffer;
}

void BufferPool::Return(std::shared_ptr<std::vector<char>> buffer) {
    std::lock_guard<std::mutex> lock(mutex_);
    pool_.push(buffer);
}