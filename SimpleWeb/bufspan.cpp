#include "bufspan.h"

#include <stdexcept>

BufferSpan::BufferSpan(char* data, size_t length)
    : data_(data), length_(length) {}
char* BufferSpan::Data()
{
    return data_;
}
size_t BufferSpan::Length() const
{
    return length_;
}
BufferSpan BufferSpan::Slice(size_t offset, size_t len) const
{
    if (offset + len > length_) throw std::out_of_range("Slice out of range");
    return BufferSpan(data_ + offset, len);
}