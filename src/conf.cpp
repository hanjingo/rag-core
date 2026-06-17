#include "conf.h"

#include <exception>

conf::conf()
    : _cfg{}
{
    if(!_cfg.read_file("./core.ini"))
        throw std::runtime_error("Failed to read config file: core.ini");
}

conf::~conf()
{
}

hj::ini conf::data()
{
    return _cfg;
}

std::string conf::issuer_id()
{
    return _cfg.get<std::string>("issuer.id", "default");
}

hj::license::sign_algo conf::issuer_algo()
{
    int algo = _cfg.get<int>("issuer.algo", 0);
    switch(algo)
    {
        case 1:
            return hj::license::sign_algo::rsa256;
        default:
            return hj::license::sign_algo::none;
    }
}

std::vector<std::string> conf::issuer_keys()
{
    std::vector<std::string> keys;
    keys.push_back(_cfg.get<std::string>("issuer.pub_key", ""));
    keys.push_back(_cfg.get<std::string>("issuer.pri_key", ""));
    keys.push_back(_cfg.get<std::string>("issuer.encrypted_pub_key", ""));
    keys.push_back(_cfg.get<std::string>("issuer.encrypted_pri_key", ""));
    return keys;
}

int conf::issuer_valid_times()
{
    return _cfg.get<int>("issuer.valid_times", 1000);
}

int conf::issuer_leeway()
{
    return _cfg.get<int>("issuer.leeway", 1);
}

std::string conf::verifier_id()
{
    return _cfg.get<std::string>("verifier.id", "default");
}

hj::license::sign_algo conf::verifier_algo()
{
    int algo = _cfg.get<int>("verifier.algo", 0);
    switch(algo)
    {
        case 1:
            return hj::license::sign_algo::rsa256;
        default:
            return hj::license::sign_algo::none;
    }
}

std::vector<std::string> conf::verifier_keys()
{
    std::vector<std::string> keys;
    keys.push_back(_cfg.get<std::string>("verifier.pub_key", ""));
    keys.push_back(_cfg.get<std::string>("verifier.pri_key", ""));
    keys.push_back(_cfg.get<std::string>("verifier.encrypted_pub_key", ""));
    keys.push_back(_cfg.get<std::string>("verifier.encrypted_pri_key", ""));
    return keys;
}
