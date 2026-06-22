#ifndef GLOBAL_H
#define GLOBAL_H

#include <string>
#include <vector>

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

// SQL TEMPLATE
static constexpr const char *SQL_SELECT_USER_ID_BY_USERNAME_PASSWD =
    R"(SELECT id FROM user WHERE username = '{}' AND encrypted_passwd = '{}')";

static constexpr const char *SQL_SELECT_USER_BY_USERNAME_PASSWD =
    R"(SELECT id, username, encrypted_passwd, privilege FROM user WHERE username = '{}' AND encrypted_passwd = '{}')";

static constexpr const char *SQL_INSERT_USER =
    R"(INSERT INTO user (id, username, encrypted_passwd, privilege) VALUES ({}, '{}', '{}', {}))";

static constexpr const char *SQL_SELECT_SESSION_BY_ID =
    R"(SELECT id, user_id, title, content, timestamp FROM session WHERE id = {})";

static constexpr const char *SQL_SELECT_SESSION_BY_USER_ID =
    R"(SELECT id, user_id, title, content, timestamp FROM session WHERE user_id = {})";

static constexpr const char *SQL_INSERT_SESSION =
    R"(INSERT INTO session (id, user_id, title, content, timestamp) VALUES ({}, {}, '{}', '{}', '{}'))";

static constexpr const char *SQL_UPDATE_SESSION_TITLE_BY_ID =
    R"(UPDATE session SET title = '{}' WHERE id = {})";

static constexpr const char *SQL_INSERT_MODEL =
    R"(INSERT INTO model (hash, name, publisher, timestamp, addr, capabilities, context_size, cost) VALUES ('{}', '{}', '{}', '{}', '{}', '{}', {}, {}))";

static constexpr const char *SQL_SELECT_MODEL =
    R"(SELECT hash, name, publisher, timestamp, addr, capabilities, context_size, cost FROM model)";

static constexpr const char *SQL_SELECT_MODEL_BY_HASH =
    R"(SELECT hash, name, publisher, timestamp, addr, capabilities, context_size, cost FROM model WHERE hash = '{}')";

static constexpr const char *SQL_SELECT_SKILL_INFO =
    R"(SELECT hash, platform, name, desc, publisher, version, timestamp FROM skill)";

static constexpr const char *SQL_SELECT_SKILL_INFO_BY_HASH =
    R"(SELECT hash, platform, name, desc, publisher, version, timestamp FROM skill WHERE hash = '{}')";

static constexpr const char *SQL_SELECT_FILE_BY_HASH =
    R"(SELECT addr, owner, size_kb FROM file WHERE hash = '{}')";

#endif