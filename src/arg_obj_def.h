#pragma once

#include <stdbool.h>
#include <stddef.h>
#include "cc_shell.h"

struct arg_obj {
  size_t size;
  size_t capacity;
  char **args;
  char *input;
  char *curr_char;
  redir_t redir_type;
  int pipe_amt;
};
