#include "arg_obj_def.h"
#include "argparser.h"
#include "handle_commands.h"
#include <readline/history.h>
#include <readline/readline.h>
#include <stdlib.h>
#include <time.h>
#include <unistd.h>
#include "cc_shell.h"
struct arg_obj *ao;

char *possible_completion_options[] = {"echo", "exit", NULL};
char *built_in_generator(const char *text, int state);
char *executable_name_generator(const char *text, int state);
char **completion_func(const char *text, int start, int end);
void add_partial_completion(char *text_to_insert, bool add_space);

typedef enum { COMMAND, EXEC, EXEC_LTP, NONE } Multiple_T;

static Multiple_T found_multiple = NONE;
static char **matches;
static int num_of_matches = 0;
static int ltp_ind = 0;

char *built_in_generator(const char *text, int state) {
  static int list_index, len;
  char *name;

  if (!state) {
    list_index = 0;
    len = strlen(text);
  }

  while ((name = possible_completion_options[list_index++])) {
    if (strncmp(name, text, len) == 0) {
      return strdup(name);
    }
  }

  return NULL;
}

char *executable_name_generator(const char *text, int state) {
  static char *possible_execs[1024];
  static int index = 0;

  char *path_env_var, *curr_path;
  if (state == 0) {
    int i = 0;
    for (int x = 0; x < 1024; x++) {
      possible_execs[x] = NULL;
    }
    char *full_path = (char *)malloc(PATH_MAX * sizeof(char));
    // Create copy of path_env
    path_env_var = strdup(getenv("PATH"));
    if (path_env_var == NULL) {
      perror("executable_name_generator");
      exit(EXIT_FAILURE);
    }
    // Get one path at a time and open that directory
    curr_path = strtok(path_env_var, ":");
    DIR *dirp;
    while (curr_path != NULL) {
      dirp = opendir(curr_path);
      if (dirp == NULL) {
        curr_path = strtok(NULL, ":");
        continue;
      }
      struct dirent *pDirent;
      int len_text = strlen(text);
      // Loop through every entry in directory
      while ((pDirent = readdir(dirp)) != NULL) {
        if (strncmp(text, pDirent->d_name, len_text) != 0) {
          // Didn't find matching prefix, try next dirent
          continue;
        }
        // Found matching file, now need to check if it's executable
        sprintf(full_path, "%s/%s", curr_path, pDirent->d_name);
        if ((access(full_path, X_OK)) != 0) {
          // It's not executable, try next dirent;
          continue;
        }
        // It's executable, add it to the array!
        possible_execs[i++] = strdup(pDirent->d_name);
      }
      closedir(dirp);
      curr_path = strtok(NULL, ":");
    }
    free(path_env_var);
    free(full_path);
  }
  while (possible_execs[index] != NULL) {
    return strdup(possible_execs[index++]);
  }

  return NULL;
}

int match_sort(const void *a, const void *b) {
  return strcmp(*(const char **)a, *(const char **)b);
}

char **multiple_matches() {
  char **next_ltp_comp;
  int len_of_buff;
  switch (found_multiple) {
  case COMMAND:
    found_multiple = NONE;
    num_of_matches = 0;
    return matches;
  case EXEC:
    /*
     * Check if there are commands with prefix of previous
     * Ex. xyz_             is full prefix of
     *     xyz_foo          which is full prefix of
     *     xyz_foo_bar_     which is full prefix of
     *     xyz_foo_bar_baz
     */
    qsort(matches, num_of_matches, sizeof(char *), match_sort);
    found_multiple = NONE;
    num_of_matches = 0;
    return matches;
    break;
  case EXEC_LTP:
    len_of_buff = strlen(rl_line_buffer);
    for (int i = 1; i < num_of_matches; i++) {
      if (strncmp(rl_line_buffer, matches[i], len_of_buff) == 0) {
        if (i == num_of_matches - 1) {
          add_partial_completion(matches[i], true);
        } else {
          add_partial_completion(matches[i], false);
        }
        return NULL;
      }
    }
    found_multiple = NONE;
    break;
  case NONE:
    printf("You shouldn't be in %s when found_multiple is 'NONE'!\n",
           __FUNCTION__);
    exit(EXIT_FAILURE);
    break;
  }
  return NULL;
}

bool check_for_partial_completions() {
  int i = 1;
  while (i + 1 < num_of_matches) {
    int len = strlen(matches[i]);
    if (strncmp(matches[i], matches[i + 1], len) != 0) {
      break;
    }
    i++;
  }
  return i > 1 ? true : false;
}

void print_partial_completions() {
  printf("Inside %s\n", __FUNCTION__);
  for (int i = 0; i < num_of_matches; i++) {
    printf("match[%d]: %s\n", i, matches[i]);
  }
}

void update_num_of_matches() {
  num_of_matches = 0;
  while (matches[num_of_matches] != NULL) {
    num_of_matches++;
  }
}

void add_partial_completion(char *text_to_insert, bool add_space) {
  rl_delete_text(0, rl_end);
  rl_point = 0;
  if (text_to_insert == NULL) {
    rl_insert_text(matches[1]);
  } else {
    rl_insert_text(text_to_insert);
  }
  if (add_space) {
    rl_insert_text(" ");
  }
}

char **completion_func(const char *text, int start, int end) {
  if (found_multiple != NONE) {
    return multiple_matches();
  }
  matches = rl_completion_matches(text, built_in_generator);
  if (matches != NULL) {
    update_num_of_matches();
    if (num_of_matches == 1) {
      return matches;
    }
    qsort(matches, num_of_matches, sizeof(char *), match_sort);
    found_multiple = COMMAND;
    return NULL;
  }
  matches = rl_completion_matches(text, executable_name_generator);
  if (matches != NULL) {
    update_num_of_matches();
    if (num_of_matches == 1) {
      return matches;
    }
    qsort(matches, num_of_matches, sizeof(char *), match_sort);
    if (check_for_partial_completions() == true) {
      add_partial_completion(NULL, false);
      found_multiple = EXEC_LTP;
      return NULL;
    }
    found_multiple = EXEC;
    return NULL;
  }
  return NULL;
}

int main() {
  rl_attempted_completion_function = completion_func;
  ao = create_arg_obj();
  char *input;
  while (true) {
    // Wait for user input
    input = readline("$ ");
    if (!input) {
      free(input);
      break;
    }
    // Replace \n at end of string with null
    int len_of_input = strlen(input);
    if (len_of_input > 0 && input[len_of_input - 1] == '\n') {
      input[len_of_input - 1] = '\0';
    }
    /*ao->input = input;*/
    /*ao->curr_char = input;*/
    Cmd_Header *cmd = create_command(input);
    switch (cmd->type) {
      case CMD_EXIT:
        handle_exit_command();
        break;
      case CMD_INVALID:
        handle_invalid_command(cmd);
        break;
      case CMD_ECHO:
        handle_echo_command(cmd);
        break;
      case CMD_TYPE:
        handle_type_command(cmd);
        break;
      case CMD_EXECUTABLE:
        handle_executable_command(cmd);
        break;
      case CMD_PWD:
        handle_pwd_command();
        break;
      case CMD_CD:
        handle_cd_command(cmd);
        break;
    }
    free(input);
    free(cmd);
  }
  free(ao);
  return 0;
}
