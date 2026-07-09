#ifndef LLM_H
#define LLM_H

#include <unordered_map>
#include <vector>
#include <memory>

#include <hj/ai/llama.hpp>
#include <hj/sync/thread_pool.hpp>

#include "conf.h"

#ifndef LLM_TOKEN_PIECE_BUF_SZ
#define LLM_TOKEN_PIECE_BUF_SZ 4096
#endif

#define LLM_CONTINUE true
#define LLM_STOP false

class llm_mgr
{
  public:
    llm_mgr();
    ~llm_mgr();

    static llm_mgr &instance()
    {
        static llm_mgr inst;
        return inst;
    }

    static hj::llama::model_params_t default_model_params()
    {
        hj::llama::model_params_t params = hj::llama::model::default_params();
        return params;
    }

    int load(const std::string               &model_id,
             const std::string               &model_path,
             const hj::llama::model_params_t &model_config);

    std::vector<hj::llama::token_t> tokenize(const std::string &model,
                                             const std::string &text,
                                             const bool         add_special,
                                             const bool         parse_special);

    hj::llama::context_params_t create_ctx_params();

    hj::llama::model_params_t create_model_params();

    int loop_query(const std::string                              &model_id,
                   std::vector<hj::llama::token_t>                &tokens,
                   const hj::llama::context_params_t              &ctx_params,
                   const hj::llama::sampler::params               &smpl_params,
                   const std::function<bool(std::string &output)> &callback);

  private:
    std::unordered_map<std::string, std::unique_ptr<hj::llama::model>> _llms;
};

#endif