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

int llm_mgr::load(const std::string               &model_id,
                  const std::string               &model_path,
                  const hj::llama::model_params_t &model_config)
{
    if(_llms.find(model_id) != _llms.end())
    {
        LOG_ERROR("Model {} already loaded, skip", model_id);
        return LLM_ERR_MODEL_ALREADY_LOADED;
    }

    auto model =
        std::make_unique<hj::llama::model>(model_path.c_str(), model_config);
    if(model->data() == nullptr)
    {
        LOG_ERROR("Failed to load model {} from file {}", model_id, model_path);
        return LLM_ERR_MODEL_LOAD_FAIL;
    }

    _llms[model_id] = std::move(model);
    LOG_INFO("Loaded model {} from file {} with {} GPU layers",
             model_id,
             model_path,
             model_config.n_gpu_layers);
    return OK;
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

hj::llama::context_params_t llm_mgr::create_ctx_params()
{
    auto params = hj::llama::context::default_params();
    return params;
}

hj::llama::model_params_t llm_mgr::create_model_params()
{
    auto params = hj::llama::model::default_params();
    return params;
}

int llm_mgr::loop_query(
    const std::string                              &model_id,
    std::vector<hj::llama::token_t>                &tokens,
    const hj::llama::context_params_t              &ctx_params,
    const hj::llama::sampler::params               &smpl_params,
    const std::function<bool(std::string &output)> &callback)
{
    if(tokens.size() <= 0)
    {
        LOG_WARN("Invalid tokens");
        return OK;
    }

    // Load model
    if(_llms.find(model_id) == _llms.end())
    {
        LOG_ERROR("Model {} not found", model_id);
        return LLM_ERR_MODEL_NOT_EXIST;
    }

    hj::llama::model *model = _llms.find(model_id)->second.get();
    if(model->data() == nullptr)
    {
        LOG_ERROR("Model {} data is null", model_id);
        return LLM_ERR_MODEL_NOT_EXIST;
    }

    // Create context
    hj::llama::context ctx{model, ctx_params};
    if(ctx.data() == nullptr)
    {
        LOG_ERROR("Failed to create context for model {}", model_id);
        return LLM_ERR_MODEL_CREATE_CTX_FAIL;
    }

    // Create sampler
    hj::llama::sampler sampler{hj::llama::sampler::default_chain_params(),
                               smpl_params};

    // Prefill
    hj::llama::batch pre_batch{tokens};
    pre_batch.set_logits(static_cast<int32_t>(tokens.size()) - 1, true);

    // Clear KV cache and evaluate prompt
    // auto batch = hj::llama::batch_get_one(tokens);
    if(ctx.decode(pre_batch) != 0)
    {
        LOG_ERROR("Decode failed! tokens.size: {}", tokens.size());
        return LLM_ERR_MODEL_CTX_DECODE_FAIL;
    }

    // llama_batch loop_batch = llama_batch_init(1, 0, 1);
    hj::llama::batch loop_batch{1, 0, 1};
    int32_t          pos = static_cast<int32_t>(tokens.size());

    // Reset sampler state (clears token history)
    sampler.reset();

    // loop query
    LOG_DEBUG("llm loop query start");
    std::string pieces;
    do
    {
        // Sample the next token
        int32_t sample_idx =
            (pos == static_cast<int32_t>(tokens.size())) ? (pos - 1) : 0;
        auto next_token = sampler.sample(ctx, sample_idx);
        if(model->token_is_eog(next_token))
        {
            LOG_DEBUG("End of Generation token encountered, stopping query");
            break;
        }

        // Convert token to piece/string and print it
        char buf[LLM_TOKEN_PIECE_BUF_SZ];
        int  n_chars =
            model->token_to_piece(next_token, buf, sizeof(buf), 0, false);
        pieces = std::string(buf, n_chars);

        // Accept the token (updates sampler state)
        sampler.accept(next_token);

        // Prepare the next token for the next iteration
        loop_batch.set_tokens(&next_token, 1, pos);
        loop_batch.set_logits(0, true);
        if(ctx.decode(loop_batch) != 0)
        {
            LOG_ERROR("Decode batch fail");
            break;
        }

        pos++;
    } while(callback(pieces));

    LOG_DEBUG("llm loop query end");
    return OK;
}