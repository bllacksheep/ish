#include "ht.h"
#include "ish.h"
#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/wait.h>
#include <unistd.h>

enum ERRORS {
  ERRNOBYTES = 200,
  ERRNOBUFFER,
  ERRITERSTATE,
  ERRNOEXPR,
  ERRALLOC,
  ERRBUFFERINUSE,
  ERRNOTOKEN,
  ERRNOTOKENCOUNT,
};

builtin_t builtins[MAX] = {
    {echo, "echo"},
    {free_exit, "exit"},
    {free_exit, "q"},
    {unset, "unset"},
};

builtin_t *get_builtins(void) { return builtins; }

// free and exit
int free_exit(size_t argc, void **argv) {
  shell_destroy_tokens(argc, (semantic_token_t **)argv);
  exit(EXIT_SUCCESS);
  return 0;
}

int unset(size_t argc, void **argv) { return ht_del_var(argv[1]); }

int err_exit(char *msg, unsigned err) {
  fprintf(stderr, "i.sh: %s: code %d\n", msg, err);
  exit(err);
}

void repl() {
  char input[MAX + 1] = {0};
  printf("> ");
  if (fflush(stdout) == EOF) {
    err_exit("repl flush", EXIT_FAILURE);
  };
  switch (read_input(input)) {
  case 1:
    if (input[0] == '\n')
      break;
    break;
  case 0:
    fprintf(stderr, "i.sh: no bytes, strange, should never happen...\n");
    return;
    break;
  default:
    // will move to parser.c
    shell_simple_parser(input);
    break;
  }
}

