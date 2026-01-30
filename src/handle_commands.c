#include "handle_commands.h"
#include "arg_obj_def.h"
#include "argparser.h"
#include "cc_shell.h"
#include "cmd_arg_parser.h"
#include "shell_types.h"
#include <fcntl.h>
#include <linux/limits.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
extern struct arg_obj *ao;
extern char *input;
extern char *curr_char;

char **resize_command_args(char **curr_args, int new_arg_size);

Cmd_Header *create_command(Args *ao) {
  char *possible_exe;
  if (ao->contains_pipe == true) {
    return create_pipeline_command(ao);
  } else if (ao->redir_type != NO_REDIR) {
    return create_redir_command(ao);
  } else if (strncmp(ao->args[0], "exit", 4) == 0) {
    return create_exit_command();
  } else if (strncmp(ao->args[0], "echo", 4) == 0) {
    return create_echo_command(ao);
  } else if (strncmp(ao->args[0], "type", 4) == 0) {
    return create_type_command(ao);
  } else if (strncmp(ao->args[0], "pwd", 3) == 0) {
    return create_pwd_command();
  } else if (strncmp(ao->args[0], "cd", 2) == 0) {
    return create_cd_command(ao);
  } else if ((possible_exe = search_for_exec(ao->args[0])) != NULL) {
    return create_executable_command(ao);
  } else {
    return create_invalid_command();
  }
  return NULL;
}

void handle_command(Cmd_Header *cmd) {
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
  case CMD_REDIR:
    handle_redir_command(cmd);
    break;
  case CMD_PIPELINE:
    handle_pipeline_command(cmd);
    break;
  default:
    fprintf(stderr, "Unable to determine command type!");
    exit(EXIT_FAILURE);
    break;
  }
}

static Cmd_Header *create_exit_command() {
  Exit_Command *c = (Exit_Command *)malloc(sizeof(Exit_Command));
  c->hdr.type = CMD_EXIT;
  return (Cmd_Header *)c;
}

static Cmd_Header *create_invalid_command() {
  Invalid_Command *c = (Invalid_Command *)malloc(sizeof(Invalid_Command));
  c->hdr.type = CMD_INVALID;
  c->txt = input;
  return (Cmd_Header *)c;
}

static Cmd_Header *create_echo_command(Args *ao) {
  Echo_Command *c = (Echo_Command *)malloc(sizeof(Echo_Command));
  c->hdr.type = CMD_ECHO;
  c->ao = ao;
  return (Cmd_Header *)c;
}

static Cmd_Header *create_type_command(Args *ao) {
  Type_Command *c = (Type_Command *)malloc(sizeof(Type_Command));
  c->hdr.type = CMD_TYPE;
  c->ao = ao;
  return (Cmd_Header *)c;
}

static Cmd_Header *create_executable_command(Args *ao) {
  Executable_Command *c =
      (Executable_Command *)malloc(sizeof(Executable_Command));
  c->hdr.type = CMD_EXECUTABLE;
  c->redir_type = check_if_redir_in_exec(ao);
  c->ao = ao;
  return (Cmd_Header *)c;
}

Cmd_Header *create_pwd_command() {
  Pwd_Command *c = (Pwd_Command *)malloc(sizeof(Pwd_Command));
  c->hdr.type = CMD_PWD;
  return (Cmd_Header *)c;
}

Cmd_Header *create_cd_command(Args *ao) {
  Cd_Command *c = (Cd_Command *)malloc(sizeof(Cd_Command));
  c->hdr.type = CMD_CD;
  c->ao = ao;
  return (Cmd_Header *)c;
}

Cmd_Header *create_redir_command(Args *ao) {
  Redir_Command *c = (Redir_Command *)malloc(sizeof(Redir_Command));
  c->hdr.type = CMD_REDIR;
  int redir_arg_num = 0;
  for (int i = 0; i < ao->size; i++) {
    if (strncmp(ao->args[i], ">>", 2) == 0 ||
        strncmp(ao->args[i], ">", 1) == 0 ||
        strncmp(ao->args[i], "1>>", 3) == 0 ||
        strncmp(ao->args[i], "1>", 2) == 0 ||
        strncmp(ao->args[i], "2>", 2) == 0 ||
        strncmp(ao->args[i], "2>>", 3) == 0) {
      break;
    }
    redir_arg_num++;
  }
  Args *new_args = create_args_obj();
  for (int i = 0; i < redir_arg_num; i++) {
    new_args->args[i] = ao->args[i];
    new_args->size++;
  }
  Cmd_Header *left_command = create_command(new_args);
  if (left_command == NULL) {
    fprintf(stderr, "Unable to get left-side command in %s! \n", __FUNCTION__);
    exit(EXIT_FAILURE);
  }
  c->command = left_command;
  c->filename = ao->args[redir_arg_num + 1];
  c->redir_type = check_if_redir_in_exec(ao);
  return (Cmd_Header *)c;
}

