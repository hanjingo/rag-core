#include "llm.h"

#include "err.h"

#include <hj/log/log.hpp>

llm_mgr::llm_mgr()
    : _thread_pool(conf::instance().llm_num_threads())
{
    hj::llama::backend_init();
}

llm_mgr::~llm_mgr()
{
    _thread_pool.clear();
    hj::llama::backend_free();
}

void llm_mgr::load(
    const std::unordered_map<std::string, conf::model_config> &model_configs)
{
    for(auto &kv : model_configs)
    {
        if(_llms.find(kv.first) != _llms.end())
        {
            LOG_ERROR("Model {} already loaded, skip", kv.first);
            continue;
        }

        auto params         = hj::llama::model::default_params();
        params.n_gpu_layers = kv.second.n_gpu_layers;
        auto model =
            std::make_unique<hj::llama::model>(kv.second.path.c_str(), params);
        if(model->data() == nullptr)
        {
            LOG_ERROR("Failed to load model {} from file {}, skip",
                      kv.first,
                      kv.second.path);
            continue;
        }

        _llms[kv.first] = std::move(model);
        LOG_INFO("Loaded model {} from file {} with {} GPU layers",
                 kv.first,
                 kv.second.path,
                 kv.second.n_gpu_layers);
    }
}

std::vector<hj::llama::token_t> llm_mgr::tokenize(const std::string &model,
                                                  const std::string &text,
                                                  const bool add_special,
                                                  const bool parse_special)
{
    auto it = _llms.find(model);
    if(it == _llms.end())
        return {};

    return it->second->tokenize(text, add_special, parse_special);
}

hj::llama::context_params_t
llm_mgr::create_ctx_params(const size_t ctx_window_sz)
{
    auto params  = hj::llama::context::default_params();
    params.n_ctx = ctx_window_sz;
    return params;
}

hj::llama::model_params_t llm_mgr::create_model_params(const int n_gpu_layers)
{
    auto params         = hj::llama::model::default_params();
    params.n_gpu_layers = n_gpu_layers;
    return params;
}

int llm_mgr::loop_query(
    const std::string                              &model_id,
    std::vector<hj::llama::token_t>                &tokens,
    const hj::llama::context_params_t              &ctx_params,
    const std::function<bool(std::string &output)> &callback)
{
    if(_llms.find(model_id) == _llms.end())
        return LLM_ERR_MODEL_NOT_EXIST;

    // create context
    auto model = _llms.find(model_id)->second.get();
    if(model->data() == nullptr)
        return LLM_ERR_MODEL_NOT_EXIST;

    hj::llama::context ctx{model, ctx_params};
    if(ctx.data() == nullptr)
        return LLM_ERR_MODEL_CREATE_CTX_FAIL;

    // import prompt
    auto batch = hj::llama::batch_get_one(tokens);

    // loop query
    LOG_DEBUG("llm loop query start");
    std::string pieces;
    do
    {
        if(ctx.decode(batch) != 0)
            return LLM_ERR_MODEL_CTX_DECODE_FAIL;

        // Sample the next token (Greedy sampling used here for simplicity)
        auto               logits     = ctx.get_logits_ith(batch.n_tokens - 1);
        hj::llama::token_t next_token = 0;
        float              max_logit  = -1e9;

        // Simple argmax sampling over vocabulary
        int n_vocab = model->n_vocab();
        for(int v = 0; v < n_vocab; ++v)
        {
            if(logits[v] > max_logit)
            {
                max_logit  = logits[v];
                next_token = v;
            }
        }

        // Check for End of Generation (EOG/EOS) tokens
        if(model->token_is_eog(next_token))
            break;

        // Convert token to piece/string and print it
        char buf[LLM_TOKEN_PIECE_BUF_SZ];
        int  n_chars =
            model->token_to_piece(next_token, buf, sizeof(buf), 0, false);
        pieces = std::string(buf, n_chars);

        // Prepare the next token for the next iteration
        batch = hj::llama::batch_get_one(&next_token, 1);
    } while(callback(pieces));

    LOG_DEBUG("llm loop query end");
    return OK;
}

// int llm_mgr::loop_query_async(
//     const std::string                             &model_id,
//     std::vector<hj::llama::token_t>               &tokens,
//     const hj::llama::context_params_t             &ctx_params,
//     const std::function<bool(std::string &output)> callback)
// {
//     _thread_pool.enqueue(
//         [model_id, tokens, ctx_params, callback]() mutable -> int {
//             return llm_mgr::instance().loop_query(model_id,
//                                                   tokens,
//                                                   ctx_params,
//                                                   callback);
//         });

//     return OK;
// }