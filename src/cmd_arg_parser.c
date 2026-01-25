#include "cmd_arg_parser.h"
#include "cc_shell.h"
#include <unistd.h>

extern char *input;
extern char *curr_char;

static char **args;
static char curr_arg[BUFF_LENGTH];

Args *create_args_obj() {
  Args *ao = (Args *)malloc(sizeof(Args));
  ao->size = 0;
  ao->capacity = 10;
  ao->args = (char **)calloc(ao->capacity, sizeof(char *));
  if (ao->args == NULL) {
    fprintf(stderr, "calloc failed! (%s: Line %d)\n", __FUNCTION__, __LINE__);
    exit(EXIT_FAILURE);
  }
  ao->redir_type = NO_REDIR;
  return ao;
}

void add_cmd_args(Args *ao) {
  char *received_arg;

  while (*curr_char != '\0') {
    if (strncmp(curr_char, "\'", 1) == 0) {
      received_arg = get_single_quote_arg();
      if (strncmp(curr_char, "\'", 1) == 0) {
        received_arg =
            skip_past_adjacent_quotes_and_combine(received_arg, '\'');
      }
    } else if (strncmp(curr_char, "\"", 1) == 0) {
      received_arg = get_double_quote_arg();
      if (strncmp(curr_char, "\"", 1) == 0) {
        received_arg =
            skip_past_adjacent_quotes_and_combine(received_arg, '\"');
      }
    } else {
      received_arg = get_normal_arg();
      received_arg = check_empty_quoted_arg(received_arg);
    }
    skip_past_spaces();
    /*args[n++] = received_arg;*/
    if (strncmp(received_arg, ">", 1) == 0) {
      ao->redir_type = STD_OUT;
    }
    ao->args[ao->size++] = received_arg;
    curr_arg[0] = '\0';
  }
}

static char *get_normal_arg(void) {
  size_t count = 0;
  while (*curr_char != '\0' && *curr_char != ' ' /*&& *curr_char != '\'' &&
         *curr_char != '\"'*/) {
    if (*curr_char == '\\') {
      curr_arg[count++] = handle_backslash_char(OUTSIDE_QUOTES);
    } else if (empty_single_quotes_in_normal_arg()) {
      curr_char += 2;
    } else if (empty_double_quotes_in_normal_arg()) {
      curr_char += 2;
    }else if (start_of_single_quoted_text()) {
      curr_char++; /* skip past first single quote */
      while (*curr_char != '\'' && *curr_char != '\0') {
        curr_arg[count++] = *curr_char++;
      }
      if (*curr_char == '\'') {
        curr_char++; /* skip past second single quote */
      }
    } else if (start_of_double_quote_text()) {
      curr_char++; /* skip past first double quote */
      while (*curr_char != '\"' && *curr_char != '\0') {
        curr_arg[count++] = *curr_char++;
      }
      if (*curr_char == '\"') {
        curr_char++; /* skip past second double quote */
      }
    } else {
      curr_arg[count++] = *curr_char++;
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

static bool empty_single_quotes_in_normal_arg(void) {
  return strncmp(curr_char, "\'\'", 2) == 0;
}

static bool empty_double_quotes_in_normal_arg(void) {
  return strncmp(curr_char, "\"\"", 2) == 0;
}

static bool start_of_single_quoted_text(void) {
  return strncmp(curr_char, "\'", 1) == 0;
}

static bool start_of_double_quote_text(void) {
  return strncmp(curr_char, "\'", 1) == 0;
}

static void skip_past_spaces(void) {
  while (*curr_char == ' ') {
    curr_char++;
  }
}

static char *get_single_quote_arg(void) {
  curr_char++; /* Skip past first single quote */
  size_t count = 0;
  while (*curr_char != '\0' && *curr_char != '\'') {
    curr_arg[count++] = *curr_char++;
  }
  curr_arg[count] = '\0';
  if (*curr_char == '\'') { /* Skip past second single quote */
    curr_char++;
  }
  char *new_arg = strndup(curr_arg, count);
  if (new_arg == NULL) {
    fprintf(stderr, "strndup failed at line %d in %s\n", (__LINE__)-2,
            __FUNCTION__);
  }
  return new_arg;
}

static char *get_double_quote_arg(void) {
  curr_char++; /* Skip past first double quote */
  size_t count = 0;
  while (*curr_char != '\0' && *curr_char != '\"') {
    if (*curr_char == '\\') {
      curr_arg[count++] = handle_backslash_char(INSIDE_DOUBLE_QUOTES);
    } else {
      curr_arg[count++] = *curr_char++;
    }
  }
  if (*curr_char == '\"') {
    curr_char++;
  }
  /*
   * running the while loop again to collect reset of argument that's adjacent
   * to end of quote. Example below    |
   *                         |
   *                         V
   * echo "test\"insidequotes"example\"
   * */
  while (*curr_char != '\"' && *curr_char != ' ' && *curr_char != '\0') {
    if (*curr_char == '\\') {
      curr_arg[count++] = handle_backslash_char(OUTSIDE_QUOTES);
    } else {
      curr_arg[count++] = *curr_char++;
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

static char *check_empty_quoted_arg(char *first_arg) {
  char *new_arg;
  if (strncmp(curr_char, "\'\'", 2) == 0) {
    curr_char++;
    new_arg = skip_past_adjacent_quotes_and_combine(first_arg, '\'');
    return new_arg;
  }
  if (strncmp(curr_char, "\"\"", 2) == 0) {
    curr_char++;
    new_arg = skip_past_adjacent_quotes_and_combine(first_arg, '\"');
    return new_arg;
  }
  return first_arg;
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

static char handle_backslash_char(BKSLSH_MODE bm) {
  if (*curr_char != '\\') {
    fprintf(stderr, "How the fuck did you get here? %s Line %d\n", __FUNCTION__,
            __LINE__);
    exit(EXIT_FAILURE);
  }
  char possible_return_char;
  switch (bm) {
  case OUTSIDE_QUOTES:
    curr_char++;
    switch (*curr_char) {
    case ' ':
      curr_char++;
      return ' ';
    case '\'':
      curr_char++;
      return '\'';
    case '\"':
      curr_char++;
      return '\"';
    case 'n':
      curr_char++;
      return 'n';
    case '\\':
      curr_char++;
      return '\\';
    default:
      possible_return_char = *curr_char;
      curr_char++;
      return possible_return_char;
    }
  case INSIDE_DOUBLE_QUOTES:
    curr_char++;
    switch (*curr_char) {
    case '\\':
      curr_char++;
      return '\\';
    case '\"':
      curr_char++;
      return '\"';
    default:
      return '\\';
    }
  default:
    fprintf(stderr, "Fuck you. %s Line %d", __FUNCTION__, __LINE__);
    exit(EXIT_FAILURE);
  }
}
