#include "handle_commands.h"
#include "arg_obj_def.h"
#include "argparser.h"
#include "cc_shell.h"
#include "cmd_arg_parser.h"
#include <fcntl.h>
#include <linux/limits.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
extern struct arg_obj *ao;
extern char* input;
extern char* curr_char;

static char *search_for_exec(char *exec_name);
Cmd_Header *create_exit_command();
Cmd_Header *create_invalid_command();
Cmd_Header *create_echo_command();
Cmd_Header *create_type_command();
Cmd_Header *create_executable_command();
Cmd_Header *create_pwd_command();
Cmd_Header *create_cd_command();
char **resize_command_args(char **curr_args, int new_arg_size);

Cmd_Header *create_command() {
  char *possible_exe;
  if (strncmp(input, "exit", 4) == 0) {
    return create_exit_command();
  } else if (strncmp(input, "echo ", 5) == 0) {
    curr_char += 5;
    return create_echo_command();
  } else if (strncmp(input, "type ", 5) == 0) {
    curr_char += 5;
    return create_type_command();
  } else if (strncmp(input, "pwd", 3) == 0) {
    return create_pwd_command();
  } else if (strncmp(input, "cd ", 3) == 0) {
    curr_char += 3;
    return create_cd_command();
  } else if ((possible_exe = search_for_exec(input)) != NULL) {
    return create_executable_command();
  } else {
    return create_invalid_command();
  }
  return NULL;
}

Cmd_Header *create_exit_command() {
  Exit_Command *c = (Exit_Command *)malloc(sizeof(Exit_Command));
  c->hdr.type = CMD_EXIT;
  return (Cmd_Header *)c;
}

Cmd_Header *create_invalid_command() {
  Invalid_Command *c = (Invalid_Command *)malloc(sizeof(Invalid_Command));
  c->hdr.type = CMD_INVALID;
  c->txt = input;
  return (Cmd_Header *)c;
}

Cmd_Header *create_echo_command() {
  Echo_Command *c = (Echo_Command *)malloc(sizeof(Echo_Command));
  c->hdr.type = CMD_ECHO;
  c->txt = input + 5;
  c->args = (char **)malloc(sizeof(char *) * 10);
  c->num_of_args = add_cmd_args(c->args);
  return (Cmd_Header *)c;
}

Cmd_Header *create_type_command() {
  Type_Command *c = (Type_Command *)malloc(sizeof(Type_Command));
  c->hdr.type = CMD_TYPE;
  c->txt = input + 5;
  return (Cmd_Header *)c;
}

Cmd_Header *create_executable_command() {
  Executable_Command *c =
      (Executable_Command *)malloc(sizeof(Executable_Command));
  c->hdr.type = CMD_EXECUTABLE;
  int arg_size = 10;
  c->args = (char **)malloc(sizeof(char *) * arg_size);
  c->num_of_args = add_cmd_args(c->args);
  return (Cmd_Header *)c;
}

Cmd_Header *create_pwd_command() {
  Pwd_Command *c = (Pwd_Command *)malloc(sizeof(Pwd_Command));
  c->hdr.type = CMD_PWD;
  return (Cmd_Header *)c;
}

Cmd_Header *create_cd_command() {
  Cd_Command *c = (Cd_Command *)malloc(sizeof(Cd_Command));
  c->hdr.type = CMD_CD;
  c->txt = input + 3;
  return (Cmd_Header *)c;
}

char **resize_command_args(char **curr_args, int new_arg_size) {
  char **new_args = realloc(curr_args, (sizeof(char **) * new_arg_size));
  if (new_args == NULL) {
    printf("Realloc failed in %s (Line: %d)\n", __FUNCTION__, __LINE__);
    exit(EXIT_FAILURE);
  }
  return new_args;
}

void handle_invalid_command(Cmd_Header *c) {
  printf("%s: command not found\n", ((Invalid_Command *)c)->txt);
}

void handle_exit_command() { exit(0); }

void handle_echo_command(Cmd_Header *c) {
  Echo_Command *ec = (Echo_Command *)c;
  for (int i = 0; i < ec->num_of_args; i++) {
    if (i == 0) {
      printf("%s", ec->args[i]);
    } else {
      printf(" %s", ec->args[i]);
    }
  }
  printf("\n");
}

void handle_type_command(Cmd_Header *c) {
  char *t = ((Type_Command *)c)->txt;
  char *possible_exe;
  if (strcmp(t, "echo") == 0 || strcmp(t, "exit") == 0 ||
      strcmp(t, "type") == 0 || strcmp(t, "pwd") == 0) {
    printf("%s is a shell builtin\n", t);
  } else if ((possible_exe = search_for_exec(t)) != NULL) {
    printf("%s is %s\n", t, possible_exe);
  } else {
    printf("%s: not found\n", t);
  }
}

