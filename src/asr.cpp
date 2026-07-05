#include "asr.h"

#include <hj/log/logger.hpp>
#include "err.h"

asr_mgr::asr_mgr()
    : _state_pool{}
    , _ctxs{}
{
}

asr_mgr::~asr_mgr()
{
}

hj::asr::ctx_params_t asr_mgr::create_ctx_params()
{
    return hj::asr::context::default_params();
}

hj::asr::full_params_t asr_mgr::create_full_params()
{
    return hj::asr::context::default_full_params();
}

int asr_mgr::load(const std::string           &ctx_id,
                  const std::string           &path,
                  const hj::asr::ctx_params_t &config)
{
    if(_ctxs.find(ctx_id) != _ctxs.end())
    {
        LOG_ERROR("Asr Ctx {} already loaded, skip", ctx_id);
        return ASR_ERR_CTX_NOT_EXIST;
    }

    auto ctx = std::make_unique<hj::asr::context>(path.c_str(), config);
    if(ctx->data() == nullptr)
    {
        LOG_ERROR("Failed to load ctx {} from file {}", ctx_id, path);
        return ASR_ERR_CTX_LOAD_FAIL;
    }

    _ctxs[ctx_id] = std::move(ctx);
    LOG_INFO("Loaded ctx {} from file {} with GPU {}",
             ctx_id,
             path,
             config.gpu_device);
    return OK;
}

int asr_mgr::translate(std::string                  &segment,
                       const std::string            &ctx_id,
                       const std::vector<float>     &data,
                       const hj::asr::full_params_t &params)
{
    if(_ctxs.find(ctx_id) == _ctxs.end())
    {
        LOG_ERROR("Ctx '{}' not found", ctx_id);
        return ASR_ERR_CTX_NOT_EXIST;
    }

    // create state
    auto ctx = _ctxs.find(ctx_id)->second.get();
    if(ctx->data() == nullptr)
    {
        LOG_ERROR("Ctx '{}' data is null", ctx_id);
        return ASR_ERR_CTX_NOT_EXIST;
    }

    // translate
    auto err = ctx->full(params, data);
    if(err != 0)
    {
        LOG_ERROR("ASR full() failed with error:{}", err);
        return err;
    }

    // parse segments
    auto n_segments = ctx->n_segments();
    for(auto i = 0; i < n_segments; ++i)
    {
        std::string tmp;
        ctx->get_segment_text(tmp, i);
        segment += tmp;
        LOG_DEBUG("Parse segment:{}", segment);
    }

    return OK;
}