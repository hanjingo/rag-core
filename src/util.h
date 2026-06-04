#ifndef UTIL_H
#define UTIL_H

#include <vector>
#include <hj/util/string_util.hpp>

#include "global.h"

static std::vector<std::string> parse_set_param(const std::string &str)
{
    auto ret = hj::string_util::split_regex(str, PATTERN_SET_CMD_PARAM);
    if(ret.size() % 2 != 0)
        ret.push_back("");
    return ret;
}

static std::vector<std::string> parse_get_param(const std::string &str)
{
    auto ret = hj::string_util::split_regex(str, PATTERN_GET_CMD_PARAM);
    return ret;
}

#endif