void handle_executable_command(Cmd_Header *c) {
  Executable_Command *exec = (Executable_Command *)c;
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
    exec->args[exec->num_of_args] = NULL;
    execvp(exec->args[0], exec->args);
    perror("execvp");
    exit(EXIT_FAILURE);
  }
  wait(&p);
}
void handle_program_execution() {
  char *full_path = search_for_exec(ao->args[0]);
  if (full_path == NULL) {
    printf("%s: command not found\n", ao->args[0]);
    return;
  }
  pid_t p = fork();
  switch (p) {
  case -1:
    fprintf(stderr, "Fork failed!\n");
    exit(EXIT_FAILURE);
  case 0:
    /*
     * Make sure last argument in list is NULL to satisfy requirements
     * of execvp
     */
    ao->args[ao->size] = (char *)NULL;
    execvp(full_path, ao->args);
    break;
  default:
    wait(&p);
    free(full_path);
  }
}

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

void handle_program_exec_w_redirect_or_append() {
  size_t i = 0;
  size_t size_of_full_command = 0;
  switch (ao->redir_type) {
  case STD_OUT:
    for (; i < ao->size; i++) {
      if (strncmp(ao->args[i], ">", 1) == 0 ||
          strncmp(ao->args[i], "1>", 2) == 0) {
        break;
      }
      size_of_full_command += strlen(ao->args[i]);
    }
    break;
  case STD_ERR:
    for (; i < ao->size; i++) {
      if (strncmp(ao->args[i], "2>", 2) == 0) {
        break;
      }
      size_of_full_command += strlen(ao->args[i]);
    }
    break;
  case APPEND_STD_OUT:
    for (; i < ao->size; i++) {
      if (strncmp(ao->args[i], ">>", 2) == 0 ||
          strncmp(ao->args[i], "1>>", 3) == 0) {
        break;
      }
      size_of_full_command += strlen(ao->args[i]);
    }
    break;
  case APPEND_STD_ERR:
    for (; i < ao->size; i++) {
      if (strncmp(ao->args[i], "2>>", 3) == 0) {
        break;
      }
      size_of_full_command += strlen(ao->args[i]);
    }
    break;
  case NO_REDIR:
    fprintf(stderr, "ao->redir_type is INITIAL_VAL for some reason \n");
    break;
  }

  size_of_full_command += i; // For spaces between each command;
  if (i == ao->size) {
    char *err_line;
    switch (ao->redir_type) {
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
  strcat(full_command, ao->args[0]);
  for (size_t x = 1; x < i; x++) {
    strcat(full_command, " ");
    strcat(full_command, ao->args[x]);
  }
  char output_buff[BUFF_LENGTH] = {0};
  if (ao->redir_type == STD_OUT || ao->redir_type == APPEND_STD_OUT) {
    FILE *cmd_f_ptr = popen(full_command, "r");
    if (cmd_f_ptr == NULL) {
      fprintf(stderr, "popen failed on line %d in %s\n", __LINE__,
              __FUNCTION__);
      exit(EXIT_FAILURE);
    }
    FILE *output_file;
    if (ao->redir_type == STD_OUT) {
      output_file = fopen(ao->args[i + 1], "w");
    } else {
      output_file = fopen(ao->args[i + 1], "a");
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
    if (ao->redir_type == STD_ERR) {
      fd = open(ao->args[i + 1], O_CREAT | O_WRONLY);
    } else {
      fd = open(ao->args[i + 1], O_CREAT | O_APPEND | O_WRONLY);
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
  char *exec_only = strdup(exec_input);
  exec_only = strtok(exec_only, " ");
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
    int len_exec_name = strlen(exec_only);
    // Loop through every entry in directory
    while ((pDirent = readdir(dirp)) != NULL) {
      if (strncmp(exec_only, pDirent->d_name, len_exec_name) != 0) {
        // Didn't find matching filename, check next dirent
        continue;
      }
      // Found matching file, now need to check if it's executable
      sprintf(full_path, "%s/%s", curr_path,
              exec_only); // Construct full path name
      if ((access(full_path, X_OK)) != 0) {
        // File not executable, check next dirent
        continue;
      }
      closedir(dirp);
      free(paths);
      free(exec_only);
      return full_path;
    }
    curr_path = strtok(NULL, ":");
  }
  free(full_path);
  free(paths);
  free(exec_only);
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
  if (cd_comm->txt == NULL) {
    return;
  }
  char new_dir[PATH_MAX] = {0};
  if (strncmp(cd_comm->txt, "./", 2) == 0) {
    strcat(new_dir, getenv("PWD"));
    strcat(new_dir, cd_comm->txt + 1);
    if (check_if_directory_exists(new_dir) == false) {
      printf("cd: %s: No such file or directory\n", new_dir);
      return;
    }
    setenv("PWD", new_dir, 1);
    return;
  }
  if (strncmp(cd_comm->txt, "../", 3) == 0) {
    int count = 0;
    char *s = cd_comm->txt;
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
  if (strncmp(cd_comm->txt, "~", 1) == 0) {
    char *h = getenv("HOME");
    if (h != NULL) {
      setenv("PWD", h, 1);
    }
    return;
  }
  if (check_if_directory_exists(cd_comm->txt) == false) {
    printf("cd: %s: No such file or directory\n", cd_comm->txt);
    return;
  }
  setenv("PWD", cd_comm->txt, 1);
}
