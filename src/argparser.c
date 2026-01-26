#include "argparser.h"
#include "arg_obj_def.h"
#include "cc_shell.h"
#include "cmd_arg_parser.h"
#include <stdlib.h>
#include <string.h>

extern struct arg_obj *ao;

static char *get_normal_arg();
static char *get_single_quote_arg();
static char *get_double_quote_arg();
static char *skip_past_adjacent_quotes_and_combine(char *first_arg,
                                                   char type_of_quote);
static char handle_backslash_char(BKSLSH_MODE bm);
static void set_redir_type(char *arg);

int times_double_quotes_called = 0;
static char curr_arg[BUFF_LENGTH];

struct arg_obj *create_arg_obj() {
  struct arg_obj *created_ao = malloc(sizeof(struct arg_obj));
  created_ao->size = 0;
  created_ao->capacity = 10;
  char **initial_args = (char **)malloc(sizeof(char *) * 10);
  if (initial_args == NULL) {
    fprintf(stderr, "Failed to create arg_obj!");
    exit(EXIT_FAILURE);
  }
  created_ao->args = initial_args;
  created_ao->input = NULL;
  created_ao->redir_type = NO_REDIR;
  created_ao->pipe_amt = 0;
  return created_ao;
};

void expand_arg_obj() {
  ao->capacity = ao->capacity * 2;
  char **new_args = realloc(ao->args, sizeof(char *) * ao->capacity);
  if (new_args == NULL) {
    fprintf(stderr, "Failed to rellocate arg_obj args size!");
    exit(EXIT_FAILURE);
  }
}

void skip_past_spaces() {
  while (*ao->curr_char == ' ') {
    ao->curr_char++;
  }
}

void add_args() {
  char *received_arg;
  while (*ao->curr_char != '\0') {
    /*
     * Check to see if arg_obj has enough space for next arg. If it doesn't,
     * expand the size of arg_obj->args array
     */
    if ((ao->size + 1) >= ao->capacity) {
      expand_arg_obj();
    }
    if (strncmp(ao->curr_char, "|", 1) == 0) {
      ao->pipe_amt = 1;
      received_arg = strdup("|");
      while (*ao->curr_char == '|') {
        ao->curr_char++;
      }
      skip_past_spaces();
      ao->args[ao->size++] = received_arg;
      curr_arg[0] = '\0';
    } else if (strncmp(ao->curr_char, "\'", 1) == 0) {
      received_arg = get_single_quote_arg();
      if (strncmp(ao->curr_char, "\'", 1) == 0) {
        received_arg =
            skip_past_adjacent_quotes_and_combine(received_arg, '\'');
      }
      skip_past_spaces();
      ao->args[ao->size++] = received_arg;
    } else if (strncmp(ao->curr_char, "\"", 1) == 0) {
      received_arg = get_double_quote_arg();
      if (strncmp(ao->curr_char, "\"", 1) == 0) {
        received_arg =
            skip_past_adjacent_quotes_and_combine(received_arg, '\"');
      }
      skip_past_spaces();
      ao->args[ao->size++] = received_arg;
    } else {
      received_arg = get_normal_arg();
      if (strncmp(ao->curr_char, "\'\'", 2) == 0) {
        ao->curr_char++;
        received_arg =
            skip_past_adjacent_quotes_and_combine(received_arg, '\'');
      } else if (strncmp(ao->curr_char, "\"\"", 2) == 0) {
        ao->curr_char++;
        received_arg =
            skip_past_adjacent_quotes_and_combine(received_arg, '\"');
      }
      skip_past_spaces();
      set_redir_type(received_arg);
      ao->args[ao->size++] = received_arg;
      curr_arg[0] = '\0';
    }
  }
}

static void set_redir_type(char *arg) {
  if (strncmp(arg, ">>", 2) == 0 || strncmp(arg, "1>>", 3) == 0) {
    ao->redir_type = APPEND_STD_OUT;
  } else if (strncmp(arg, "2>>", 3) == 0) {
    ao->redir_type = APPEND_STD_ERR;
  } else if (strncmp(arg, ">", 1) == 0 || strncmp(arg, "1>", 2) == 0) {
    ao->redir_type = REDIR_STD_OUT;
  } else if (strncmp(arg, "2>", 2) == 0) {
    ao->redir_type = REDIR_STD_ERR;
  }
}

