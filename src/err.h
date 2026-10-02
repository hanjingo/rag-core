#ifndef ERR_H
#define ERR_H

#include <hj/testing/error.hpp>
#include <hj/testing/error_handler.hpp>
#include <hj/util/init.hpp>

static constexpr int OK = 0;

enum class err : int
{
    FAIL            = -1,
    INVALID_SUBCMD  = 1,
    ARGC_TOO_LESS   = 2,
    ACCOUNT_INVALID = 3,
    STOP_FAIL       = 4,
    QUERY_CANCELLED = 5,

    DB_NOT_EXIST         = 100,
    DB_EXISTED           = 101,
    SQLITE_GET_CONN_FAIL = 102,
    SQLITE_EXEC_FAIL     = 103,
    UNKNOWN_PIPELINE     = 104,
    DB_CONN_POOL_EMPTY   = 105,

    AUTH_ISSUER_EXIST     = 200,
    AUTH_ISSUER_NOT_EXIST = 201,
    AUTH_ISSUE_FAIL       = 202,

    AUTH_VERIFIER_EXIST     = 300,
    AUTH_VERIFIER_NOT_EXIST = 301,
    AUTH_INVALID_LICENSE    = 302,

    LLM_MODEL_NOT_EXIST       = 400,
    LLM_MODEL_LOAD_FAIL       = 401,
    LLM_MODEL_ALREADY_LOADED  = 402,
    LLM_MODEL_TOKENIZE_FAIL   = 403,
    LLM_MODEL_QUERY_FAIL      = 404,
    LLM_MODEL_CREATE_CTX_FAIL = 405,
    LLM_MODEL_CTX_DECODE_FAIL = 406,
    LLM_MODEL_DECODE_FAIL     = 407,

    LLM_REPEAT_TOO_MANY_TIMES        = 500,
    LLM_EMBEDDING_EXTRACT_FAIL       = 501,
    LLM_EMBEDDING_INVALID            = 502,
    LLM_EMBEDDING_DIMENSION_MISMATCH = 503,
    LLM_EMBEDDING_SERIALIZE_FAIL     = 504,

    ASR_CTX_NOT_EXIST      = 600,
    ASR_CTX_LOAD_FAIL      = 601,
    ASR_NO_AUDIO_CHUNK     = 602,
    ASR_CTX_ALREADY_LOADED = 603,

    CALLER_NOT_EXIST            = 701,
    CALLER_REMOTE_RESPONSE_FAIL = 702,

    SUB_UNSUB_FAIL      = 800,
    SUB_NO_ACTIVE_SUBER = 801
};

HJ_REG_ERR_CATEGORY(err, rag_core_category, "rag_core")

inline std::error_condition
rag_core_category::default_error_condition(int ev) const noexcept
{
    return std::error_category::default_error_condition(ev);
}

inline std::string rag_core_category::message(int ev) const
{
    switch(static_cast<err>(ev))
    {
        case err::FAIL:
            return "fail";
        case err::INVALID_SUBCMD:
            return "invalid subcmd";
        case err::ARGC_TOO_LESS:
            return "argc too less";
        case err::ACCOUNT_INVALID:
            return "invalid account";
        case err::STOP_FAIL:
            return "stop fail";
        case err::QUERY_CANCELLED:
            return "query cancelled";

        case err::DB_NOT_EXIST:
            return "database not exist";
        case err::DB_EXISTED:
            return "database already existed";
        case err::SQLITE_GET_CONN_FAIL:
            return "sqlite get connection fail";
        case err::SQLITE_EXEC_FAIL:
            return "sqlite execute fail";
        case err::UNKNOWN_PIPELINE:
            return "unknown pipeline";
        case err::DB_CONN_POOL_EMPTY:
            return "database connection pool empty";

        case err::AUTH_ISSUER_EXIST:
            return "auth issuer exist";
        case err::AUTH_ISSUER_NOT_EXIST:
            return "auth issuer not exist";
        case err::AUTH_ISSUE_FAIL:
            return "auth issue fail";

        case err::AUTH_VERIFIER_EXIST:
            return "auth verifier exist";
        case err::AUTH_VERIFIER_NOT_EXIST:
            return "auth verifier not exist";
        case err::AUTH_INVALID_LICENSE:
            return "auth invalid license";

        case err::LLM_MODEL_NOT_EXIST:
            return "llm model not exist";
        case err::LLM_MODEL_LOAD_FAIL:
            return "llm model load fail";
        case err::LLM_MODEL_ALREADY_LOADED:
            return "llm model already loaded";
        case err::LLM_MODEL_TOKENIZE_FAIL:
            return "llm model tokenize fail";
        case err::LLM_MODEL_QUERY_FAIL:
            return "llm model query fail";
        case err::LLM_MODEL_CREATE_CTX_FAIL:
            return "llm model create context fail";
        case err::LLM_MODEL_CTX_DECODE_FAIL:
            return "llm model context decode fail";
        case err::LLM_MODEL_DECODE_FAIL:
            return "llm model decode fail";

        case err::LLM_REPEAT_TOO_MANY_TIMES:
            return "llm repeat too many times";
        case err::LLM_EMBEDDING_EXTRACT_FAIL:
            return "llm embedding extract fail";
        case err::LLM_EMBEDDING_INVALID:
            return "llm embedding invalid";
        case err::LLM_EMBEDDING_DIMENSION_MISMATCH:
            return "llm embedding dimension mismatch";
        case err::LLM_EMBEDDING_SERIALIZE_FAIL:
            return "llm embedding serialize fail";

        case err::ASR_CTX_NOT_EXIST:
            return "asr context not exist";
        case err::ASR_CTX_LOAD_FAIL:
            return "asr context load fail";
        case err::ASR_NO_AUDIO_CHUNK:
            return "asr no audio chunk";
        case err::ASR_CTX_ALREADY_LOADED:
            return "asr context already loaded";

        case err::CALLER_NOT_EXIST:
            return "caller not exist";
        case err::CALLER_REMOTE_RESPONSE_FAIL:
            return "caller remote response fail";

        case err::SUB_UNSUB_FAIL:
            return "subscription unsubscribe fail";
        case err::SUB_NO_ACTIVE_SUBER:
            return "subscription no active subscriber";

        default:
            return "unknown error";
    }
}

static inline std::error_code error(const int e)
{
    return std::error_code(e, get_rag_core_category());
}

static inline std::error_code error(const err e)
{
    return std::error_code(static_cast<int>(e), get_rag_core_category());
}

using err_t = std::error_code;

#endif