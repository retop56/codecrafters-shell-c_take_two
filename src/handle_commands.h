#pragma once

#include "cc_shell.h"
#include "shell_types.h"


static char *search_for_exec(char *exec_name);
Cmd_Header *create_command(Args *ao);
void free_command(Cmd_Header *cmd);
static Cmd_Header *create_exit_command();
static Cmd_Header *create_echo_command(Args *ao);
static Cmd_Header *create_type_command(Args *ao);
static Cmd_Header *create_invalid_command();
static Cmd_Header *create_executable_command(Args *ao);
static Cmd_Header *create_pwd_command();
static Cmd_Header *create_cd_command(Args *ao);
static Cmd_Header *create_redir_command(Args *ao);
static Cmd_Header *create_pipeline_command(Args *ao);
static Cmd_Header *create_history_command(Args *ao);
static redir_t check_if_redir_in_exec(Args *ao);

void handle_command(Cmd_Header* cmd);
void handle_exit_command();
void handle_invalid_command(Cmd_Header *c);
void handle_echo_command(Cmd_Header *c);
void handle_type_command(Cmd_Header *c);
void handle_executable_command(Cmd_Header *c);
void handle_program_execution();
void handle_pwd_command();
void handle_cd_command(Cmd_Header *c);
void handle_redir_command(Cmd_Header *c);
void handle_pipeline_command(Cmd_Header *c);
void handle_history_command(Cmd_Header *c);
