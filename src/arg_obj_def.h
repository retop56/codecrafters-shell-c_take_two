#pragma once

#include <stdbool.h>
#include <stddef.h>
enum redir_t { STD_OUT, STD_ERR, APPEND_STD_OUT, APPEND_STD_ERR, INITIAL_VAL };
struct arg_obj {
  size_t size;
  size_t capacity;
  char **args;
  char *input;
  char *curr_char;
  enum redir_t redir_type;
};
