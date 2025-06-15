#pragma once
#include <cstddef>

class BufferSpan {
public:
    BufferSpan(char* data, size_t length);

    char* Data();
    size_t Length() const;
    BufferSpan Slice(size_t offset, size_t length) const;

private:
    char* data_;
    size_t length_;
};
