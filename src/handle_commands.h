#include "arg_obj_def.h"
#include "cc_shell.h"
void handle_exit_command();
void handle_invalid_command(Cmd_Header *c);
void handle_echo_command(Cmd_Header *c);
void handle_type_command(Cmd_Header *c);
void handle_executable_command(Cmd_Header *c);
void handle_program_execution();
void handle_pwd_command();
void handle_cd_command(Cmd_Header *c);
void handle_program_exec_w_redirect_or_append();
void handle_program_exec_w_pipe();
Cmd_Header *create_command(char *input);
