#include "arg_obj_def.h"
#include "cmd_arg_parser.h"
#include "argparser.h"
#include "handle_commands.h"
#include <readline/history.h>
#include <readline/readline.h>
#include <stdlib.h>
#include <time.h>
#include <unistd.h>
#include "cc_shell.h"
#include "shell_types.h"
#include "completion.h"
struct arg_obj *ao;
char *input = NULL;
char *curr_char = NULL;
int last_append_entry_num = -1;

void check_for_history() {
  char *hist_file = getenv("HISTFILE");
  if (hist_file == NULL) {
    return;
  }
  // if (read_history(hist_file) != 0) {
  //   fprintf(stderr, "'read_history' failed! (%s: Line %d)\n", __FUNCTION__, __LINE__);
  //   exit(EXIT_FAILURE);
  // }
  read_history(hist_file);
}

int main() {
  rl_attempted_completion_function = completion_func;
  Args *ao;
  check_for_history();
  using_history();
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
    free(cmd);
    free_arg_object(ao);
  }
  return 0;
}
