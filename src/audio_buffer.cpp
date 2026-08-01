#include "audio_buffer.h"

audio_buffer::audio_buffer(size_t capacity)
    : _buffer(capacity)
{
}

audio_buffer::~audio_buffer()
{
}

void audio_buffer::push(const float *data, size_t size)
{
    std::unique_lock<std::mutex> lock(_mu);
    _buffer.insert(_buffer.end(), data, data + size);
    _cv.notify_one();
}

size_t audio_buffer::pop(float *data, size_t size)
{
    std::unique_lock<std::mutex> lock(_mu);
    size_t                       actual = std::min(size, _buffer.size());
    std::copy(_buffer.begin(), _buffer.begin() + actual, data);
    _buffer.erase(_buffer.begin(), _buffer.begin() + actual);
    std::fill(data + actual, data + size, 0.0f);
    return actual;
}

size_t audio_buffer::pop_until(float *data,
                               size_t size,
                               size_t min_size,
                               int    timeout_ms)
{
    std::unique_lock<std::mutex> lock(_mu);
    if(min_size > _buffer.capacity())
        return 0;

    _cv.wait_for(lock,
                 std::chrono::milliseconds(timeout_ms),
                 [this, min_size]() { return _buffer.size() >= min_size; });

    size_t actual = std::min(size, _buffer.size());
    std::copy(_buffer.begin(), _buffer.begin() + actual, data);
    _buffer.erase(_buffer.begin(), _buffer.begin() + actual);
    std::fill(data + actual, data + size, 0.0f);
    return actual;
}

void audio_buffer::clear()
{
    std::unique_lock<std::mutex> lock(_mu);
    _buffer.clear();
}

size_t audio_buffer::size() const
{
    std::unique_lock<std::mutex> lock(_mu);
    return _buffer.size();
}

size_t audio_buffer::capacity() const
{
    std::unique_lock<std::mutex> lock(_mu);
    return _buffer.capacity();
}