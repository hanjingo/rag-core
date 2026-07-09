#ifndef GLOBAL_H
#define GLOBAL_H

#include <string>
#include <vector>

static constexpr const char *CORE_CONFIG_FILE = "core.ini";

// time format: "2023-08-01 12:34:56"
static constexpr const char *TIME_FORMAT = "%Y-%m-%d %H:%M:%S";

// "model:qwen;slice_sz:100;"" -> ["model", "qwen", "slice_sz", "100"]
static constexpr const char *PATTERN_SET_CMD_PARAM = "[:;]+";

// "model;slice_sz" -> ["model", "slice_sz"]
static constexpr const char *PATTERN_GET_CMD_PARAM = "[;]+";

// DB Interface
class db
{
  public:
    virtual const std::string id()                  = 0;
    virtual int               exec(const char *sql) = 0;
    virtual int               query(std::vector<std::vector<std::string>> &outs,
                                    const char                            *sql) = 0;
    virtual int64_t           last_insert_id(const char *table) = 0;
};

// const params
static constexpr const char *DB_SQLITE = "sqlite";

static constexpr const char *ROLE_USER      = "user";
static constexpr const char *ROLE_ASSISTANT = "assistant";
static constexpr const char *ROLE_SYSTEM    = "system";

static constexpr int64_t NONE_MSG_ID = 0;

// SQL TEMPLATE
static constexpr const char *SQL_SELECT_USER_ID_BY_USERNAME_PASSWD =
    R"(SELECT id FROM user WHERE username = %Q AND encrypted_passwd = %Q)";

static constexpr const char *SQL_SELECT_USER_BY_USERNAME_PASSWD =
    R"(SELECT id, username, encrypted_passwd, privilege FROM user WHERE username = %Q AND encrypted_passwd = %Q)";

static constexpr const char *SQL_INSERT_USER =
    R"(INSERT INTO user (id, username, encrypted_passwd, privilege) VALUES (%lld, %Q, %Q, %d))";

static constexpr const char *SQL_SELECT_MESSAGE_BY_ID =
    R"(SELECT id, session_id, role, content, prev_message_id, timestamp FROM message WHERE id = %lld ORDER BY timestamp DESC)";

static constexpr const char *SQL_SELECT_MESSAGE_BY_SESSION_ID =
    R"(SELECT id, session_id, role, content, prev_message_id, timestamp FROM message WHERE session_id = %lld ORDER BY timestamp DESC)";

static constexpr const char *SQL_INSERT_MESSAGE =
    R"(INSERT INTO message (id, session_id, role, content, prev_message_id, timestamp) VALUES (%lld, %lld, %Q, %Q, %lld, %lld))";

static constexpr const char *SQL_DELETE_MESSAGE_BY_SESSION_ID =
    R"(DELETE FROM message WHERE session_id = %lld)";

static constexpr const char *SQL_SELECT_SESSION_BY_ID =
    R"(SELECT id, user_id, title, timestamp FROM session WHERE id = %lld)";

static constexpr const char *SQL_SELECT_SESSION_BY_USER_ID =
    R"(SELECT id, user_id, title, timestamp FROM session WHERE user_id = %lld)";

static constexpr const char *SQL_INSERT_SESSION =
    R"(INSERT INTO session (id, user_id, title, timestamp) VALUES (%lld, %lld, %Q, %lld))";

static constexpr const char *SQL_UPDATE_SESSION_TITLE_BY_ID =
    R"(UPDATE session SET title = %Q WHERE id = %lld)";

static constexpr const char *SQL_DELETE_SESSION_BY_ID =
    R"(DELETE FROM session WHERE id = %lld)";

static constexpr const char *SQL_SELECT_SKILL_INFO =
    R"(SELECT hash, platform, name, desc, publisher, version, timestamp FROM skill)";

static constexpr const char *SQL_SELECT_SKILL_INFO_BY_HASH =
    R"(SELECT hash, platform, name, desc, publisher, version, timestamp FROM skill WHERE hash = %Q)";

static constexpr const char *SQL_SELECT_FILE_BY_HASH =
    R"(SELECT addr, owner, size_kb FROM file WHERE hash = %Q)";


// Pipeline
static constexpr const char *PIPELINE_LOCAL = "local";

static constexpr const char *PIPELINE_REMOTE = "remote";

static constexpr const char *PIPELINE_HYBRID = "hybrid";

// Prompt class
static constexpr const char *PROMPT_TYPE_UNKNOWN = "unknown";

static constexpr const char *PROMPT_TYPE_CODE = "code";

static constexpr const char *PROMPT_TYPE_ALGO = "algo";

static constexpr const char *PROMPT_TYPE_MATH = "math";

static constexpr const char *PROMPT_TYPE_CHAT = "chat";

// caller type
static constexpr const char *CALLER_TYPE_DEEPSEEK = "deepseek";

#endif