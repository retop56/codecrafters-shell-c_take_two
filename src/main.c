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
char *command_name_generator(const char *text, int state);
char *executable_name_generator(const char *text, int state);
char **completion_func(const char *text, int start, int end);

static bool found_multiple = false;

char *command_name_generator(const char *text, int state) {
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

char **completion_func(const char *text, int start, int end) {
  static char **matches;
  if (found_multiple == true) {
    found_multiple = false;
    return matches;
  }
  matches = rl_completion_matches(text, command_name_generator);
  if (matches == NULL) {
    matches = rl_completion_matches(text, executable_name_generator);
    if (matches == NULL) {
      return NULL;
    }
  }
  int num_of_matches = 0;
  while (matches[num_of_matches] != NULL) {
    num_of_matches++;
  }
  if (num_of_matches > 1) {
    found_multiple = true;
    return NULL;
  }
  return matches;
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