static Cmd_Header *create_pipeline_command(Args *ao) {
  Pipeline_Command *c = (Pipeline_Command *)malloc(sizeof(Pipeline_Command));
  c->hdr.type = CMD_PIPELINE;
  c->pipe_arg_num = 0;
  for (int i = 0; i < ao->size; i++) {
    if (strncmp(ao->args[i], "|", 1) == 0) {
      break;
    }
    c->pipe_arg_num++;
  }
  c->cmds = malloc(sizeof(Cmd_Header *) * 10);
  c->num_of_cmds = 0;
  int ao_arg_cnt = 0;
  int curr_arg_cnt = 0;
  Args *a = create_args_obj();
  while (ao_arg_cnt < ao->size) {
    if (*ao->args[ao_arg_cnt] != '|') {
      a->args[curr_arg_cnt] = ao->args[ao_arg_cnt];
      curr_arg_cnt++;
      ao_arg_cnt++;
      a->size++;
    } else {
      c->cmds[c->num_of_cmds++] = create_command(a);
      ao_arg_cnt++; // Skip past '|'
      curr_arg_cnt = 0;
      if (ao_arg_cnt < ao->size) {
        a = create_args_obj();
      }
    }
  }
  c->cmds[c->num_of_cmds++] = create_command(a);
  return (Cmd_Header *)c;
}

void handle_invalid_command(Cmd_Header *c) {
  Invalid_Command *ic = (Invalid_Command *)c;
  printf("%s: command not found\n", ((Invalid_Command *)c)->txt);
}

void handle_exit_command() { exit(0); }

void handle_echo_command(Cmd_Header *c) {
  Echo_Command *ec = (Echo_Command *)c;
  for (int i = 1; i < ec->ao->size; i++) {
    if (i == 1) {
      printf("%s", ec->ao->args[i]);
    } else {
      printf(" %s", ec->ao->args[i]);
    }
  }
  printf("\n");
}

void handle_type_command(Cmd_Header *c) {
  Type_Command *tc = (Type_Command *)c;
  char *possible_exe;
  if (strcmp(tc->ao->args[1], "echo") == 0 ||
      strcmp(tc->ao->args[1], "exit") == 0 ||
      strcmp(tc->ao->args[1], "type") == 0 ||
      strcmp(tc->ao->args[1], "pwd") == 0) {
    printf("%s is a shell builtin\n", tc->ao->args[1]);
  } else if ((possible_exe = search_for_exec(tc->ao->args[1])) != NULL) {
    printf("%s is %s\n", tc->ao->args[1], possible_exe);
  } else {
    printf("%s: not found\n", tc->ao->args[1]);
  }
}

void handle_executable_command(Cmd_Header *c) {
  Executable_Command *exec = (Executable_Command *)c;
  if (exec->redir_type != NO_REDIR) {
    return;
  }
  pid_t p = fork();
  switch (p) {
  case -1:
    fprintf(stderr, "Fork failed!\n");
    exit(EXIT_FAILURE);
    break;
  case 0:
    /*
     * Make sure last argument in list is NULL to satisfy requirements
     * of execvp
     */
    exec->ao->args[exec->ao->size] = NULL;
    execvp(exec->ao->args[0], exec->ao->args);
    perror("execvp");
    exit(EXIT_FAILURE);
  }
  wait(&p);
}

redir_t check_if_redir_in_exec(Args *ao) {
  for (int i = 0; i < ao->size; i++) {
    if (strncmp(ao->args[i], "1>>", 3) == 0 ||
        strncmp(ao->args[i], ">>", 2) == 0) {
      return APPEND_STD_OUT;
    }
    if (strncmp(ao->args[i], ">", 1) == 0 ||
        strncmp(ao->args[i], "1>", 2) == 0) {
      return REDIR_STD_OUT;
    }
    if (strncmp(ao->args[i], "2>>", 3) == 0) {
      return APPEND_STD_ERR;
    }
    if (strncmp(ao->args[i], "2>", 2) == 0) {
      return REDIR_STD_ERR;
    }
  }
  return NO_REDIR;
}

