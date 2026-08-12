#ifndef GLOBAL_H
#define GLOBAL_H

#include <string>
#include <vector>

// time format: "2023-08-01 12:34:56"
static constexpr const char *TIME_FORMAT = "%Y-%m-%d %H:%M:%S";

// "model:qwen;slice_sz:100;"" -> ["model", "qwen", "slice_sz", "100"]
static constexpr const char *PATTERN_SET_CMD_PARAM = "[:;]+";

// "model;slice_sz" -> ["model", "slice_sz"]
static constexpr const char *PATTERN_GET_CMD_PARAM = "[;]+";

// const params
static constexpr const char *ROLE_USER      = "user";
static constexpr const char *ROLE_ASSISTANT = "assistant";
static constexpr const char *ROLE_SYSTEM    = "system";

static constexpr int64_t NONE_MSG_ID = 0;

// SQL TEMPLATE
static constexpr const char *SQL_SELECT_USER_BY_USERNAME_PASSWD =
    R"(SELECT id, username, encrypted_passwd FROM user WHERE username = ? AND encrypted_passwd = ? LIMIT 1)";

static constexpr const char *SQL_INSERT_USER =
    R"(INSERT INTO user (id, username, encrypted_passwd) VALUES (?, ?, ?))";

static constexpr const char *SQL_SELECT_MESSAGE_BY_ID =
    R"(SELECT id, session_id, role, content, prev_message_id, timestamp FROM message WHERE id = ? ORDER BY timestamp DESC LIMIT 1)";

static constexpr const char *SQL_SELECT_MESSAGE_BY_SESSION_ID =
    R"(SELECT id, session_id, role, content, prev_message_id, timestamp FROM message WHERE session_id = ? ORDER BY timestamp DESC LIMIT ?)";

static constexpr const char *SQL_INSERT_MESSAGE =
    R"(INSERT INTO message (id, session_id, role, content, prev_message_id, timestamp) VALUES (?, ?, ?, ?, ?, ?))";

static constexpr const char *SQL_DELETE_MESSAGE_BY_SESSION_ID =
    R"(DELETE FROM message WHERE session_id = ?)";

static constexpr const char *SQL_SELECT_SESSION_BY_ID =
    R"(SELECT id, user_id, title, timestamp FROM session WHERE id = ? LIMIT ?)";

static constexpr const char *SQL_SELECT_SESSION_BY_USER_ID =
    R"(SELECT id, user_id, title, timestamp FROM session WHERE user_id = ? LIMIT ?)";

static constexpr const char *SQL_INSERT_SESSION =
    R"(INSERT INTO session (id, user_id, title, timestamp) VALUES (?, ?, ?, ?))";

static constexpr const char *SQL_UPDATE_SESSION_TITLE_BY_ID =
    R"(UPDATE session SET title = ? WHERE id = ?)";

static constexpr const char *SQL_DELETE_SESSION_BY_ID =
    R"(DELETE FROM session WHERE id = ?)";

static constexpr const char *SQL_SELECT_PLUGIN_INFO =
    R"(SELECT hash, platform, name, desc, publisher, version, timestamp FROM plugin LIMIT ?)";

static constexpr const char *SQL_SELECT_PLUGIN_INFO_BY_HASH =
    R"(SELECT hash, platform, name, desc, publisher, version, timestamp FROM plugin WHERE hash = ? LIMIT 1)";

static constexpr const char *SQL_SELECT_PLUGIN_INFO_BY_PUBLISHER =
    R"(SELECT hash, platform, name, desc, publisher, version, timestamp FROM plugin WHERE publisher = ? LIMIT ?)";

static constexpr const char *SQL_INSERT_PLUGIN_INFO =
    R"(INSERT INTO plugin (hash, platform, name, desc, publisher, version, timestamp) VALUES (%Q, %d, %Q, %Q, %Q, %Q, %lld))";

static constexpr const char *SQL_SELECT_FILE_BY_HASH =
    R"(SELECT addr, owner, size_kb FROM file WHERE hash = ? LIMIT 1)";

static constexpr const char *SQL_INSERT_FILE =
    R"(INSERT INTO file (hash, addr, owner, size_kb) VALUES (?, ?, ?, ?))";


// Pipeline
static constexpr const char *PIPELINE_LOCAL = "local";

static constexpr const char *PIPELINE_REMOTE = "remote";

static constexpr const char *PIPELINE_HYBRID = "hybrid";

// Prompt class
static constexpr const char *PROMPT_TYPE_UNKNOWN = "unknown";

static constexpr const char *PROMPT_TYPE_HARD = "hard";

static constexpr const char *PROMPT_TYPE_NORM = "norm";

// caller type
static constexpr const char *CALLER_TYPE_DEEPSEEK = "deepseek";

// pub-sub
static constexpr const char *INPROC_PUB_SUB_ADDR = "inproc://rag-qt-publish";

static constexpr const char *TOPIC_SEPARATOR  = "|";
static constexpr const char *TOPIC_RAG_CORE   = "topic-rag-core";
static constexpr const char *TOPIC_PLUGIN_PUB = "topic-plugin-pub";

// DB
static constexpr uint32_t DB_SQLITE1 = 0;

#endif