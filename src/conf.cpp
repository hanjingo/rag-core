#include "conf.h"

conf::conf()
    : _cfg{}
{
}

conf::~conf()
{
}

bool conf::init()
{
    if(!_cfg.read_file("./core.ini"))
        return false;

    return true;
}

hj::ini conf::data()
{
    return _cfg;
}