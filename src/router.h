#ifndef ROUTER_H
#define ROUTER_H

#include <string>
#include <regex>

#include <hj/log/logger.hpp>
#include <hj/encoding/unicode.hpp>

#include "conf.h"

class router
{
  public:
    router()
    {
        LOG_DEBUG("classify with norm_regex:{}, hard_regex:{}",
                  conf::instance().regex_norm_prompt(),
                  conf::instance().regex_hard_prompt());

        auto wnorm =
            hj::unicode::from_utf8(conf::instance().regex_norm_prompt());
        auto whard =
            hj::unicode::from_utf8(conf::instance().regex_hard_prompt());

        _norm_pattern =
            std::wregex(wnorm, std::regex::icase | std::regex::optimize);
        _hard_pattern =
            std::wregex(whard, std::regex::icase | std::regex::optimize);
    }
    ~router() {}

    router(const router &)            = delete;
    router &operator=(const router &) = delete;
    router(router &&)                 = delete;
    router &operator=(router &&)      = delete;

    static router &instance()
    {
        static router inst;
        return inst;
    }

    virtual void
    route(std::string &pipeline, std::string &model, const std::string &prompt);

    virtual std::string classify(const std::string &prompt);

    virtual void select(std::string &model, const std::string &pipeline);

  private:
    std::wregex _norm_pattern;
    std::wregex _hard_pattern;
};

#endif // ROUTER_H