#ifndef WATCH_DOG_H
#define WATCH_DOG_H

#include <string>
#include <set>
#include <vector>

#include "conf.h"

class watch_dog
{
  public:
    static watch_dog &instance()
    {
        static watch_dog dog;
        return dog;
    }

    bool watch(const std::string &output);

    bool watch_pub(const std::vector<std::string> &topics,
                   const std::string &addr = conf::instance().publish_addr());

  private:
    void _on_pub_msg(const std::string &msgs);

  private:
    watch_dog()  = default;
    ~watch_dog() = default;
};


#endif