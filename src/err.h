#ifndef ERR_H
#define ERR_H

#include <hj/testing/error.hpp>
#include <hj/testing/error_handler.hpp>
#include <hj/util/init.hpp>

static inline std::error_code error(const int   e,
                                    const char *category = "rag-core")
{
    return hj::make_err(e, category);
}

#define ERR_FAIL -1
#define OK 0
#define ERR_INVALID_SUBCMD 1
#define ERR_ARGC_TOO_LESS 2

using err_t = std::error_code;

INIT(hj::register_err("rag-core", ERR_FAIL, "rag-core failed");)

INIT(hj::register_err("rag-core", ERR_INVALID_SUBCMD, "invalid subcmd");
     hj::register_err("rag-core", ERR_ARGC_TOO_LESS, "argc too less");)

#endif