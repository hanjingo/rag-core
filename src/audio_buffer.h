#ifndef AUDIO_BUFFER_H
#define AUDIO_BUFFER_H

#include <mutex>
#include <condition_variable>

#include <hj/io/ring_buffer.hpp>

class audio_buffer
{
  public:
    explicit audio_buffer(size_t capacity);
    ~audio_buffer();

    void   push(const float *data, size_t size);
    size_t pop(float *data, size_t size);
    size_t
    pop_until(float *data, size_t size, size_t min_size, int timeout_ms = 3000);
    void clear();

    size_t size() const;
    size_t capacity() const;

  private:
    hj::ring_buffer<float>  _buffer;
    mutable std::mutex      _mu;
    std::condition_variable _cv;
};

#endif // AUDIO_BUFFER_H