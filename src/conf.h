#ifndef CONF_H
#define CONF_H

#include <vector>
#include <string>

#include <hj/encoding/ini.hpp>
#include <hj/util/license.hpp>
#include <hj/ai/llama.hpp>

class conf
{
  public:
    struct model_config
    {
        std::string      id;
        std::string      path;
        int              n_gpu_layers;
        llama_split_mode split_mode;
        int              main_gpu;
        bool             vocab_only;
        bool             use_mmap;
        bool             use_direct_io;
        bool             use_mlock;
        bool             check_tensors;
        bool             use_extra_bufts;
        bool             no_host;
        bool             no_alloc;
    };

    struct asr_ctx_config
    {
        std::string id;
        std::string path;
        bool        use_gpu;
        int         gpu_device;
    };

  public:
    conf();
    ~conf();

    static conf &instance()
    {
        static conf inst;
        return inst;
    }

    hj::ini data();

    int         log_min_lvl();
    int         log_flush_on();
    std::string log_filename();
    int         log_max_size();
    int         log_max_files();

    size_t        sync_write_queue_size();
    unsigned long sync_thread_pool_size();

    std::string server_addr();

    std::string sqlite_id();
    std::string sqlite_path();
    int         sqlite_pool();
    int         sqlite_msg_limit();


    std::string              issuer_id();
    hj::license::sign_algo   issuer_algo();
    std::vector<std::string> issuer_keys();
    int                      issuer_valid_times();
    int                      issuer_leeway();

    std::string              verifier_id();
    hj::license::sign_algo   verifier_algo();
    std::vector<std::string> verifier_keys();

    std::unordered_map<std::string, model_config> llm_models();
    int                                           llm_max_repeats();
    int                                           llm_ctx_window_sz();
    int                                           llm_num_threads();

    std::unordered_map<std::string, asr_ctx_config> asr_ctxs();

  private:
    void _init();

  private:
    hj::ini _cfg;

    std::unordered_map<std::string, conf::model_config>   _models;
    std::unordered_map<std::string, conf::asr_ctx_config> _asr_ctxs;
};

#endif