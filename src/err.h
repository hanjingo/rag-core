#ifndef ERR_H
#define ERR_H

#include <hj/testing/error.hpp>
#include <hj/testing/error_handler.hpp>
#include <hj/util/init.hpp>

static constexpr int ERR_FAIL           = -1;
static constexpr int OK                 = 0;
static constexpr int ERR_INVALID_SUBCMD = 1;
static constexpr int ERR_ARGC_TOO_LESS  = 2;
static constexpr int ACCOUNT_INVALID    = 3;

static constexpr int ERR_DB_NOT_EXIST         = 100;
static constexpr int ERR_DB_EXISTED           = 101;
static constexpr int ERR_SQLITE_GET_CONN_FAIL = 102;
static constexpr int ERR_SQLITE_EXEC_FAIL     = 103;

static constexpr int AUTH_ERR_ISSUER_EXIST     = 200;
static constexpr int AUTH_ERR_ISSUER_NOT_EXIST = 201;
static constexpr int AUTH_ERR_ISSUE_FAIL       = 202;

static constexpr int AUTH_ERR_VERIFIER_EXIST     = 300;
static constexpr int AUTH_ERR_VERIFIER_NOT_EXIST = 301;
static constexpr int AUTH_ERR_INVALID_LICENSE    = 302;

static constexpr int LLM_ERR_MODEL_NOT_EXIST       = 400;
static constexpr int LLM_ERR_MODEL_LOAD_FAIL       = 401;
static constexpr int LLM_ERR_MODEL_TOKENIZE_FAIL   = 402;
static constexpr int LLM_ERR_MODEL_QUERY_FAIL      = 403;
static constexpr int LLM_ERR_MODEL_CREATE_CTX_FAIL = 404;
static constexpr int LLM_ERR_MODEL_CTX_DECODE_FAIL = 405;

static constexpr int LLM_ERR_REPEAT_TOO_MANY_TIMES = 500;


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