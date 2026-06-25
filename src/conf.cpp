#include "conf.h"

#include <exception>
#include <hj/util/string_util.hpp>

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

int conf::sqlite_msg_limit()
{
    return _cfg.get<int>("sqlite.msg_limit", 1000);
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

std::unordered_map<std::string, std::string> conf::llm_files()
{
    auto             str = _cfg.get<std::string>("llm.llm_files", "");
    std::string_view tag{";", 1};
    auto             items = hj::string_util::split(str, tag);

    std::string_view                             kv_tag{":", 1};
    std::unordered_map<std::string, std::string> files;
    for(const auto &item : items)
    {
        auto kv = hj::string_util::split(item, kv_tag);
        if(kv.size() == 2)
            files[kv[0]] = kv[1];
    }
    return files;
}

int conf::llm_ctx_window_sz()
{
    return _cfg.get<int>("llm.ctx_window_sz", 4096);
}

int conf::llm_num_threads()
{
    return _cfg.get<int>("llm.num_threads", 1);
}