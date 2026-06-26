#ifndef CONF_H
#define CONF_H

#include <vector>
#include <string>

#include <hj/encoding/ini.hpp>
#include <hj/util/license.hpp>

class conf
{
  public:
    struct model_config
    {
        std::string id;
        std::string path;
        int         n_gpu_layers;
        int         max_repeats;
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
    int llm_model_max_repeats(const std::string &model_id);
    int llm_ctx_window_sz();
    int llm_num_threads();

  private:
    void _init();

  private:
    hj::ini _cfg;

    std::unordered_map<std::string, conf::model_config> _models;
};

#endif