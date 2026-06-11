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
    virtual const std::string id()                   = 0;
    virtual int               exec(const char *sql)  = 0;
    virtual int               query(std::vector<std::vector<std::string>> &outs,
                                    const char                            *sql) = 0;
};

#endif