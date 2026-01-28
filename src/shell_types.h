#pragma once
#include <stdbool.h>
typedef enum { COMMAND, EXEC, EXEC_LTP, NONE } Multiple_T;

typedef enum Backslash_Mode {
  INSIDE_DOUBLE_QUOTES,
  OUTSIDE_QUOTES
} BKSLSH_MODE;

typedef enum {
  REDIR_STD_OUT,
  REDIR_STD_ERR,
  APPEND_STD_OUT,
  APPEND_STD_ERR,
  NO_REDIR
} redir_t;

typedef struct ao {
  char **args;
  int size;
  int capacity;
  redir_t redir_type;
  bool contains_pipe;
} Args;
