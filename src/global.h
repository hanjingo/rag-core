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
static constexpr const char *SQL_SELECT_BY_ID =
    "SELECT id, user_id, title, content, timestamp, vector_index FROM session "
    "WHERE id = {}";

static constexpr const char *SQL_SELECT_BY_USER_ID =
    "SELECT id, user_id, title, content, timestamp, vector_index FROM session "
    "WHERE user_id = {}";

static constexpr const char *SQL_INSERT_SESSION =
    "INSERT INTO session (user_id, title, content, timestamp, vector_index) "
    "VALUES "
    "({}, '{}', '{}', '{}', {})";

static constexpr const char *SQL_UPDATE_SESSION_TITLE_BY_ID =
    "UPDATE session SET title = '{}' WHERE id = {}";

#endif