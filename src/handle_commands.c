#include "handle_commands.h"
#include "arg_obj_def.h"
#include "argparser.h"
#include "cc_shell.h"
#include "cmd_arg_parser.h"
#include "shell_types.h"
#include <fcntl.h>
#include <linux/limits.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
extern struct arg_obj *ao;
extern char *input;
extern char *curr_char;

char **resize_command_args(char **curr_args, int new_arg_size);

Cmd_Header *create_command(Args *ao) {
  char *possible_exe;
  if (ao->redir_type != NO_REDIR) {
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
    if (strncmp(ao->args[i], ">", 1) == 0) {
      break;
    }
    redir_arg_num++;
  }
  printf("redir_arg_num: %d\n", redir_arg_num);
  exit(EXIT_FAILURE);
}

/*char **resize_command_args(char **curr_args, int new_arg_size) {*/
/*  char **new_args = realloc(curr_args, (sizeof(char **) * new_arg_size));*/
/*  if (new_args == NULL) {*/
/*    printf("Realloc failed in %s (Line: %d)\n", __FUNCTION__, __LINE__);*/
/*    exit(EXIT_FAILURE);*/
/*  }*/
/*  return new_args;*/
/*}*/

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
    handle_program_exec_w_redirect_or_append(exec);
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
    if (strncmp(ao->args[i], ">", 1) == 0 ||
        strncmp(ao->args[i], "1>", 2) == 0) {
      return STD_OUT;
    }
  }
  return NO_REDIR;
}
/*void handle_program_execution() {*/
/*  char *full_path = search_for_exec(ao->args[0]);*/
/*  if (full_path == NULL) {*/
/*    printf("%s: command not found\n", ao->args[0]);*/
/*    return;*/
/*  }*/
/*  pid_t p = fork();*/
/*  switch (p) {*/
/*  case -1:*/
/*    fprintf(stderr, "Fork failed!\n");*/
/*    exit(EXIT_FAILURE);*/
/*  case 0:*/
/*    /**/
/*     * Make sure last argument in list is NULL to satisfy requirements*/
/*     * of execvp*/
/*     */
/*    ao->args[ao->size] = (char *)NULL;*/
/*    execvp(full_path, ao->args);*/
/*    break;*/
/*  default:*/
/*    wait(&p);*/
/*    free(full_path);*/
/*  }*/
/*}*/

void handle_program_exec_w_pipe() {
  size_t i = 0;
  for (; i < ao->size; i++) {
    if (strncmp(ao->args[i], "|", 1) == 0) {
      break;
    }
  }
  char **first_prog_args = (char **)malloc((i + 1) * sizeof(char *));
  for (size_t a = 0; a < i; a++) {
    first_prog_args[a] = ao->args[a];
  }
  first_prog_args[i] = NULL; /* argument array must be null-terminated */
  int fildes[2];
  if (pipe(fildes) == -1) {
    perror("pipe");
    exit(EXIT_FAILURE);
  }
  char buf[BUFF_LENGTH];
  pid_t first_fork = fork();
  switch (first_fork) {
  case -1:
    perror("fork");
    exit(EXIT_FAILURE);
    break;
  case 0: // child process
    close(STDOUT_FILENO);
    dup(fildes[1]);
    close(fildes[0]);
    close(fildes[1]);
    execvp(first_prog_args[0], first_prog_args);
    perror("execvp");
    exit(EXIT_FAILURE);
    break;
  }
  char **second_prog_args = (char **)malloc((ao->size - i) * sizeof(char *));
  int a = 0;
  i++; /* skip past '|' arg */
  while (i < ao->size) {
    second_prog_args[a] = ao->args[i];
    a++;
    i++;
  }
  second_prog_args[a] = (char *)NULL;
  pid_t second_fork = fork();
  switch (second_fork) {
  case -1:
    perror("fork");
    exit(EXIT_FAILURE);
    break;
  case 0: /* child process */
    close(STDIN_FILENO);
    dup(fildes[0]);
    close(fildes[0]);
    close(fildes[1]);
    execvp(second_prog_args[0], second_prog_args);
    perror("execvp");
    exit(EXIT_FAILURE);
    break;
  }
  close(fildes[0]);
  close(fildes[1]);
  waitpid(first_fork, NULL, 0);
  waitpid(second_fork, NULL, 0);
}

