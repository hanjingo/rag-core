#ifndef GLOBAL_H
#define GLOBAL_H

// "model:qwen;slice_sz:100;"" -> ["model", "qwen", "slice_sz", "100"]
static constexpr const char *PATTERN_SET_CMD_PARAM = "[:;]+";

// "model;slice_sz" -> ["model", "slice_sz"]
static constexpr const char *PATTERN_GET_CMD_PARAM = "[;]+";

#endif