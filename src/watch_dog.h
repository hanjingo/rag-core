#ifndef WATCH_DOG_H
#define WATCH_DOG_H

#include <string>
#include <unordered_map>

class watch_dog
{
  public:
    watch_dog(int max_repeat_times)
        : _max_repeats{max_repeat_times}
    {
    }
    ~watch_dog() {}

    bool watch(const std::string &output);

  private:
    int                                      _max_repeats;
    std::unordered_map<std::string, int64_t> _repeats;
};


#endif