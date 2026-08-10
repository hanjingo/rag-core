#include "conf.h"

#include <exception>
#include <hj/util/string_util.hpp>

#include <hj/log/logger.hpp>
#include <hj/io/filepath.hpp>
#include <hj/os/env.h>

#include "global.h"

conf::conf()
    : _cfg{}
{
}

conf::~conf()
{
}

void conf::init(const std::string &config_file_path)
{
    if(_inited.load())
        return;

    _inited.store(true);
    _init(config_file_path);
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
    return _cfg.get<unsigned long>("sync/thread_pool_size", 4);
}

std::string conf::server_addr()
{
    return _cfg.get<std::string>("server/addr", "");
}

std::string conf::publish_addr()
{
    return _cfg.get<std::string>("server/publish_addr", INPROC_PUB_SUB_ADDR);
}

std::unordered_map<std::string, conf::client_config> conf::clients()
{
    return _clients;
}

std::string conf::clients_file_path()
{
#if defined(__APPLE__) // standard macOS bundle structure
    return hj::filepath::join(hj::filepath::pwd(), "../Resources");
#else
    return hj::filepath::join(hj::filepath::pwd());
#endif
}

std::string conf::sqlite_id()
{
    return _cfg.get<std::string>("sqlite/id", "default");
}

std::string conf::sqlite_path()
{
#if defined(__APPLE__) // standard macOS bundle structure
    return hj::filepath::join(
        hj::filepath::pwd() + "/../Resources",
        _cfg.get<std::string>("sqlite/path", "default.db"));
#else
    return _cfg.get<std::string>("sqlite/path", "default.db");
#endif
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
    return _cfg.get<int>("issuer/valid_times", 10000000);
}

