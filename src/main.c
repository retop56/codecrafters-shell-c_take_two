#include "arg_obj_def.h"
#include "argparser.h"
#include "handle_commands.h"
#include <readline/history.h>
#include <readline/readline.h>
#include <stdlib.h>
#include <time.h>
#include <unistd.h>
struct arg_obj *ao;

char *possible_completion_options[] = {"echo", "exit", NULL};
char *built_in_generator(const char *text, int state);
char *executable_name_generator(const char *text, int state);
char **completion_func(const char *text, int start, int end);

typedef enum { COMMAND, EXEC, NONE } Multiple_T;

static Multiple_T found_multiple = NONE;
static char **matches;
static int num_of_matches = 0;

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
  switch (found_multiple) {
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
  case COMMAND:
    found_multiple = NONE;
    num_of_matches = 0;
    return matches;
  case NONE:
    printf("You shouldn't be in %s when found_multiple is 'NONE'!\n",
           __FUNCTION__);
    exit(EXIT_FAILURE);
  }
}

char **completion_func(const char *text, int start, int end) {
  if (found_multiple != NONE) {
    return multiple_matches();
  }
  /*static int num_of_matches;*/
  /*if (found_multiple == EXEC) {*/
  /*  /**/
  /*   * Check if there are commands with prefix of previous*/
  /*   * Ex. xyz_             is full prefix of*/
  /*   *     xyz_foo          which is full prefix of*/
  /*   *     xyz_foo_bar_     which is full prefix of*/
  /*   *     xyz_foo_bar_baz*/
  /*   */
  /*  qsort(matches, num_of_matches, sizeof(char *), match_sort);*/
  /*  found_multiple = NONE;*/
  /*  num_of_matches = 0;*/
  /*  return matches;*/
  /*}*/
  /*if (found_multiple == COMMAND) {*/
  /*  found_multiple = NONE;*/
  /*  num_of_matches = 0;*/
  /*  return matches;*/
  /*}*/
  num_of_matches = 0;
  matches = rl_completion_matches(text, built_in_generator);
  if (matches != NULL) {
    /*num_of_matches = 0;*/
    while (matches[num_of_matches] != NULL) {
      num_of_matches++;
    }
    if (num_of_matches == 1) {
      return matches;
    }
    found_multiple = COMMAND;
    return NULL;
  }
  matches = rl_completion_matches(text, executable_name_generator);
  if (matches != NULL) {
    /*num_of_matches = 0;*/
    while (matches[num_of_matches] != NULL) {
      num_of_matches++;
    }
    if (num_of_matches == 1) {
      return matches;
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
    ao->input = input;
    ao->curr_char = input;
    add_args();
    if (ao->redir_type != NO_REDIR) {
      handle_program_exec_w_redirect_or_append();
    } else if (strncmp(ao->args[0], "exit", 4) == 0) {
      handle_exit_command();
    } else if (strncmp(ao->args[0], "echo", 4) == 0) {
      handle_echo_command();
    } else if (strncmp(ao->args[0], "type", 4) == 0) {
      handle_type_command();
    } else if (strncmp(ao->args[0], "pwd", 3) == 0) {
      handle_pwd_command();
    } else if (strncmp(ao->args[0], "cd", 2) == 0) {
      handle_cd_command();
    } else {
      handle_program_execution();
    }
    clear_args();
    free(input);
  }
  free(ao);
  return 0;
}
