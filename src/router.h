#ifndef ROUTER_H
#define ROUTER_H

#include <string>

class router
{
  public:
    router();
    ~router();

    static router &instance()
    {
        static router inst;
        return inst;
    }

    virtual void route(const std::string &prompt);
};

#endif // ROUTER_H