void handle_redir_command(Cmd_Header *c) {
  Redir_Command *rc = (Redir_Command *)c;
  int filefd;
  pid_t child_process;
  if (rc->redir_type == REDIR_STD_OUT) {
    filefd = open(rc->filename, O_WRONLY | O_CREAT, 0666);
    child_process = fork();
    switch (child_process) {
    case -1:
      fprintf(stderr, "Fork failed! (Func %s: Line %d)\n", __FUNCTION__,
              __LINE__);
      perror("fork");
      exit(EXIT_FAILURE);
      break;
    case 0: // child process
      close(STDOUT_FILENO);
      dup(filefd);
      handle_command(rc->command);
      close(filefd);
      exit(EXIT_SUCCESS);
      break;
    default: // parent process
      close(filefd);
      wait(&child_process);
      break;
    }
    return;
  }
  if (rc->redir_type == REDIR_STD_ERR) {
    filefd = open(rc->filename, O_WRONLY | O_CREAT, 0666);
    child_process = fork();
    switch (child_process) {
    case -1:
      fprintf(stderr, "Fork failed! (Func %s: Line %d)\n", __FUNCTION__,
              __LINE__);
      perror("fork");
      exit(EXIT_FAILURE);
      break;
    case 0: // child process
      close(STDERR_FILENO);
      dup(filefd);
      handle_command(rc->command);
      close(filefd);
      exit(EXIT_SUCCESS);
      break;
    default: // parent process
      close(filefd);
      wait(&child_process);
      break;
    }
    return;
  }
  if (rc->redir_type == APPEND_STD_OUT) {
    filefd = open(rc->filename, O_WRONLY | O_APPEND | O_CREAT, 0666);
    if (filefd == -1) {
      fprintf(stderr, "Failed to open %s! (Func %s: Line %d)\n", rc->filename,
              __FUNCTION__, __LINE__);
      perror("open");
      exit(EXIT_FAILURE);
    }
    child_process = fork();
    switch (child_process) {
    case -1:
      fprintf(stderr, "Fork failed! (Func %s: Line %d)\n", __FUNCTION__,
              __LINE__);
      perror("fork");
      exit(EXIT_FAILURE);
    case 0: // child process
      close(STDOUT_FILENO);
      dup(filefd);
      handle_command(rc->command);
      close(filefd);
      exit(EXIT_SUCCESS);
    default: // parent process
      close(filefd);
      wait(&child_process);
    }
    return;
  }
  if (rc->redir_type == APPEND_STD_ERR) {
    filefd = open(rc->filename, O_WRONLY | O_APPEND | O_CREAT, 0666);
    if (filefd == -1) {
      fprintf(stderr, "Failed to open %s! (Func %s: Line %d)\n", rc->filename,
              __FUNCTION__, __LINE__);
      perror("open");
      exit(EXIT_FAILURE);
    }
    child_process = fork();
    switch (child_process) {
    case -1:
      fprintf(stderr, "Fork failed! (Func %s: Line %d)\n", __FUNCTION__,
              __LINE__);
      perror("fork");
      exit(EXIT_FAILURE);
    case 0: // child process
      close(STDERR_FILENO);
      dup(filefd);
      handle_command(rc->command);
      close(filefd);
      exit(EXIT_SUCCESS);
    default: // parent process
      close(filefd);
      wait(&child_process);
    }
    return;
  }
}

void handle_pipeline_command(Cmd_Header *c) {
  Pipeline_Command *pc = (Pipeline_Command *)c;
  int prev_read_pipe = STDIN_FILENO;
  for (int curr_cmd = 0; curr_cmd < pc->num_of_cmds; curr_cmd++) {
    if (pipe(pc->fd) != 0) {
      perror("pipe");
      exit(EXIT_FAILURE);
    }
    pid_t f = fork();
    switch (f) {
    case -1:
      perror("fork");
      exit(EXIT_FAILURE);
      break;
    case 0: // child process
      if (curr_cmd == 0) {
        // first command
        close(STDOUT_FILENO);
        dup(pc->fd[1]);
      } else if (curr_cmd + 1 != pc->num_of_cmds) {
        // middle command
        close(STDIN_FILENO);
        dup(prev_read_pipe);
        close(STDOUT_FILENO);
        dup(pc->fd[1]);
      } else {
        // last command
        close(STDIN_FILENO);
        dup(prev_read_pipe);
      }
      close(pc->fd[0]);
      close(pc->fd[1]);
      handle_command(pc->cmds[curr_cmd]);
      exit(EXIT_SUCCESS);
    default:
      close(pc->fd[1]);
      prev_read_pipe = pc->fd[0];
      break;
    }
  }
  // close(pc->fd[0]);
  // close(pc->fd[1]);
  while (wait(NULL) > 0)
    ;
}

