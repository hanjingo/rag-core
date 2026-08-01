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
        return ASR_ERR_CTX_ALREADY_LOADED;
    }

    auto ctx = std::make_unique<hj::asr::context>(path.c_str(), config);
    if(ctx->data() == nullptr)
    {
        LOG_ERROR("Failed to load ctx {} from file {}", ctx_id, path);
        return ASR_ERR_CTX_LOAD_FAIL;
    }

    _ctxs[ctx_id] = std::move(ctx);
    LOG_INFO("Loaded ctx {} from file {} with GPU enable {}, device {}",
             ctx_id,
             path,
             config.use_gpu,
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
    LOG_DEBUG(">>> start full with param: ctx_id: {}, "
              "n_threads: {}, n_max_text_ctx: {}, offset_ms: {}, duration_ms: "
              "{}, "
              "translate: {}, detect_language: {}, language: {}, no_ctx: {}, "
              "no_timestamps: {}, "
              "single_segment: {}, print_special: {}, print_progress: {}, "
              "print_realtime: {}, "
              "print_timestamps: {}, carry_initial_prompt: {}, initial_prompt: "
              "{}, suppress_regex: {}, "
              "suppress_blank: {}, suppress_nst: {}, temperature: {}, "
              "temperature_inc: {}, "
              "max_initial_ts: {}, length_penalty: {}, entropy_thold: {}, "
              "logprob_thold: {}, "
              "no_speech_thold: {}, vad: {}, vad_model_path: {}, "
              "vad_threshold: {}, vad_min_speech_duration_ms: {}, "
              "vad_min_silence_duration_ms: {}, vad_max_speech_duration_s: {}, "
              "vad_speech_pad_ms: {}, vad_samples_overlap: {}",
              ctx_id,
              params.n_threads,
              params.n_max_text_ctx,
              params.offset_ms,
              params.duration_ms,
              params.translate,
              params.detect_language,
              params.language,
              params.no_context,
              params.no_timestamps,
              params.single_segment,
              params.print_special,
              params.print_progress,
              params.print_realtime,
              params.print_timestamps,
              params.carry_initial_prompt,
              params.initial_prompt,
              params.suppress_regex,
              params.suppress_blank,
              params.suppress_nst,
              params.temperature,
              params.temperature_inc,
              params.max_initial_ts,
              params.length_penalty,
              params.entropy_thold,
              params.no_speech_thold,
              params.vad,
              params.vad_model_path,
              params.vad_params.threshold,
              params.vad_params.min_speech_duration_ms,
              params.vad_params.min_silence_duration_ms,
              params.vad_params.max_speech_duration_s,
              params.vad_params.speech_pad_ms,
              params.vad_params.samples_overlap);
    auto err = ctx->full(params, data);
    if(err != 0)
    {
        LOG_ERROR("ASR full() failed with error:{}", err);
        return err;
    }

    // parse segments
    LOG_DEBUG(">>> start parse segments");
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