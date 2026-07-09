#include "conf.h"

#include <exception>
#include <hj/util/string_util.hpp>

#include <hj/log/logger.hpp>
#include <hj/io/filepath.hpp>

#include "global.h"

conf::conf()
    : _cfg{}
{
    if(!_cfg.read_file(
           hj::filepath::join(hj::filepath::pwd(), CORE_CONFIG_FILE).c_str()))
        throw std::runtime_error("Failed to read config file: "
                                 + std::string(CORE_CONFIG_FILE));

    _init();
}

conf::~conf()
{
}

hj::ini conf::data()
{
    return _cfg;
}

int conf::log_min_lvl()
{
    return _cfg.get<int>("log/min_lvl", 0);
}

int conf::log_flush_on()
{
    return _cfg.get<int>("log/flush_on", 0);
}

std::string conf::log_filename()
{
    return _cfg.get<std::string>("log/filename", "default.log");
}

int conf::log_max_size()
{
    return _cfg.get<int>("log/max_size", 1); // MB
}

int conf::log_max_files()
{
    return _cfg.get<int>("log/max_files", 5);
}

size_t conf::sync_write_queue_size()
{
    return _cfg.get<size_t>("sync/write_queue_size", 1);
}

unsigned long conf::sync_thread_pool_size()
{
    return _cfg.get<unsigned long>("sync/thread_pool_size", 1);
}

std::string conf::server_addr()
{
    return _cfg.get<std::string>("server/addr", "");
}

std::string conf::sqlite_id()
{
    return _cfg.get<std::string>("sqlite/id", "default");
}

std::string conf::sqlite_path()
{
    return _cfg.get<std::string>("sqlite/path", "default.db");
}

int conf::sqlite_pool()
{
    return _cfg.get<int>("sqlite/pool", 5);
}

int conf::sqlite_msg_limit()
{
    return _cfg.get<int>("sqlite/msg_limit", 1000);
}

std::string conf::issuer_id()
{
    return _cfg.get<std::string>("issuer/id", "default");
}

hj::license::sign_algo conf::issuer_algo()
{
    int algo = _cfg.get<int>("issuer/algo", 0);
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
    keys.push_back(_cfg.get<std::string>("issuer/pub_key", ""));
    keys.push_back(_cfg.get<std::string>("issuer/pri_key", ""));
    keys.push_back(_cfg.get<std::string>("issuer/encrypted_pub_key", ""));
    keys.push_back(_cfg.get<std::string>("issuer/encrypted_pri_key", ""));
    return keys;
}

int conf::issuer_valid_times()
{
    return _cfg.get<int>("issuer/valid_times", 1000);
}

int conf::issuer_leeway()
{
    return _cfg.get<int>("issuer/leeway", 1);
}

std::string conf::verifier_id()
{
    return _cfg.get<std::string>("verifier/id", "default");
}

hj::license::sign_algo conf::verifier_algo()
{
    int algo = _cfg.get<int>("verifier/algo", 0);
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
    keys.push_back(_cfg.get<std::string>("verifier/pub_key", ""));
    keys.push_back(_cfg.get<std::string>("verifier/pri_key", ""));
    keys.push_back(_cfg.get<std::string>("verifier/encrypted_pub_key", ""));
    keys.push_back(_cfg.get<std::string>("verifier/encrypted_pri_key", ""));
    return keys;
}

int conf::llm_max_repeats()
{
    return _cfg.get<int>("llm/max_repeats", 5);
}

int conf::llm_ctx_window_sz()
{
    return _cfg.get<int>("llm/ctx_window_sz", 4096);
}

int conf::llm_num_threads()
{
    return _cfg.get<int>("llm/num_threads", 1);
}

int conf::llm_local_prompt_threshold()
{
    return _cfg.get<int>("llm/local_prompt_threshold", 100);
}

std::unordered_set<std::string> conf::llm_hard_prompt_class()
{
    return _hard_prompt_class;
}

std::unordered_map<std::string, conf::remote_api_config> conf::llm_remote_apis()
{
    return _remote_apis;
}

