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
    for(size_t i = 0; i < size; ++i)
        _buffer.push_back(data[i]);
}

size_t audio_buffer::pop(float *data, size_t size)
{
    std::unique_lock<std::mutex> lock(_mu);
    size_t                       actual = std::min(size, _buffer.size());
    for(size_t i = 0; i < actual; ++i)
    {
        data[i] = _buffer.front();
        _buffer.pop_front();
    }

    for(size_t i = actual; i < size; ++i)
        data[i] = 0.0f;
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