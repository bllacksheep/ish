#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/wait.h>
#include <unistd.h>
#define MAX 100

void parser(char *);
ssize_t read_command(char *);
void repl();
void exec_command(int, char **);
void mystrcspn(char **c);

void repl() {
  char command[MAX + 1] = {0};
  printf("> ");
  fflush(stdout);
  switch (read_command(command)) {
  case 1:
    if (command[0] == '\n')
      break;
    break;
  default:
    parser(command);
    break;
  }
}

// read in the input and remove any newline at the end of the command
ssize_t read_command(char *buf) {
  ssize_t n = read(STDIN_FILENO, buf, MAX);
  if (n == -1) {
    perror("get command");
    exit(EXIT_FAILURE);
  }
  buf[n] = '\0';
  // newline in input command must be removed or break
  mystrcspn(&buf);
  return n;
}

// len bruh
size_t mylen(char **c) {
  size_t len = 0;
  while (c[len] != NULL)
    len++;
  return len;
}

// remove a newline by replace it with \0
void mystrcspn(char **c) {
  size_t len = 0;
  while ((*c)[len] != '\n')
    len++;
  (*c)[len] = '\0';
}

// parse out the number from the start of a command if exists
// step the command pointer forward to pos arg 1 the actual command name
void parse_iterator(char **buf, unsigned *iter) {
  if (buf == NULL && *buf == NULL && *iter != 0) {
    perror("parse iterator");
    exit(EXIT_FAILURE);
  }

  char capture[MAX + 1] = {0};
  char *p = *buf;
  size_t i = 0, s = 0;
  for (; *p != ' ' && *p != '\0'; capture[i++] = *p++)
    ;

  for (int j = 0; j < i; j++) {
    if (capture[j] == '0' || capture[j] == '1' || capture[j] == '2' ||
        capture[j] == '3' || capture[j] == '4' || capture[j] == '5' ||
        capture[j] == '6' || capture[j] == '7' || capture[j] == '8' ||
        capture[j] == '9') {
      *iter = *iter * 10 + (capture[j] - '0');
      (*buf)++;
      s++;
    }
    if ((*buf)[0] == ' ')
      (*buf)++;
  }
  /*
  printf("the capture %s\n", capture);
  printf("iter is %u\n", *iter);
  fflush(stdout);
  */
}

// create official argc argv used with exec
void parse_args(char *buf, char **argv, size_t *argn) {
  if (buf == NULL && argv == NULL && *argv == NULL) {
    perror("parse args");
    exit(EXIT_FAILURE);
  }
  char capture[MAX + 1] = {0};
  char *p = buf;
  size_t argc = 0;
  while (*p != '\0') {
    for (size_t i = 0; *p != ' ' && *p != '\0'; p++) {
      capture[i++] = *p;
    }
    argv[argc++] = strdup(capture);
    // skip space
    p++;
    memset(capture, 0, sizeof(capture));
  }
  argv[argc + 1] = NULL;
  *argn = argc;
}

// parser orchestroator pull together iterator + command + args
void parser(char *c) {
  // real parsing logic on c goes here
  unsigned iterator = 0;
  size_t arg_count = 0;
  // soft max on num args per command
  char *arg_vector[MAX] = {0};

  parse_iterator(&c, &iterator);

  // by convention use cmd ... args ... NULL terminate in NULL
  // return or set argc, command must end with NULL see execv
  parse_args(c, arg_vector, &arg_count);

  /*
  printf("num args %zu\n", arg_count);
  printf("iterator %u\n", iterator);
  fflush(stdout);
  */

  // handle builtins here
  if (strcmp(c, "exit") == 0)
    exit(0);
  if (iterator) {
    for (int i = 0; i < iterator; i++) {
      exec_command(arg_count, arg_vector);
    }
    return;
  }
  exec_command(arg_count, arg_vector);
  return;
}

// fork exec
void exec_command(int argc, char **argv) {
  pid_t pid;
  pid = fork();

  switch (pid) {
  case -1:
    perror("fork");
    exit(EXIT_FAILURE);
    break;
  case 0:
    // vp path aware
    execvp(argv[0], argv);
    break;
  default:
    // in sys/wait.h
    wait(NULL);
    break;
  }
}

int main(void) {

  while (1) {
    repl();
  }

  return 0;
}
