#pragma once

typedef enum Backslash_Mode {
  INSIDE_DOUBLE_QUOTES,
  OUTSIDE_QUOTES
} BKSLSH_MODE;

typedef enum {
  STD_OUT,
  STD_ERR,
  APPEND_STD_OUT,
  APPEND_STD_ERR,
  NO_REDIR
} redir_t;

typedef struct ao {
  char **args;
  int size;
  int capacity;
  redir_t redir_type;
} Args;
