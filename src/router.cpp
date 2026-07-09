#include "router.h"

#include <algorithm>
#include <hj/log/logger.hpp>

#include "llm.h"
#include "conf.h"
#include "global.h"

void router::route(std::string &pipeline, const std::string &prompt)
{
    if(pipeline == PIPELINE_LOCAL || pipeline == PIPELINE_REMOTE)
        return;

    // hybrid route
    // long prompt for remote, short prompt for local
    if(prompt.size() > conf::instance().llm_local_prompt_threshold())
    {
        LOG_DEBUG("prompt too long, route to remote");
        // send to remote
        pipeline = PIPELINE_REMOTE;
        return;
    }

    // useage include: [code, algorithm, unknown] -> remote
    auto hard_class = conf::instance().llm_hard_prompt_class();
    if(hard_class.find(classify(prompt)) != hard_class.end())
    {
        LOG_DEBUG("prompt too hard, route to remote");
        // send to remote
        pipeline = PIPELINE_REMOTE;
        return;
    }

    // send to local
    LOG_DEBUG("prompt route to local");
    pipeline = PIPELINE_LOCAL;
    return;
}

std::string router::classify(const std::string &prompt)
{
    // TODO:May be we should use a llm to classify the prompt.

    std::string lower_prompt = prompt;
    std::transform(lower_prompt.begin(),
                   lower_prompt.end(),
                   lower_prompt.begin(),
                   ::tolower);

    if(lower_prompt.find("code") != std::string::npos
       || lower_prompt.find("program") != std::string::npos)
        return PROMPT_TYPE_CODE;

    if(lower_prompt.find("algorithm") != std::string::npos
       || lower_prompt.find("algo") != std::string::npos)
        return PROMPT_TYPE_ALGO;

    if(lower_prompt.find("math") != std::string::npos
       || lower_prompt.find("function") != std::string::npos
       || lower_prompt.find("equation") != std::string::npos)
        return PROMPT_TYPE_MATH;

    if(lower_prompt.find("hello") != std::string::npos
       || lower_prompt.find("story") != std::string::npos
       || lower_prompt.find("tell") != std::string::npos)
        return PROMPT_TYPE_CHAT;

    return PROMPT_TYPE_UNKNOWN;
}