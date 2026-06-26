#include "router.h"

#include <algorithm>

#include "llm.h"

void router::route(const std::string &prompt)
{
    std::string lower_prompt = prompt;
    std::transform(lower_prompt.begin(),
                   lower_prompt.end(),
                   lower_prompt.begin(),
                   ::tolower);

    // long prompt for remote, short prompt for local
    if(lower_prompt.find("code") != std::string::npos
       || lower_prompt.find("algorithm") != std::string::npos
       || lower_prompt.length() > 20)
    {
        // send to remote
        return;
    }

    // send to local
    return;
}