char *search_for_exec(char *exec_input) {
  char *full_path = (char *)malloc(PATH_MAX * sizeof(char));
  char *paths = strdup(getenv("PATH"));
  if (paths == NULL) {
    printf("Unable to get 'PATH' environment variable!\n");
    exit(EXIT_FAILURE);
  }
  char *curr_path = strtok(paths, ":");
  /*
    Loop through every path in the 'PATH' env_var and
    search for 'exec_name' in the path. If found, print it
  */
  DIR *dirp;
  while (curr_path != NULL) {
    dirp = opendir(curr_path);
    if (dirp == NULL) {
      curr_path = strtok(NULL, ":");
      continue;
    }
    struct dirent *pDirent;
    int len_exec_name = strlen(exec_input);
    // Loop through every entry in directory
    while ((pDirent = readdir(dirp)) != NULL) {
      if (strncmp(exec_input, pDirent->d_name, len_exec_name) != 0) {
        // Didn't find matching filename, check next dirent
        continue;
      }
      // Found matching file, now need to check if it's executable
      sprintf(full_path, "%s/%s", curr_path,
              exec_input); // Construct full path name
      if ((access(full_path, X_OK)) != 0) {
        // File not executable, check next dirent
        continue;
      }
      closedir(dirp);
      free(paths);
      return full_path;
    }
    curr_path = strtok(NULL, ":");
  }
  free(full_path);
  free(paths);
  closedir(dirp);
  return NULL;
}

void handle_pwd_command() {
  const char *pwd_str = getenv("PWD");
  if (pwd_str == NULL) {
    fprintf(stderr, "Unable to get pwd!\n");
    return;
  }
  printf("%s\n", pwd_str);
}

bool check_if_directory_exists(char *s) {
  DIR *dir = opendir(s);
  if (dir) {
    return true;
  }
  return false;
}

void handle_cd_command(Cmd_Header *c) {
  Cd_Command *cd_comm = (Cd_Command *)c;
  if (cd_comm->ao->args[0] == NULL) {
    return;
  }
  char new_dir[PATH_MAX] = {0};
  if (strncmp(cd_comm->ao->args[1], "./", 2) == 0) {
    strcat(new_dir, getenv("PWD"));
    strcat(new_dir, cd_comm->ao->args[1] + 1);
    if (check_if_directory_exists(new_dir) == false) {
      printf("cd: %s: No such file or directory\n", new_dir);
      return;
    }
    setenv("PWD", new_dir, 1);
    return;
  }
  if (strncmp(cd_comm->ao->args[1], "../", 3) == 0) {
    int count = 0;
    char *s = cd_comm->ao->args[1];
    while (strncmp(s, "../", 3) == 0) {
      count++;
      s += 3;
    }
    strcat(new_dir, getenv("PWD"));
    int len_of_curr_pwd = strlen(getenv("PWD"));
    char *start = new_dir;
    s = &new_dir[len_of_curr_pwd - 1];
    while (s != start && count != 0) {
      if (*s == '/') {
        if (--count == 0) {
          break;
        }
      }
      s--;
    }
    if (count == 0) {
      *s = '\0';
      setenv("PWD", new_dir, 1);
    }
    return;
  }
  if (strncmp(cd_comm->ao->args[1], "~", 1) == 0) {
    char *h = getenv("HOME");
    if (h != NULL) {
      setenv("PWD", h, 1);
    }
    return;
  }
  if (check_if_directory_exists(cd_comm->ao->args[1]) == false) {
    printf("cd: %s: No such file or directory\n", cd_comm->ao->args[1]);
    return;
  }
  setenv("PWD", cd_comm->ao->args[1], 1);
}
