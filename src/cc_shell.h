#pragma once

#include <dirent.h>
#include <limits.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>
#include <stddef.h>
#include "cmd_arg_parser.h"

#define BUFF_LENGTH 1000

typedef enum {
  CMD_EXIT,
  CMD_INVALID,
  CMD_ECHO,
  CMD_TYPE,
  CMD_EXECUTABLE,
  CMD_PWD,
  CMD_CD,
  CMD_COUNT
} Cmd_Type;


typedef struct {
  Cmd_Type type;
} Cmd_Header;

typedef struct {
  Cmd_Header hdr;
} Exit_Command;

typedef struct {
  Cmd_Header hdr;
  char * txt;
} Invalid_Command;

typedef struct {
  Cmd_Header hdr;
  char * txt;
  Args *ao;
} Echo_Command;

typedef struct {
  Cmd_Header hdr;
  Args *ao;
} Type_Command;

typedef struct {
  Cmd_Header hdr;
  Args *ao;
} Executable_Command;

typedef struct {
  Cmd_Header hdr;
} Pwd_Command;

typedef struct {
  Cmd_Header hdr;
  Args *ao;
} Cd_Command;
