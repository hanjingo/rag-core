#ifndef CONF_H
#define CONF_H

#include <hj/encoding/ini.hpp>

class conf
{
  public:
    conf();
    ~conf();

    static conf &instance()
    {
        static conf inst;
        return inst;
    }

    bool    init();
    hj::ini data();

  private:
    hj::ini _cfg;
};

#endif