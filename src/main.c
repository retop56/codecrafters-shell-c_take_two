#include "arg_obj_def.h"
#include "cmd_arg_parser.h"
#include "handle_commands.h"
#include <readline/history.h>
#include <readline/readline.h>
#include <stdlib.h>
#include <time.h>
#include <unistd.h>
#include "cc_shell.h"
#include "shell_types.h"
#include "completion.h"
#include "history_management.h"
struct arg_obj *ao;
char *input = NULL;
char *curr_char = NULL;
int last_append_entry_num = -1;

int main() {
  rl_attempted_completion_function = completion_func;
  Args *ao;
  using_history();
  check_for_history();
  while (true) {
    // Wait for user input
    ao = create_args_obj();
    input = readline("$ ");
    if (!input) {
      free(input);
      break;
    }
    add_history(input);
    curr_char = input;
    add_cmd_args(ao);
    // Replace \n at end of string with null
    int len_of_input = strlen(input);
    if (len_of_input > 0 && input[len_of_input - 1] == '\n') {
      input[len_of_input - 1] = '\0';
    }
    Cmd_Header *cmd = create_command(ao);
    handle_command(cmd);
    free(input);
    free_command(cmd);
  }
  return 0;
}
