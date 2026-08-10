#ifndef CONF_H
#define CONF_H

#include <atomic>
#include <vector>
#include <string>
#include <unordered_set>
#include <unordered_map>

#include <hj/encoding/ini.hpp>
#include <hj/util/license.hpp>
#include <hj/ai/llama.hpp>

class conf
{
  public:
    struct client_config
    {
        std::string platform;
        std::string arch;
        uint32_t    rollout_percent;
        uint8_t     version_major;
        uint8_t     version_minor;
        uint8_t     version_patch;
    };

    struct remote_api_config
    {
        std::string id;
        std::string type;
        std::string api_key;
        int         timeout_sec;
    };

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

    void    init(const std::string &config_file_path);
    hj::ini data();

    int         log_min_lvl();
    int         log_flush_on();
    std::string log_filename();
    int         log_max_size();
    int         log_max_files();

    size_t        sync_write_queue_size();
    unsigned long sync_thread_pool_size();

    std::string server_addr();
    std::string publish_addr();

    std::unordered_map<std::string, conf::client_config> clients();
    std::string                                          clients_file_path();

    std::string sqlite_id();
    std::string sqlite_path();
    int         sqlite_pool();
    int         sqlite_msg_limit();


    std::string              issuer_id();
    hj::license::sign_algo   issuer_algo();
    std::vector<std::string> issuer_keys();
    int                      issuer_valid_times();
    int                      issuer_expired_days();

    std::string              verifier_id();
    hj::license::sign_algo   verifier_algo();
    std::vector<std::string> verifier_keys();

    std::unordered_map<std::string, remote_api_config> llm_remote_apis();
    std::unordered_map<std::string, model_config>      llm_models();
    std::string                                        llm_model_file_path();
    std::string llm_remote_api_file_path();
    int         llm_remote_api_sz();
    int         llm_model_sz();
    bool        llm_is_local_model(const std::string &model_id);
    bool        llm_is_remote_api(const std::string &model_id);
    int         llm_max_repeats();
    int         llm_ctx_window_sz();
    int         llm_num_threads();
    int         llm_local_prompt_threshold();
    std::string llm_embedding_model();

    std::string                                     asr_file_path();
    std::string                                     asr_vad_model_path();
    std::unordered_map<std::string, asr_ctx_config> asr_ctxs();
    int                                             asr_audio_buffer_size();
    int                                             asr_audio_min_chunk_size();
    int asr_audio_wait_chunk_timeout_ms();

    std::string regex_file_path();
    std::string regex_norm_prompt();
    std::string regex_hard_prompt();

    std::string              watch_dog_pub_addr();
    std::vector<std::string> watch_dog_pub_topics();

  private:
    void _init(const std::string &config_file_path);

  private:
    std::atomic<bool> _inited{false};
    hj::ini           _cfg;

    std::unordered_map<std::string, remote_api_config>    _remote_apis;
    std::unordered_map<std::string, conf::model_config>   _models;
    std::unordered_map<std::string, conf::asr_ctx_config> _asr_ctxs;
    std::unordered_map<std::string, conf::client_config>  _clients;

    std::string _regex_norm_prompt;
    std::string _regex_hard_prompt;

    std::vector<std::string> _watch_dog_pub_topics;
    std::string              _watch_dog_pub_addr;
};

#endif