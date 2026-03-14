#include "shell.h"
#include "builtins.h"
#include "errors.h"
#include "ht.h"
#include "parser.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/wait.h>
#include <unistd.h>

#define MAX_INPUT_STREAM 100
#define MAX_ARGV_LENGTH 10

typedef struct ish_state {
  semantic_token_t **session_tokens;
  size_t session_token_count;
  size_t iterator_x; // user provided at cli
  size_t iterator_i; // shell builtin 1
  size_t iterator_j; // shell builtin 2
                     // builtins
                     // environment variables
  size_t argc;
  char *argv[MAX_ARGV_LENGTH];
  handler_t handler;
} shell_state_t;

// use getter here
static shell_state_t ishell = {0};

void shell_execution_pipeline();
ssize_t shell_read_input_stream(char *);
void shell_execution_handler(size_t, char **);
void shell_run_command(handler_t callback, size_t argc, void **argv);
void mystrcspn(char **);
void shell_destroy_tokens(size_t, semantic_token_t **);

void shell_repl() {
  char input[MAX_INPUT_STREAM + 1] = {0};
  printf("> ");
  if (fflush(stdout) == EOF) {
    err_exit("repl flush", EXIT_FAILURE);
  };
  switch (shell_read_input_stream(input)) {
  case 1:
    if (input[0] == '\n')
      break;
    break;
  case 0:
    fprintf(stderr, "i.sh: no bytes, strange, should never happen...\n");
    return;
    break;
  default:
    parser_simple_parser(input);
    shell_execution_pipeline();
    // shell_destroy_tokens(tc, tvec);
    break;
  }
}

// read in the input and remove any newline at the end of the command
ssize_t shell_read_input_stream(char *buf) {
  if (buf == NULL) {
    err_exit("no input buffer", ERRNOBUFFER);
  }
  ssize_t n = read(STDIN_FILENO, buf, MAX_INPUT_STREAM);
  if (n == -1) {
    err_exit("failed to read input", EXIT_FAILURE);
  }
  if (n > 0)
    buf[n] = '\0';
  if (n == ERRNOBYTES) {
    // fprintf(stderr, "i.sh: no bytes read from input, code: %d\n",
    // ERRNOBYTES);
    return ERRNOBYTES;
  }
  // newline in input command must be removed or break
  mystrcspn(&buf);
  return n;
}

// len bruh
size_t mylen(char **c) {
  if (c == NULL || *c == NULL) {
    err_exit("no buffer to take len", ERRNOBUFFER);
  }
  size_t len = 0;
  while (c[len] != NULL)
    len++;
  return len;
}

// remove a newline by replace it with \0
void mystrcspn(char **c) {
  if (c == NULL || *c == NULL) {
    err_exit("no buffer to remove new line", ERRNOBUFFER);
  }
  size_t len = 0;
  while ((*c)[len] != '\n')
    len++;
  (*c)[len] = '\0';
}

// destroy args allocated with strdup
void shell_destroy_tokens(size_t tokenc, semantic_token_t **tokenv) {
  for (size_t i = 0; i < tokenc; i++) {
    if (tokenv[i] != NULL) {
      if (tokenv[i]->buf != NULL)
        // allocated by strdup at parse time
        free(tokenv[i]->buf);
      // allocated at arg initialization
      free(tokenv[i]);
    }
  }
}

// populate env vars and builtins here
shell_state_t *shell_get_shell_state(void) {
  // shell_check_shell_state();
  // shell_init_shell_state();
  return &ishell;
}

void shell_execution_pipeline() {
  // if private can only hold a pointer
  shell_state_t *ish = shell_get_shell_state();

  // shell functions will run this
  // should run at least once
  for (int i = 0; i < (ish->iterator_x == 0 ? 1 : ish->iterator_x); i++) {
    shell_run_shell_command(ish->handler, ish->argc, ish->argv);
  }
  // shell_clean_shell_state();
}

void shell_run_shell_command(handler_t callback, size_t argc, void **argv) {
  // some checks
  callback(argc, argv);
}

void shell_set_shell_argv(char **argv) {
  shell_state_t *st = shell_get_shell_state();
  memcpy(st->argv, argv, sizeof(**argv));
}

void shell_set_shell_tokens(semantic_token_t **tokenvec) {
  shell_state_t *st = shell_get_shell_state();
  memcpy(st->session_tokens, tokenvec, sizeof(**tokenvec));
}

void shell_set_shell_it_x(size_t x) {
  shell_state_t *st = shell_get_shell_state();
  st->iterator_x = x;
}

void shell_set_shell_it_i(size_t i) {
  shell_state_t *st = shell_get_shell_state();
  st->iterator_i = i;
}

void shell_set_shell_it_j(size_t j) {
  shell_state_t *st = shell_get_shell_state();
  st->iterator_j = j;
}

void shell_set_shell_tc(size_t tc) {
  shell_state_t *st = shell_get_shell_state();
  st->session_token_count = tc;
}

void shell_set_shell_argc(size_t argc) {
  shell_state_t *st = shell_get_shell_state();
  st->argc = argc;
}

builtin_t *shell_get_shell_builtins(char *c) { return shell_get_builtin(c); }

void shell_set_shell_builtin(char *cmd) {
  builtin_t *handler = shell_get_shell_builtins(cmd);
  shell_state_t *st = shell_get_shell_state();
  if (handler != NULL)
    st->handler = handler;
}

void shell_run_shell_command(shell_state_t *st) {
  handler_t *hd = shell_get_shell_builtins((*st)->argv[0]);
  if (hd != NULL) {
    shell_run_shell_command((void *)hd, st->argc, (void **)st->argv);
  }
  shell_run_shell_command((void *)execvp, st->argc, (void **)st->argv);
}

void shell_set_shell_session_state(semantic_token_t **tokens,
                                   size_t token_count, size_t it_x, size_t it_i,
                                   size_t it_j, size_t ac, char **av) {

  shell_set_shell_tokens(tokens);
  shell_set_shell_it_x(it_x);
  shell_set_shell_it_i(it_i);
  shell_set_shell_it_j(it_j);
  shell_set_shell_tc(token_count);
  shell_set_shell_argc(ac);
  shell_set_shell_argv(av);
  shell_set_shell_builtin(av[0]);
}

// find the builtin to run or execvp
void shell_execution_handler(size_t argc, char **argv) {
  pid_t pid;
  pid = fork();

  switch (pid) {
  case -1:
    err_exit("fork", EXIT_FAILURE);
    break;
  case 0:
    /*
    builtin_t builtin = get_builtin(argv);
    if (builtin == NULL) {
      builtin(argc, argv);
    }
    */
    // walks built ins every time and compares, kind of bogus
    for (size_t i = 0; i < MAX_BUILTINS; i++) {
      // will move to hashmap in builtins.c
      if (builtins[i].name == NULL) {
        break;
      }
      if (strncmp(builtins[i].name, argv[0], 30) == MATCH) {
        handler_t hd = builtins[i].builtin;
        // if run errors how to handle?
        shell_run_shell_command(hd, argc, (void **)argv);
        break;
      }
    }

    // if $y not set, exit and do nothing
    if (argv[0][0] == '\0' && argc == 1) {
      exit(EXIT_SUCCESS);
    }
    // vp path aware
    if (execvp(argv[0], argv) == -1) {
      fprintf(stderr, "%s: command not found\n", *argv);
      _exit(EXIT_FAILURE);
    };
    exit(EXIT_SUCCESS);
    break;
  default:
    // in sys/wait.h for the parent
    wait(NULL);
  }
}
