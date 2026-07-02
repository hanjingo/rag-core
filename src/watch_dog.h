#ifndef WATCH_DOG_H
#define WATCH_DOG_H

#include <string>
#include <set>

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
    int                   _max_repeats  = 3;
    int                   _repeat_count = 0;
    std::set<std::string> _recent_outputs;
};


#endif