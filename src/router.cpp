#include "router.h"

#include <regex>
#include <algorithm>

#include <hj/log/logger.hpp>

#include "llm.h"
#include "conf.h"
#include "global.h"

void router::route(std::string       &pipeline,
                   std::string       &model,
                   const std::string &prompt)
{
    LOG_DEBUG("route entry with pipeline:{}, model:{}, prompt:{}",
              pipeline,
              model,
              prompt);
    if(pipeline == PIPELINE_LOCAL || pipeline == PIPELINE_REMOTE)
        return;

    // no remote api
    if(conf::instance().llm_remote_api_sz() == 0)
    {
        pipeline = PIPELINE_LOCAL;
        select(model, pipeline);
        LOG_DEBUG("no remote api, use pipeline:{},  model:{}", pipeline, model);
        return;
    }

    // no local model
    if(conf::instance().llm_model_sz() == 0)
    {
        pipeline = PIPELINE_REMOTE;
        select(model, pipeline);
        LOG_DEBUG("no local model, use pipeline:{},  model:{}",
                  pipeline,
                  model);
        return;
    }

    // long prompt for remote, short prompt for local
    LOG_DEBUG("hybrid mode check prompt.size():{}", prompt.size());
    if(prompt.size() > conf::instance().llm_local_prompt_threshold())
    {
        pipeline = PIPELINE_REMOTE;
        select(model, pipeline);
        LOG_DEBUG("prompt too long, route to remote api:{}", model);
        return;
    }

    // useage include: [code, algorithm, unknown] -> remote
    auto typ = classify(prompt);
    LOG_DEBUG("route typ:{} with prompt:{}", typ, prompt);
    if(typ != PROMPT_TYPE_NORM)
    {
        pipeline = PIPELINE_REMOTE;
        select(model, pipeline);
        LOG_DEBUG("prompt too hard, route to remote");
        return;
    }

    // send to local
    LOG_DEBUG("prompt route to local");
    pipeline = PIPELINE_LOCAL;
    select(model, pipeline);
    return;
}

std::string router::classify(const std::string &prompt)
{
    // TODO:May be we should use a llm to classify the prompt.
    auto wprompt = hj::unicode::from_utf8(prompt);
    if(prompt.empty() || wprompt.empty())
        return PROMPT_TYPE_UNKNOWN;

    std::wsmatch hard_match;
    if(std::regex_search(wprompt, hard_match, _hard_pattern))
        return PROMPT_TYPE_HARD;

    std::wsmatch norm_match;
    if(std::regex_search(wprompt, norm_match, _norm_pattern))
        return PROMPT_TYPE_NORM;

    return PROMPT_TYPE_UNKNOWN;
}

void router::select(std::string &model, const std::string &pipeline)
{
    if(pipeline == PIPELINE_LOCAL)
    {
        if(conf::instance().llm_is_local_model(model))
            return;

        auto models = conf::instance().llm_models();
        for(auto item : models)
        {
            model = item.first;
            break;
        }
        return;
    }

    if(pipeline == PIPELINE_REMOTE)
    {
        if(conf::instance().llm_is_remote_api(model))
            return;

        auto remote_apis = conf::instance().llm_remote_apis();
        for(auto item : remote_apis)
        {
            model = item.first;
            break;
        }
        return;
    }
}