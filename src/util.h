#ifndef UTIL_H
#define UTIL_H

#include <vector>
#include <hj/str/str.hpp>

#include "global.h"

static std::vector<std::string> parse_set_param(const std::string &str)
{
    auto ret = hj::str::regex_split(str, PATTERN_SET_CMD_PARAM);
    if(ret.size() % 2 != 0)
        ret.push_back("");
    return ret;
}

static std::vector<std::string> parse_get_param(const std::string &str)
{
    auto ret = hj::str::regex_split(str, PATTERN_GET_CMD_PARAM);
    return ret;
}

#endif