#include "arg_obj_def.h"
#include "argparser.h"
#include "handle_commands.h"
#include <readline/readline.h>
#include <readline/history.h>
#include <time.h>
struct arg_obj *ao;

char *possible_completion_options[] = {
  "echo",
  "exit",
  NULL
};

char * command_name_generator(const char *text, int state) {
  static int list_index, len;
  char * name;

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

char **completion_func(const char *text, int start, int end) {
  /*rl_attempted_completion_over = 1;*/
  return rl_completion_matches(text, command_name_generator);
};


int main() {
  rl_bind_key('\t', rl_complete);
  rl_attempted_completion_function = completion_func;

  ao = create_arg_obj();
  char *input;
  while (true) {
    // Wait for user input
    input = readline("$ ");
    if (!input) break;
    // Replace \n at end of string with null
    int len_of_input = strlen(input);
    if (len_of_input > 0 && input[len_of_input - 1] == '\n') {
      input[len_of_input - 1] = '\0';
    }
    ao->input = input;
    ao->curr_char = input;
    add_args();
    if (ao->redir_type != INITIAL_VAL) {
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
  }
  free(ao);
  free(input);
  return 0;
}
