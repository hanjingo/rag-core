#ifndef LLM_H
#define LLM_H

#include <unordered_map>
#include <vector>
#include <memory>

#include <hj/ai/llama.hpp>
#include <hj/sync/thread_pool.hpp>

#ifndef LLM_TOKEN_PIECE_BUF_SZ
#define LLM_TOKEN_PIECE_BUF_SZ 128
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

    void load(const std::unordered_map<std::string, std::string> &model_paths);

    std::vector<hj::llama::token_t> tokenize(const std::string &model,
                                             const std::string &text,
                                             const bool         add_special,
                                             const bool         parse_special);

    hj::llama::context_params_t create_ctx_params(const size_t ctx_window_sz);

    int loop_query(const std::string                               &model_id,
                   std::vector<hj::llama::token_t>                 &tokens,
                   const hj::llama::context_params_t               &ctx_params,
                   const std::function<bool(std::string &peieces)> &callback);

    // int
    // loop_query_async(const std::string                             &model_id,
    //                  std::vector<hj::llama::token_t>               &tokens,
    //                  const hj::llama::context_params_t             &ctx_params,
    //                  const std::function<bool(std::string &output)> callback);

  private:
    hj::thread_pool _thread_pool;
    std::unordered_map<std::string, std::unique_ptr<hj::llama::model>> _llms;
};

#endif