#include "shell.h"
#include "builtins.h"
#include "errors.h"
#include "parser.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/wait.h>
#include <unistd.h>

#define MAX_INPUT_STREAM 100
#define MAX_ARGV_LENGTH 10

typedef struct ish_state {
  semantic_token_t **token_vector;
  size_t token_vec_count;
  size_t iterator_x; // user provided at cli
  size_t iterator_i; // shell builtin 1
  size_t iterator_j; // shell builtin 2
                     // builtins
                     // environment variables
  size_t argc;
  char *argv[MAX_ARGV_LENGTH];

  handler_t handler;
  // anchor pointer to tables, required by ht api
  ht_table_t builtins_table;
  ht_table_t token_table;
} shell_state_t;

// use getter here
static shell_state_t state = {0};

void shell_execution_pipeline();
ssize_t shell_read_input_stream(char *);
void shell_execution_handler(size_t, char **);
void shell_set_shell_state(semantic_token_t **, size_t, size_t, size_t, size_t,
                           size_t, char **);
void shell_run_shell_command(handler_t, size_t, void **);
void mystrcspn(char *const *);

void shell_start_repl() {
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
    // parser_destroy_tokens(tc, tvec);
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
size_t mylen(const char *const *c) {
  if (c == NULL || *c == NULL) {
    err_exit("no buffer to take len", ERRNOBUFFER);
  }
  size_t len = 0;
  while (c[len] != NULL)
    len++;
  return len;
}

// remove a newline by replace it with \0
void mystrcspn(char *const *c) {
  if (c == NULL || *c == NULL) {
    err_exit("no buffer to remove new line", ERRNOBUFFER);
  }
  size_t len = 0;
  while ((*c)[len] != '\n')
    len++;
  (*c)[len] = '\0';
}

// populate env vars and builtins here
shell_state_t *shell_get_shell_state(void) {
  // shell_check_shell_state();
  // shell_init_shell_state();
  return &state;
}

semantic_token_t **shell_state_get_token_vector(void) {
  shell_state_t *st = shell_get_shell_state();
  return st->token_vector;
}

void shell_execution_pipeline() {
  // if private can only hold a pointer
  shell_state_t *st = shell_get_shell_state();

  // shell functions will run this
  // should run at least once
  for (int i = 0; i < (st->iterator_x == 0 ? 1 : st->iterator_x); i++) {
    st->handler(st->argc, (void **)st->argv);
  }
  // shell_clean_shell_state();
}

void shell_state_set_input_argv(size_t argc, char **argv) {
  shell_state_t *st = shell_get_shell_state();
  memcpy(st->argv, argv, sizeof(*argv) * argc);
}

void shell_state_set_input_tokens(semantic_token_t **tokenvec, size_t count) {
  shell_state_t *st = shell_get_shell_state();
  st->token_vec_count = count;
  // assumes st->tokens is cleaned up
  parser_copy_tokens(st->token_vector, tokenvec, st->token_vec_count);
}

void shell_state_set_input_it_x(size_t x) {
  shell_state_t *st = shell_get_shell_state();
  st->iterator_x = x;
}

void shell_state_set_input_it_i(size_t i) {
  shell_state_t *st = shell_get_shell_state();
  st->iterator_i = i;
}

void shell_state_set_input_it_j(size_t j) {
  shell_state_t *st = shell_get_shell_state();
  st->iterator_j = j;
}

void shell_state_set_input_argc(size_t argc) {
  shell_state_t *st = shell_get_shell_state();
  st->argc = argc;
}

// needs to be set in builtins.c or not at all
/*
// handler set to execvp by default
void shell_set_shell_builtins(ht_table_t builtins) {
  shell_state_t *st = shell_get_shell_state();
  st->builtins = builtins;
}
*/

// set up default handler here
handler_t shell_state_get_default_handler() { return NULL; }

// find of feel like hanlder just needs to be void* at this point
void shell_state_set_input_handler(char *command) {
  shell_state_t *st = shell_get_shell_state();

  // get built bt name table + command
  /*
  handler_t handle = bt_get_fn(st->builtins, command);
  if (handle != NULL) {
    st->handler = handle;
  }
  */
  st->handler = shell_state_get_default_handler();
}

void shell_state_set_execution_context(semantic_token_t **tokens,
                                       size_t token_count, size_t it_x,
                                       size_t it_i, size_t it_j, size_t ac,
                                       char **av) {

  shell_state_set_input_tokens(tokens, token_count);
  shell_state_set_input_it_x(it_x);
  shell_state_set_input_it_i(it_i);
  shell_state_set_input_it_j(it_j);
  shell_state_set_input_argc(ac);
  shell_state_set_input_argv(ac, av);
  shell_state_set_input_handler(av[0]);
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

    /*
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
    */
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

void shell_state_set_token_table(ht_table_t ht) {
  shell_state_t *st = shell_get_shell_state();
  if (ht != NULL) {
    st->token_table = ht;
    return;
  }
  return;
}

ht_table_t shell_state_get_token_table(void) {
  shell_state_t *st = shell_get_shell_state();
  return st->token_table;
}

void shell_state_set_builtin_table(ht_table_t ht) {
  shell_state_t *st = shell_get_shell_state();
  if (ht != NULL) {
    st->builtins_table = ht;
    return;
  }
  return;
}

ht_table_t shell_state_get_builtin_table(void) {
  shell_state_t *st = shell_get_shell_state();
  return st->builtins_table;
}

void init_shell() {
  // create shell builtins table
  shell_state_set_builtin_table(bt_create_table());
  // create parser table
  shell_state_set_token_table(parser_create_table());
}