// read in the input and remove any newline at the end of the command
ssize_t read_input(char *buf) {
  if (buf == NULL) {
    err_exit("no input buffer", ERRNOBUFFER);
  }
  ssize_t n = read(STDIN_FILENO, buf, MAX);
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

// takes state but discards it, embedded in the state is a pointer "iter"
// which when set, propagates back to the caller, even though
// the rest of the data is destroyed on the stack frame
void has_iterator(const parse_state_t s) {
  // the capture is the potential iterator, captured in the caller
  if (s.capture == NULL || s.buf == NULL || *s.buf == NULL) {
    err_exit("no buffer to parse", ERRNOBUFFER);
  }

  if (s.iterator == NULL) {
    err_exit("iterator should not be NULL", ERRITERSTATE);
  }

  if (*s.iterator != 0) {
    err_exit("iterator should be 0 at the start of parsing", ERRITERSTATE);
  }

  const char *it = s.capture;
  // convert each string '1' and '0' in "10" to decimal 10
  for (int j = 0; j < s.kwlen; j++) {
    if (it[j] >= '0' && it[j] <= '9') {
      *s.iterator = *s.iterator * 10 + (it[j] - '0');
      // walk command buffer past the iterator if exists
      (*s.buf)++;
    } else {
      break;
    }
  }
}

// parse out the number from the start of a command if exists
// step the command pointer forward to pos arg 1 the actual command name
void shell_parser_evaluate_iterator(const char **buf, size_t *iter) {
  if (buf == NULL || *buf == NULL) {
    err_exit("parse iterator", ERRNOBUFFER);
  }
  if (iter == NULL || *iter != 0) {
    err_exit("iterator should always be 0", ERRITERSTATE);
  }

  char capture[MAX + 1] = {0};
  const char *p = *buf;
  const char **input = buf;
  size_t i = 0;

  for (; *p != ' ' && *p != '\0'; capture[i++] = *p++)
    ;

  parse_state_t state = {
      .capture = capture,
      .buf = input,
      .iterator = iter,
      .kwlen = i,
  };

  has_iterator(state);

  // skip space if present
  if ((*buf)[0] == ' ')
    (*buf)++;
}

int is_expression(char *buf) {
  if (buf == NULL) {
    err_exit("no buffer in expression", ERRNOBUFFER);
  }
  // assume is at least an attempted expression
  while (*buf != '\0') {
    if (*buf == '$' || *buf == '=')
      return MATCH;
    buf++;
  }
  return !MATCH;
}

int is_builtin(char *buf) {
  if (buf == NULL) {
    err_exit("no buffer in builtin", ERRNOBUFFER);
  }
  builtin_t *b = get_builtins();
  for (int i = 0; i < MAX && b[i].name != NULL; i++) {
    if (strcmp(buf, b[i].name) == MATCH)
      return MATCH;
  }
  return !MATCH;
}

// set the type of the member argv
void shell_parser_set_token_type(semantic_token_t *token) {
  if (token->buf == NULL) {
    err_exit("no buffer when settting type", ERRNOBUFFER);
  }
  if (is_expression(token->buf) == MATCH) {
    token->type = EXPRESSION;
    return;
  }
  if (is_builtin(token->buf) == MATCH) {
    token->type = BUILTIN;
    return;
  }
  token->type = COMMAND;
  return;
}

// set the val of the member argv
void shell_parser_set_token_val(char *buf, semantic_token_t *token) {
  if (token->buf != NULL)
    free(token->buf);
  token->buf = strdup(buf);
}

// create official argc argv used with exec
void shell_parser_create_tokens(const char *buf, semantic_token_t **tokenv,
                                size_t *argn) {
  if (buf == NULL || tokenv == NULL) {
    err_exit("parse args", EXIT_FAILURE);
  }

  semantic_token_t **token_vec = tokenv;
  char capture[MAX + 1] = {0};
  const char *p = buf;
  size_t argc = 0;

  while (*p != '\0') {
    if (*p == ' ')
      p++;
    for (size_t i = 0; *p != ';' && *p != '\0'; p++) {
      capture[i++] = *p;
    }
    // now that we have work bounded by ' ' or '\0' it's a token
    if (*token_vec != NULL)
      free(*token_vec);

    *token_vec = calloc(1, sizeof(semantic_token_t));
    if (*token_vec == NULL) {
      err_exit("alloc token", ERRALLOC);
    }

    shell_parser_set_token_val(capture, *token_vec);
    shell_parser_set_token_type(*token_vec);

    // count as arg
    argc++;
    // next token
    token_vec++;
    if (*p == ';' || *p == ' ') {
      p++;
      // look ahead for next space
      if (*p + 1 == ' ')
        p++;
    }
    memset(capture, 0, sizeof(capture));
  }
  // set the last arg to NULL required by execv family of functions
  tokenv[argc + 1] = NULL;
  *argn = argc;
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

// not used right now
// destroy args allocated with strdup
void destroy_args(size_t argc, char **argv) {
  for (int i = 0; i < argc; i++) {
    if (argv[i] != NULL)
      free(argv[i]);
  }
}

// semi-colon ; has to be handled before calling parse_expr and parse_expr needs
// to be called for each colon separated expr
// parse x=1;y=2 expressions adding variable creation and reference x=$y
void shell_parser_evaluate_expressions(size_t argc, semantic_token_t **tokenv) {
  if (tokenv == NULL || *tokenv == NULL) {
    err_exit("no expression buffer", ERRNOBUFFER);
  }
  if (argc == 0) {
    err_exit("no expression to parse", ERRNOEXPR);
  }

  char key[MAX + 1] = {0};
  char val[MAX + 1] = {0};

  enum {
    IDLE,
    CREATEKEY,
    CREATEVALUE,
    GETVALUE,
    SETVALUE,
    ERROR,
    NEXT,
    DONE,
  } state = IDLE;

  char *curr_token_pos = NULL;
  semantic_token_t **token_vec = tokenv;

  for (int i = 0; *token_vec != NULL; i++) {
    if ((*token_vec)->buf == NULL) {
      err_exit("no buffer when seting idle parse state", ERRNOBUFFER);
    }

    // walk tokens - resolve expression, skip the rest
    while ((*token_vec)->type != EXPRESSION) {
      token_vec++;
      if (*token_vec == NULL)
        // exhausted tokens at this point nothing to do
        return;
    }

    switch (state) {
    case IDLE:
      // first token assignment or next token, not breaking in token like 'x=1
      // $y'
      if (curr_token_pos == NULL || *curr_token_pos != ' ')
        curr_token_pos = (*token_vec)->buf;
      // continue to eval curr token if ' ' move beyond ' '
      if (*curr_token_pos == ' ')
        curr_token_pos++;

      if (isalpha(*curr_token_pos)) {
        key[i] = *curr_token_pos;
        curr_token_pos++;
        state = CREATEKEY;
        break;
      }
      if (*curr_token_pos == '$') {
        // move past $
        curr_token_pos++;
        key[i] = *curr_token_pos;
        // move past captured token above ^
        curr_token_pos++;
        state = GETVALUE;
        break;
      }
      state = ERROR;
      break;
    case GETVALUE:
      if (isalpha(*curr_token_pos)) {
        key[i] = *curr_token_pos;
        curr_token_pos++;
        break;
      }
      if (*curr_token_pos == '\0') {
        // currently set to input buffer now being resolved to value
        /*
        if ((*token_vec)->buf != NULL)
          free((*token_vec)->buf);
        */
        (*token_vec)->buf = (char *)ht_get_var(key);
        (*token_vec)->type = COMMAND;
        state = DONE;
        break;
      }
      break;
    case CREATEKEY:
      if (isalpha(*curr_token_pos)) {
        key[i] = *curr_token_pos;
        curr_token_pos++;
        break;
      }
      if (*curr_token_pos == '=') {
        // next increment will be to 0
        i = -1;
        state = CREATEVALUE;
        // skip '='
        curr_token_pos++;
        break;
      }
      if (*curr_token_pos == '$') {
        // feel like I need to reset key here
        curr_token_pos++;
        state = GETVALUE;
      }
      state = ERROR;
      break;
    case CREATEVALUE:
      if (isalpha(*curr_token_pos) || isdigit(*curr_token_pos)) {
        val[i] = *curr_token_pos;
        curr_token_pos++;
        break;
      }
      if (*curr_token_pos == '\0' || *curr_token_pos == ' ') {
        // retain shell session variable state here
        // add value to hash table
        if (ht_put_var(key, val) == EXIT_SUCCESS) {
          // since this is an expression this token is no longer useful
          state = NEXT;
          break;
        }
        state = ERROR;
        break;
      }
      // if $x then x should already be in ht
      if (*curr_token_pos == '$') {
        curr_token_pos++;
        val[i] = *curr_token_pos;
        state = GETVALUE;
        break;
      }
      state = ERROR;
      break;
    case ERROR:
      return;
    case NEXT:
      i = -1;
      memset(key, 0, sizeof(key));
      memset(val, 0, sizeof(val));
      // if buffer exausted next token, else potentially input 'x=1 $y'
      if (*curr_token_pos == '\0')
        token_vec++;
      state = IDLE;
      break;
    case DONE:
      return;
    default:
      return;
      break;
    }
  }
}

int echo(size_t argc, void **argv) {
  char **args = (char **)argv;
  for (int i = 1; i < argc; i++)
    printf("%s ", args[i]);
  putchar('\n');
  return 0;
}

void shell_parser_promote_tokens_to_argv(size_t *argc, char **argv,
                                         semantic_token_t **tokenv) {
  if (*argc == 0) {
    err_exit("no token count when creating arg vector", ERRNOTOKENCOUNT);
  }
  if (argv == NULL) {
    err_exit("no buffer when creating arg vector", ERRNOBUFFER);
  }
  if (tokenv == NULL) {
    err_exit("no tokens when creating arg vector", ERRNOTOKEN);
  }

  // if command expression command expression passed in, argc should be 2
  // counts commands only
  size_t cmdc = 0;

  for (int i = 0; i < *argc; i++) {
    if (tokenv[i] == NULL) {
      err_exit("no token when creating arg vector", ERRNOTOKEN);
    }

    // will fail due to last item being NULL
    if (argv[i] != NULL) {
      err_exit("arg vector pos should be empty", ERRBUFFERINUSE);
    }

    if (tokenv[i]->type != EXPRESSION) {
      if (tokenv[i] != NULL) {
        argv[cmdc++] = tokenv[i]->buf;
      }
    }
  }
  // decrement argc if tokens were pruned
  *argc = cmdc;
}

typedef struct ish_state {
  semantic_token_t **session_tokens;
  size_t session_token_count;
  size_t iterator_x; // user provided at cli
  size_t iterator_i; // shell builtin 1
  size_t iterator_j; // shell builtin 2
                     // builtins
                     // environment variables
  size_t argc;
  char *argv[MAX];
  handler_t handler;
} shell_state_t;

// use getter here
static shell_state_t the_ish_shell_state = {0};

// populate env vars and builtins here
shell_state_t *shell_get_shell_state(void) {
  // shell_check_shell_state();
  // shell_init_shell_state();
  return &the_ish_shell_state;
}

void shell_execution_pipeline() {
  // if private can only hold a pointer
  shell_state_t *ish = shell_get_shell_state();

  // should run at least once
  for (int i = 0; i < (ish->iterator_x == 0 ? 1 : ish->iterator_x); i++) {
    shell_execution_handler(ish->argc, ish->argv);
  }
  // shell_clean_shell_state();
}

void run(handler_t callback, size_t argc, void **argv) { callback(argc, argv); }

void shell_set_shell_state(semantic_token_t **tokens, size_t token_count,
                           size_t it_x, size_t it_i, size_t it_j, size_t ac,
                           char **av, handler_t handle) {

  shell_state_t *ish = shell_get_shell_state();
  // shell_set_shell_tokens(ish, tokens)
  ish->session_tokens = tokens;
  // shell_set_shell_it_x(ish, it_x)
  ish->iterator_x = it_x;
  // shell_set_shell_it_i(ish, it_i)
  ish->iterator_i = it_i;
  // shell_set_shell_j(ish, it_j)
  ish->iterator_j = it_j;
  // shell_set_shell_tc(ish, token_count)
  ish->session_token_count = token_count;
  // shell_set_shell_argc(ish, ac)
  ish->argc = ac;
  // shell_set_shell_argv(ish, argv)
  ish->argv = av; // copy proper
  // shell_set_command_handler(ish);
  // should set shell handler here, and promote vars
}

// parser orchestroator pull together iterator + command + args
void shell_simple_parser(const char *buf) {
  size_t it = 0;
  size_t tc = 0;
  size_t argc = 0;
  char *argv[MAX] = {0};

  // soft max on num args per command
  semantic_token_t *tvec[MAX] = {0};
  const char *input = buf;
  shell_parser_evaluate_iterator(&input, &it);
  shell_parser_create_tokens(input, tvec, &tc);
  shell_parser_evaluate_expressions(tc, tvec);
  shell_parser_promote_tokens_to_argv(&argc, argv, tvec);

  size_t i = 0;
  size_t j = 0;

  shell_set_shell_state(tvec, tc, it, i, j, argc, argv);

  shell_execution_pipeline();
  // shell_destroy_tokens(tc, tvec);
  return;
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
    for (size_t i = 0; i < MAX; i++) {
      // will move to hashmap in builtins.c
      if (builtins[i].name == NULL) {
        break;
      }
      if (strncmp(builtins[i].name, argv[0], 30) == MATCH) {
        handler_t hd = builtins[i].builtin;
        // if run errors how to handle?
        run(hd, argc, (void **)argv);
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

#ifndef TEST
int main(void) {

  while (1) {
    repl();
  }

  return 0;
}
#endif
