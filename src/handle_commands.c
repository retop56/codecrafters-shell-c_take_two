#include "handle_commands.h"
#include "arg_obj_def.h"
#include "argparser.h"
#include "cc_shell.h"
#include <fcntl.h>
#include <stdlib.h>
#include <unistd.h>
extern struct arg_obj *ao;

static char *search_for_exec(char *exec_name);

void handle_exit_command() { exit(0); }

void handle_echo_command() {
  if (ao->args[1] != NULL) {
    printf("%s", ao->args[1]);
  }
  for (size_t i = 2; i < ao->size; i++) {
    printf(" %s", ao->args[i]);
  }
  printf("\n");
}

void handle_type_command() {
  if (strcmp(ao->args[1], "echo") == 0 || strcmp(ao->args[1], "exit") == 0 ||
      strcmp(ao->args[1], "type") == 0 || strcmp(ao->args[1], "pwd") == 0) {
    printf("%s is a shell builtin\n", ao->args[1]);
  } else {
    char *exec_name = (char *)malloc(256 * sizeof(char));
    strcpy(exec_name, ao->args[1]);
    char *full_path = search_for_exec(exec_name);
    if (full_path == NULL) {
      printf("%s: not found\n", exec_name);
      free(full_path);
      free(exec_name);
      return;
    }
    printf("%s is %s\n", exec_name, full_path);
    free(full_path);
    free(exec_name);
  }
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
  case INITIAL_VAL:
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
    case INITIAL_VAL:
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
      fd = open(ao->args[i+ 1], O_CREAT | O_APPEND | O_WRONLY);
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

char *search_for_exec(char *exec_name) {
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
    int len_exec_name = strlen(exec_name);
    // Loop through every entry in directory
    while ((pDirent = readdir(dirp)) != NULL) {
      if (strncmp(exec_name, pDirent->d_name, len_exec_name) == 0) {
        // Found matching file, now need to check if it's executable
        sprintf(full_path, "%s/%s", curr_path,
                exec_name); // Construct full path name
        if ((access(full_path, X_OK)) == 0) {
          closedir(dirp);
          free(paths);
          return full_path;
        }
      }
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

void handle_cd_command() {
  if (*ao->args[1] == '\0') {
    return;
  } else if (strncmp(ao->args[1], "./", 2) == 0) {
    char *fixed_arg = strdup(ao->args[1] + 1);
    char *new_dir = (char *)calloc(1000, sizeof(char));
    strcat(new_dir, getenv("PWD"));
    strcat(new_dir, fixed_arg);
    if (chdir(new_dir) != 0) {
      printf("cd: %s: No such file or directory\n", ao->args[1]);
      return;
    }
    setenv("PWD", new_dir, 1);
    return;
  } else if (strncmp(ao->args[1], "../", 3) == 0) {
    size_t num_of_levels = 1;
    char *arg_ptr = ao->args[1] + 3;
    while (*arg_ptr != '\0' && strncmp(arg_ptr, "../", 3) == 0) {
      arg_ptr += 3;
      num_of_levels++;
    }
    char *curr_pwd = strdup(getenv("PWD"));
    char *pwd_ptr = curr_pwd + (strlen(curr_pwd) - 1);
    for (; num_of_levels > 0; num_of_levels--) {
      pwd_ptr--;
      while (*pwd_ptr != '/') {
        pwd_ptr--;
      }
    }
    size_t length_of_new_pwd = pwd_ptr - curr_pwd;

    curr_pwd[length_of_new_pwd] = '\0';
    setenv("PWD", curr_pwd, 1);
    return;
  } else if (strncmp(ao->args[1], "~", 1) == 0) {
    char *home_dir = getenv("HOME");
    setenv("PWD", home_dir, 1);
    return;
  } else if (chdir(ao->args[1]) != 0) {
    printf("cd: %s: No such file or directory\n", ao->args[1]);
    return;
  }
  setenv("PWD", ao->args[1], 1);
}