int conf::issuer_expired_days()
{
    return _cfg.get<int>("issuer/expired_days", 30);
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

std::unordered_map<std::string, conf::remote_api_config> conf::llm_remote_apis()
{
    return _remote_apis;
}

std::unordered_map<std::string, conf::model_config> conf::llm_models()
{
    return _models;
}

std::string conf::llm_model_file_path()
{
#if defined(__APPLE__) // standard macOS bundle structure
    return hj::filepath::join(hj::filepath::pwd(), "../Resources");
#else
    return hj::filepath::join(hj::filepath::pwd());
#endif
}

std::string conf::llm_remote_api_file_path()
{
#if defined(__APPLE__) // standard macOS bundle structure
    return hj::filepath::join(hj::filepath::pwd(), "../Resources");
#else
    return hj::filepath::join(hj::filepath::pwd());
#endif
}

int conf::llm_remote_api_sz()
{
    return _remote_apis.size();
}

int conf::llm_model_sz()
{
    return _models.size();
}

bool conf::llm_is_local_model(const std::string &model_id)
{
    return _models.find(model_id) != _models.end();
}

bool conf::llm_is_remote_api(const std::string &model_id)
{
    return _remote_apis.find(model_id) != _remote_apis.end();
}

std::string conf::llm_embedding_model()
{
    return _cfg.get<std::string>("llm/embedding_model", "");
}

std::unordered_map<std::string, conf::asr_ctx_config> conf::asr_ctxs()
{
    return _asr_ctxs;
}

std::string conf::asr_file_path()
{
#if defined(__APPLE__) // standard macOS bundle structure
    return hj::filepath::join(hj::filepath::pwd(), "../Resources");
#else
    return hj::filepath::join(hj::filepath::pwd());
#endif
}

std::string conf::asr_vad_model_path()
{
#if defined(__APPLE__) // standard macOS bundle structure
    return hj::filepath::join(hj::filepath::pwd(), "../Resources");
#else
    return hj::filepath::join(hj::filepath::pwd());
#endif
}

int conf::asr_audio_buffer_size()
{
    return _cfg.get<int>("asr/audio_buffer_size", 16000);
}

int conf::asr_audio_min_chunk_size()
{
    return _cfg.get<int>("asr/audio_min_chunk_size", 16000 * 0.2);
}

int conf::asr_audio_wait_chunk_timeout_ms()
{
    return _cfg.get<int>("asr/audio_wait_chunk_timeout_ms", 4500);
}

std::vector<std::string> conf::watch_dog_pub_topics()
{
    return _watch_dog_pub_topics;
}

std::string conf::watch_dog_pub_addr()
{
    return _watch_dog_pub_addr;
}

void conf::_init(const std::string &config_file_path)
{
    // read config file
    if(!_cfg.read_file(config_file_path.c_str()))
        throw std::runtime_error("Failed to read config file: "
                                 + config_file_path);

    // init models
    _models.clear();
    auto fpath_models = _cfg.get<std::string>("llm/models", "");
    fpath_models      = hj::filepath::join(llm_model_file_path(), fpath_models);
    hj::ini models_ini;
    if(models_ini.read_file(fpath_models.c_str()))
    {
        for(const auto &item : models_ini)
        {
            auto         sect = item.second;
            model_config config;
            config.id   = sect.get<std::string>("id", "");
            config.path = sect.get<std::string>("path", "");
            config.path =
                hj::filepath::join(llm_model_file_path(), config.path);
            config.n_gpu_layers = sect.get<int>("n_gpu_layers", -1);
            config.split_mode =
                static_cast<llama_split_mode>(sect.get<int>("split_mode", 1));
            config.main_gpu        = sect.get<int>("main_gpu", 0);
            config.vocab_only      = sect.get<int>("vocab_only", 0) == 1;
            config.use_mmap        = sect.get<int>("use_mmap", 1) == 1;
            config.use_direct_io   = sect.get<int>("use_direct_io", 0) == 1;
            config.use_mlock       = sect.get<int>("use_mlock", 0) == 1;
            config.check_tensors   = sect.get<int>("check_tensors", 0) == 1;
            config.use_extra_bufts = sect.get<int>("use_extra_bufts", 1) == 1;
            config.no_host         = sect.get<int>("no_host", 0) == 1;
            config.no_alloc        = sect.get<int>("no_alloc", 0) == 1;

            if(config.id.empty() || config.path.empty()
               || config.n_gpu_layers < -1 || config.main_gpu < 0)
            {
                std::cerr << "config model id: " << config.id
                          << ", path: " << config.path
                          << ", n_gpu_layers: " << config.n_gpu_layers
                          << ", main_gpu: " << config.main_gpu << " INVALID!!!"
                          << std::endl;
                continue;
            }

            _models[config.id] = config;
        }
    }

    // init remote apis
    _remote_apis.clear();
    auto fpath_remote_apis = _cfg.get<std::string>("llm/remote_apis", "");
    fpath_remote_apis =
        hj::filepath::join(llm_remote_api_file_path(), fpath_remote_apis);
    hj::ini remote_apis_ini;
    if(remote_apis_ini.read_file(fpath_remote_apis.c_str()))
    {
        for(const auto &item : remote_apis_ini)
        {
            auto              sect = item.second;
            remote_api_config config;
            config.id          = sect.get<std::string>("id", "");
            config.type        = sect.get<std::string>("type", "");
            config.api_key     = sect.get<std::string>("api_key", "");
            config.timeout_sec = sect.get<int>("timeout_sec", 5);
            if(config.id.empty() || config.type.empty())
            {
                std::cerr << "config remote api: " << item.first
                          << ", id: " << config.id << ", type: " << config.type
                          << " INVALID!!!" << std::endl;
                continue;
            }

            _remote_apis[config.id] = config;
        }
    }

    // init asr
    _asr_ctxs.clear();
    auto fpath_asr = _cfg.get<std::string>("asr/models", "");
    fpath_asr      = hj::filepath::join(asr_file_path(), fpath_asr);
    hj::ini asr_ini;
    if(asr_ini.read_file(fpath_asr.c_str()))
    {
        for(const auto &item : asr_ini)
        {
            auto           sect = item.second;
            asr_ctx_config config;
            config.id      = sect.get<std::string>("id", "");
            config.path    = sect.get<std::string>("path", "");
            config.path    = hj::filepath::join(asr_file_path(), config.path);
            config.use_gpu = (sect.get<int>("use_gpu", 0) == 1);
            config.gpu_device = sect.get<int>("gpu_device", -1);

            _asr_ctxs[config.id] = config;
        }
    }

    // init client
    _clients.clear();
    auto fpath_client = _cfg.get<std::string>("client/clients", "");
    fpath_client      = hj::filepath::join(clients_file_path(), fpath_client);
    hj::ini client_ini;
    if(client_ini.read_file(fpath_client.c_str()))
    {
        for(const auto &item : client_ini)
        {
            auto          sect = item.second;
            client_config config;
            config.platform        = sect.get<std::string>("platform", "");
            config.arch            = sect.get<std::string>("arch", "");
            config.rollout_percent = sect.get<uint32_t>("rollout_percent", 100);
            config.version_major   = sect.get<uint8_t>("min_version_major", 0);
            config.version_minor   = sect.get<uint8_t>("min_version_minor", 0);
            config.version_patch   = sect.get<uint8_t>("min_version_patch", 0);
            if(config.platform.empty() || config.arch.empty())
            {
                std::cerr << "config client: " << item.first
                          << ", platform: " << config.platform
                          << ", arch: " << config.arch << " INVALID!!!"
                          << std::endl;
                continue;
            }

            _clients[item.first] = config;
        }
    }

    // init regex
    auto file_norm_prompt = _cfg.get<std::string>("regex/norm_prompt", "");
    file_norm_prompt = hj::filepath::join(regex_file_path(), file_norm_prompt);
    if(!file_norm_prompt.empty())
    {
        std::ifstream ifs(file_norm_prompt);
        if(ifs.is_open())
        {
            std::stringstream ss;
            ss << ifs.rdbuf();
            _regex_norm_prompt = ss.str();
        }
    }
    auto file_hard_prompt = _cfg.get<std::string>("regex/hard_prompt", "");
    file_hard_prompt = hj::filepath::join(regex_file_path(), file_hard_prompt);
    if(!file_hard_prompt.empty())
    {
        std::ifstream ifs(file_hard_prompt);
        if(ifs.is_open())
        {
            std::stringstream ss;
            ss << ifs.rdbuf();
            _regex_hard_prompt = ss.str();
        }
    }

    // init watch dog
    auto topics           = _cfg.get<std::string>("watch_dog/topics", "");
    _watch_dog_pub_topics = hj::string_util::split(topics, ",");
    _watch_dog_pub_addr   = _cfg.get<std::string>("watch_dog/addr", "");
}

std::string conf::regex_file_path()
{
#if defined(__APPLE__) // standard macOS bundle structure
    return hj::filepath::join(hj::filepath::pwd(), "../Resources");
#else
    return hj::filepath::join(hj::filepath::pwd());
#endif
}

std::string conf::regex_norm_prompt()
{
    return _regex_norm_prompt;
}

std::string conf::regex_hard_prompt()
{
    return _regex_hard_prompt;
}