std::unordered_map<std::string, conf::model_config> conf::llm_models()
{
    return _models;
}

std::unordered_map<std::string, conf::asr_ctx_config> conf::asr_ctxs()
{
    return _asr_ctxs;
}

int conf::asr_audio_buffer_size()
{
    return _cfg.get<int>("asr/audio_buffer_size", 16000);
}

int conf::asr_audio_min_chunk_size()
{
    return _cfg.get<int>("asr/audio_min_chunk_size", 16000 * 0.2);
}

void conf::_init()
{
    // init models
    auto             str = _cfg.get<std::string>("llm/models", "");
    std::string_view tag{";", 1};
    auto             items = hj::string_util::split(str, tag);
    _models.clear();
    for(const auto &item : items)
    {
        model_config config;
        config.id           = _cfg.get<std::string>(item + "/id", "");
        config.path         = _cfg.get<std::string>(item + "/path", "");
        config.n_gpu_layers = _cfg.get<int>(item + "/n_gpu_layers", -1);
        config.split_mode   = static_cast<llama_split_mode>(
            _cfg.get<int>(item + "/split_mode", 1));
        config.main_gpu      = _cfg.get<int>(item + "/main_gpu", 0);
        config.vocab_only    = _cfg.get<int>(item + "/vocab_only", 0) == 1;
        config.use_mmap      = _cfg.get<int>(item + "/use_mmap", 1) == 1;
        config.use_direct_io = _cfg.get<int>(item + "/use_direct_io", 0) == 1;
        config.use_mlock     = _cfg.get<int>(item + "/use_mlock", 0) == 1;
        config.check_tensors = _cfg.get<int>(item + "/check_tensors", 0) == 1;
        config.use_extra_bufts =
            _cfg.get<int>(item + "/use_extra_bufts", 1) == 1;
        config.no_host  = _cfg.get<int>(item + "/no_host", 0) == 1;
        config.no_alloc = _cfg.get<int>(item + "/no_alloc", 0) == 1;

        if(config.id.empty() || config.path.empty() || config.n_gpu_layers < -1
           || config.main_gpu < 0)
        {
            std::cerr << "config model: " << item << ", id: " << config.id
                      << ", path: " << config.path
                      << ", n_gpu_layers: " << config.n_gpu_layers
                      << ", main_gpu: " << config.main_gpu << " INVALID!!!"
                      << std::endl;
            continue;
        }

        _models[config.id] = config;
    }

    // init remote apis
    _remote_apis.clear();
    str              = _cfg.get<std::string>("llm/remote_apis", "");
    auto remote_apis = hj::string_util::split(str, tag);
    for(const auto &item : remote_apis)
    {
        remote_api_config config;
        config.id          = _cfg.get<std::string>(item + "/id", "");
        config.type        = _cfg.get<std::string>(item + "/type", "");
        config.api_key     = _cfg.get<std::string>(item + "/api_key", "");
        config.timeout_sec = _cfg.get<int>(item + "/timeout_sec", 5);

        if(config.id.empty() || config.type.empty())
        {
            std::cerr << "config remote api: " << item << ", id: " << config.id
                      << ", type: " << config.type << " INVALID!!!"
                      << std::endl;
            continue;
        }

        _remote_apis[config.id] = config;
    }

    // init prompt classes
    str          = _cfg.get<std::string>("llm/hard_prompt_class", "");
    auto classes = hj::string_util::split(str, tag);
    for(auto item : classes)
        _hard_prompt_class.emplace(item);

    auto asr_str   = _cfg.get<std::string>("asr/ctxs", "");
    auto asr_items = hj::string_util::split(asr_str, tag);
    _asr_ctxs.clear();
    for(const auto &item : asr_items)
    {
        asr_ctx_config config;
        config.id         = _cfg.get<std::string>(item + "/id", "");
        config.path       = _cfg.get<std::string>(item + "/path", "");
        config.use_gpu    = (_cfg.get<int>(item + "/use_gpu", 0) == 1);
        config.gpu_device = _cfg.get<int>(item + "/gpu_device", -1);

        _asr_ctxs[config.id] = config;
    }
}