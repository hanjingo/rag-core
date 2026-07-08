#ifndef WATCH_DOG_H
#define WATCH_DOG_H

#include <string>
#include <set>

class watch_dog
{
  public:
    watch_dog() {}
    ~watch_dog() {}

    bool watch(const std::string &output);

  private:
};


#endif