void handle_program_exec_w_redirect_or_append(Executable_Command *exec) {
  size_t i = 0;
  size_t size_of_full_command = 0;
  switch (exec->redir_type) {
  case STD_OUT:
    for (; i < exec->ao->size; i++) {
      if (strncmp(exec->ao->args[i], ">", 1) == 0 ||
          strncmp(exec->ao->args[i], "1>", 2) == 0) {
        break;
      }
      size_of_full_command += strlen(exec->ao->args[i]);
    }
    break;
  case APPEND_STD_OUT:
    printf("Not yet: APPEND_STD_OUT\n");
    exit(EXIT_FAILURE);
  /*  for (; i < ao->size; i++) {*/
  /*    if (strncmp(ao->args[i], ">>", 2) == 0 ||*/
  /*        strncmp(ao->args[i], "1>>", 3) == 0) {*/
  /*      break;*/
  /*    }*/
  /*    size_of_full_command += strlen(ao->args[i]);*/
  /*  }*/
  /*  break;*/
  /*case STD_ERR:*/
  /*  for (; i < ao->size; i++) {*/
  /*    if (strncmp(ao->args[i], "2>", 2) == 0) {*/
  /*      break;*/
  /*    }*/
  /*    size_of_full_command += strlen(ao->args[i]);*/
  /*  }*/
  /*  break;*/
  case APPEND_STD_ERR:
    printf("Not yet: APPEND_STD_ERR\n");
    exit(EXIT_FAILURE);
  /*  for (; i < ao->size; i++) {*/
  /*    if (strncmp(ao->args[i], "2>>", 3) == 0) {*/
  /*      break;*/
  /*    }*/
  /*    size_of_full_command += strlen(ao->args[i]);*/
  /*  }*/
  /*  break;*/
  case NO_REDIR:
    fprintf(stderr, "ao->redir_type is NO_REDIR for some reason \n");
    break;
  }

  size_of_full_command += i; // For spaces between each command;
  if (i == exec->ao->size) {
    char *err_line;
    switch (exec->redir_type) {
    case STD_OUT:
      err_line = "Unable to find '>' or '1>' operator in ao->args!\n";
      break;
    case STD_ERR:
      err_line = "Unable to find '>' or '2>' operator in ao->args!\n";
      break;
    case APPEND_STD_OUT:
      err_line = "Unable to find '>>' or '1>>' operator in ao->args!\n";
      break;
    case APPEND_STD_ERR:
      err_line = "Unable to find '2>>' operator in ao->args!\n";
    case NO_REDIR:
      err_line = "ao->redir_type is INITIAL_VAL for some reason \n";
      break;
    }
    fprintf(stderr, "%s\n", err_line);
    exit(EXIT_FAILURE);
  }
  char *full_command = (char *)malloc(sizeof(char) * size_of_full_command + 1);
  full_command[0] = '\0';
  strcat(full_command, exec->ao->args[0]);
  for (size_t x = 1; x < i; x++) {
    strcat(full_command, " ");
    strcat(full_command, exec->ao->args[x]);
  }
  char output_buff[BUFF_LENGTH] = {0};
  if (exec->redir_type == STD_OUT || exec->redir_type == APPEND_STD_OUT) {
    FILE *cmd_f_ptr = popen(full_command, "r");
    if (cmd_f_ptr == NULL) {
      fprintf(stderr, "popen failed on line %d in %s\n", __LINE__,
              __FUNCTION__);
      exit(EXIT_FAILURE);
    }
    FILE *output_file;
    if (exec->redir_type == STD_OUT) {
      output_file = fopen(exec->ao->args[i + 1], "w");
    } else {
      output_file = fopen(exec->ao->args[i + 1], "a");
    }
    if (output_file == NULL) {
      fprintf(stderr, "fopen failed on line %d in %s\n", __LINE__,
              __FUNCTION__);
      exit(EXIT_FAILURE);
    }

    while (fgets(output_buff, BUFF_LENGTH, cmd_f_ptr) != NULL) {
      fprintf(output_file, "%s", output_buff);
    }
    pclose(cmd_f_ptr);
    fclose(output_file);
    free(full_command);
    return;
  } else if (ao->redir_type == STD_ERR || ao->redir_type == APPEND_STD_ERR) {
    int fd;
    if (exec->redir_type == STD_ERR) {
      fd = open(exec->ao->args[i + 1], O_CREAT | O_WRONLY);
    } else {
      fd = open(exec->ao->args[i + 1], O_CREAT | O_APPEND | O_WRONLY);
    }
    if (fd == -1) {
      perror("open");
      exit(EXIT_FAILURE);
    }
    pid_t pid = fork();
    switch (pid) {
    case -1:
      perror("fork");
      exit(EXIT_FAILURE);
    case 0:
      if ((dup2(fd, STDERR_FILENO)) == -1) {
        perror("dup2");
        exit(EXIT_FAILURE);
      }
      system(full_command);
    default:
      wait(&pid);
    }
    free(full_command);
    return;
  }
  printf("You shouldn't be here (Line %d)\n", __LINE__);
}

char *search_for_exec(char *exec_input) {
  char *full_path = (char *)malloc(PATH_MAX * sizeof(char));
  /*char *exec_only = strdup(exec_input);*/
  /*exec_only = strtok(exec_only, " ");*/
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
