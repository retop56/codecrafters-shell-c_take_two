#include "arg_obj_def.h"
#include "argparser.h"
#include "cc_shell.h"
#include "handle_commands.h"
#include <readline/readline.h>
#include <readline/history.h>
/*char input[BUFF_LENGTH];*/
struct arg_obj *ao;

int main() {
  rl_bind_key('\t', rl_complete);
  ao = create_arg_obj();
  while (true) {
    /*printf("$ ");*/
    /*fflush(NULL);*/
    // Wait for user input
    
    /*if (fgets(input, BUFF_LENGTH, stdin) == NULL) {*/
      /*continue;*/
    /*}*/
    char *input = readline("$ ");
    if (!input) break;
    // Replace \n at end of string with null
    int len_of_input = strlen(input);
    if (len_of_input > 0 && input[len_of_input - 1] == '\n') {
      input[len_of_input - 1] = '\0';
    }
    /*ao->input = (char *)&input;*/
    ao->input = input;
    /*ao->curr_char = (char *)&input;*/
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
  return 0;
}