static char *skip_past_adjacent_quotes_and_combine(char *first_arg,
                                                   char type_of_quote) {
  if (type_of_quote == '\"') {
    char *second_arg = get_double_quote_arg();
    size_t combined_size = strlen(first_arg) + strlen(second_arg) + 1;
    first_arg = (char *)realloc(first_arg, combined_size);
    strcat(first_arg, second_arg);
    return first_arg;
  } else if (type_of_quote == '\'') {
    char *second_arg = get_single_quote_arg();
    size_t combined_size = strlen(first_arg) + strlen(second_arg) + 1;
    first_arg = (char *)realloc(first_arg, combined_size);
    strcat(first_arg, second_arg);
    return first_arg;
  }
  return NULL;
}

static char *get_normal_arg() {
  size_t count = 0;
  while (*ao->curr_char != '\0' && *ao->curr_char != ' ' &&
         *ao->curr_char != '\'' && *ao->curr_char != '\"') {
    if (*ao->curr_char == '\\') {
      curr_arg[count++] = handle_backslash_char(OUTSIDE_QUOTES);
    } else {
      curr_arg[count++] = *ao->curr_char++;
    }
  }
  curr_arg[count] = '\0';
  char *new_arg = strndup(curr_arg, count);
  if (new_arg == NULL) {
    fprintf(stderr, "strndup failed at line %d in %s\n", (__LINE__)-2,
            __FUNCTION__);
  }
  return new_arg;
}

static char *get_single_quote_arg() {
  ao->curr_char++; // Skip past first single quote
  size_t count = 0;
  while (*ao->curr_char != '\0' && *ao->curr_char != '\'') {
    curr_arg[count++] = *ao->curr_char++;
  }
  curr_arg[count] = '\0';
  if (*ao->curr_char == '\'') { // Skip past second single quote
    ao->curr_char++;
  }
  char *new_arg = strndup(curr_arg, count);
  if (new_arg == NULL) {
    fprintf(stderr, "strndup failed at line %d in %s\n", (__LINE__)-2,
            __FUNCTION__);
  }
  return new_arg;
}

static char handle_backslash_char(BKSLSH_MODE bm) {
  if (*ao->curr_char != '\\') {
    fprintf(stderr, "How the fuck did you get here? %s Line %d\n", __FUNCTION__,
            __LINE__);
    exit(EXIT_FAILURE);
  }
  switch (bm) {
  case OUTSIDE_QUOTES:
    ao->curr_char++;
    switch (*ao->curr_char) {
    case ' ':
      ao->curr_char++;
      return ' ';
    case '\'':
      ao->curr_char++;
      return '\'';
    case '\"':
      ao->curr_char++;
      return '\"';
    case 'n':
      ao->curr_char++;
      return 'n';
    case '\\':
      ao->curr_char++;
      return '\\';
    default:
      fprintf(stderr, "Failed switch block in %s\n", __FUNCTION__);
      exit(EXIT_FAILURE);
    }
  case INSIDE_DOUBLE_QUOTES:
    ao->curr_char++;
    switch (*ao->curr_char) {
    case '\\':
      ao->curr_char++;
      return '\\';
    case '\"':
      ao->curr_char++;
      return '\"';
    default:
      return '\\';
    }
  default:
    fprintf(stderr, "Fuck you. %s Line %d", __FUNCTION__, __LINE__);
    exit(EXIT_FAILURE);
  }
}

static char *get_double_quote_arg() {
  ao->curr_char++; // Skip past first double quote
  size_t count = 0;
  while (*ao->curr_char != '\0' && *ao->curr_char != '\"') {
    if (*ao->curr_char == '\\') {
      curr_arg[count++] = handle_backslash_char(INSIDE_DOUBLE_QUOTES);
    } else {
      curr_arg[count++] = *ao->curr_char++;
    }
  }
  if (*ao->curr_char == '\"') {
    ao->curr_char++;
  }
  /*
   * running the while loop again to collect reset of argument that's adjacent
   * to end of quote. Example below    |
   *                         |
   *                         V
   * echo "test\"insidequotes"example\"
   * */
  while (*ao->curr_char != '\"' && *ao->curr_char != ' ' &&
         *ao->curr_char != '\0') {
    if (*ao->curr_char == '\\') {
      curr_arg[count++] = handle_backslash_char(OUTSIDE_QUOTES);
    } else {
      curr_arg[count++] = *ao->curr_char++;
    }
  }
  curr_arg[count] = '\0';
  char *new_arg = strndup(curr_arg, count);
  if (new_arg == NULL) {
    fprintf(stderr, "strndup failed at line %d in %s\n", (__LINE__)-2,
            __FUNCTION__);
  }
  return new_arg;
}

void clear_args() {
  for (size_t i = 0; i < ao->size; i++) {
    free(ao->args[i]);
  }
  ao->size = 0;
  ao->input = NULL;
  ao->curr_char = NULL;
  ao->redir_type = NO_REDIR;
  ao->pipe_amt = 0;
}
