#ifndef ERR_H
#define ERR_H

#include <hj/testing/error.hpp>
#include <hj/testing/error_handler.hpp>
#include <hj/util/init.hpp>

static constexpr int ERR_FAIL           = -1;
static constexpr int OK                 = 0;
static constexpr int ERR_INVALID_SUBCMD = 1;
static constexpr int ERR_ARGC_TOO_LESS  = 2;

static constexpr int ERR_DB_NOT_EXIST         = 100;
static constexpr int ERR_DB_EXISTED           = 101;
static constexpr int ERR_SQLITE_GET_CONN_FAIL = 102;
static constexpr int ERR_SQLITE_EXEC_FAIL     = 103;


static inline std::error_code error(const int   e,
                                    const char *category = "rag-core")
{
    return hj::make_err(e, category);
}

using err_t = std::error_code;

INIT(hj::register_err("rag-core", ERR_FAIL, "rag-core failed");)

INIT(hj::register_err("rag-core", ERR_INVALID_SUBCMD, "invalid subcmd");
     hj::register_err("rag-core", ERR_ARGC_TOO_LESS, "argc too less");)

#endif