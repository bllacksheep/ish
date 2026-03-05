#include "ish.h"
#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/wait.h>
#include <unistd.h>

typedef enum token {
  EXPRESSION,
  COMMAND,
  BUILTIN,
} semantic_type_t;

enum ERRORS {
  ERRNOBYTES = 1,
  ERRNOBUFFER,
  ERRITERSTATE,
  ERRNOEXPR,
  ERRALLOC,
  ERRBUFFERINUSE,
};

typedef struct semantic_token {
  char *buf;
  semantic_type_t type;
} semantic_token_t;

enum parser_matching {
  MATCH = 0,
};

typedef struct builtin {
  int (*builtin)(size_t, void *[]);
  char *name;
} builtin_t;

builtin_t builtins[MAX] = {
    {echo, "echo"},
    {fexit, "exit"},
    {fexit, "q"},
};

// free and exit
int fexit(size_t argc, void **argv) {
  destroy_tokens(argc, (semantic_token_t **)argv);
  exit(EXIT_SUCCESS);
  return 0;
}

void repl() {
  char input[MAX + 1] = {0};
  printf("> ");
  if (fflush(stdout) == EOF) {
    fprintf(stderr, "i.sh: repl flush code: %d\n", EXIT_FAILURE);
    exit(EXIT_FAILURE);
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
    simple_parser(input);
    break;
  }
}

// read in the input and remove any newline at the end of the command
ssize_t read_input(char *buf) {
  if (buf == NULL) {
    fprintf(stderr, "i.sh: no input buffer: %d\n", ERRNOBUFFER);
    exit(ERRNOBUFFER);
  }
  ssize_t n = read(STDIN_FILENO, buf, MAX);
  if (n == -1) {
    fprintf(stderr, "i.sh: failed to read input, code: %d\n", EXIT_FAILURE);
    exit(EXIT_FAILURE);
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
    fprintf(stderr, "i.sh: no buffer to take len, code: %d\n", ERRNOBUFFER);
    exit(ERRNOBUFFER);
  }
  size_t len = 0;
  while (c[len] != NULL)
    len++;
  return len;
}

// remove a newline by replace it with \0
void mystrcspn(char **c) {
  if (c == NULL || *c == NULL) {
    fprintf(stderr, "i.sh: no buffer to remove, code: %d\n", ERRNOBUFFER);
    exit(ERRNOBUFFER);
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
    fprintf(stderr, "i.sh: no buffer to parse, code: %d\n", ERRNOBUFFER);
    exit(ERRNOBUFFER);
  }

  if (s.iterator == NULL) {
    fprintf(stderr, "i.sh: iterator should not be NULL, code: %d\n",
            ERRITERSTATE);
    exit(ERRITERSTATE);
  }

  if (*s.iterator != 0) {
    fprintf(stderr,
            "i.sh: iterator value should always be 0 at start of parsing, "
            "code: %d\n",
            ERRITERSTATE);
    exit(ERRITERSTATE);
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
void parse_iterator(const char **buf, size_t *iter) {
  if (buf == NULL || *buf == NULL) {
    fprintf(stderr, "i.sh: parse iterator, code: %d\n", ERRNOBUFFER);
    exit(ERRNOBUFFER);
  }
  if (iter == NULL || *iter != 0) {
    fprintf(stderr, "i.sh: iterator should always be 0, code: %d\n",
            ERRITERSTATE);
    exit(ERRITERSTATE);
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
    fprintf(stderr, "i.sh: no buffer in expression, code %d\n", ERRNOBUFFER);
    exit(ERRNOBUFFER);
  }
  // assume is at least an attempted expression
  while (*buf != '\0') {
    if (*buf == '$')
      return MATCH;
    buf++;
  }
  return !MATCH;
}

int is_builtin(char *buf) {
  if (buf == NULL) {
    fprintf(stderr, "i.sh: no buffer in builtin, code %d\n", ERRNOBUFFER);
    exit(ERRNOBUFFER);
  }

  for (int i = 0; i < MAX && builtins[i].name != NULL; i++) {
    if (strcmp(buf, builtins[i].name) == MATCH)
      return MATCH;
  }
  return !MATCH;
}

// set the type of the member argv
void parser_set_token_type(semantic_token_t *token) {
  if (token->buf == NULL) {
    fprintf(stderr, "i.sh: no buffer when setting type, code %d\n",
            ERRNOBUFFER);
    exit(ERRNOBUFFER);
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
void parser_set_token_val(char *buf, semantic_token_t *token) {
  if (token->buf != NULL)
    free(token->buf);
  token->buf = strdup(buf);
}

// create official argc argv used with exec
void parser_tokenize(const char *buf, semantic_token_t **tokenv, size_t *argn) {
  if (buf == NULL || tokenv == NULL) {
    fprintf(stderr, "i.sh: parse args, code: %d\n", EXIT_FAILURE);
    exit(EXIT_FAILURE);
  }

  semantic_token_t **token_vec = tokenv;
  char capture[MAX + 1] = {0};
  const char *p = buf;
  size_t argc = 0;

  while (*p != '\0') {
    for (size_t i = 0; *p != ' ' && *p != '\0'; p++) {
      capture[i++] = *p;
    }
    // now that we have work bounded by ' ' or '\0' it's a token
    if (*token_vec != NULL)
      free(*token_vec);

    *token_vec = calloc(1, sizeof(semantic_token_t));
    if (*token_vec == NULL) {
      fprintf(stderr, "i.sh: alloc token, code: %d\n", ERRALLOC);
      exit(ERRALLOC);
    }
    parser_set_token_val(capture, *token_vec);
    parser_set_token_type(*token_vec);
    // count arg
    argc++;
    // skip space
    p++;
    // next token
    token_vec++;
    memset(capture, 0, sizeof(capture));
  }
  // set the last arg to NULL required by execv family of functions
  tokenv[argc + 1] = NULL;
  *argn = argc;
}

// destroy args allocated with strdup
void destroy_tokens(size_t tokenc, semantic_token_t **tokenv) {
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

char *getval(char *k) {
  char *p = strdup("1234");
  if (p == NULL) {
    fprintf(stderr, "i.sh: no buffer when getting val, code: %d\n",
            ERRNOBUFFER);
    exit(ERRNOBUFFER);
  }
  return p;
}

// parse x=1;y=2 expressions adding variable creation and reference x=$y
void parse_expr(size_t argc, semantic_token_t **tokenv) {
  if (tokenv == NULL || *tokenv == NULL) {
    fprintf(stderr, "i.sh: no expression buffer, code: %d\n", ERRNOBUFFER);
    exit(ERRNOBUFFER);
  }
  if (argc == 0) {
    fprintf(stderr, "i.sh: no expression to parse, code: %d\n", ERRNOEXPR);
    exit(ERRNOEXPR);
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
      fprintf(stderr,
              "i.sh: no buffer when setting idle parse state, code: %d\n",
              ERRNOBUFFER);
      exit(ERRNOBUFFER);
    }

    // walk tokens - resolve expression, skip the rest
    while ((*token_vec)->type != EXPRESSION) {
      token_vec++;
      if (*token_vec == NULL)
        // exhausted tokens at this point
        return;
    }

    switch (state) {
    case IDLE:
      curr_token_pos = (*token_vec)->buf;
      if (isalpha(*curr_token_pos)) {
        key[i] = *curr_token_pos;
        curr_token_pos++;
        state = CREATEKEY;
        break;
      }
      if (*curr_token_pos == '$') {
        curr_token_pos++;
        key[i] = *curr_token_pos;
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
        if ((*token_vec)->buf != NULL)
          free((*token_vec)->buf);
        (*token_vec)->buf = getval(key);
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
        // move to ins(k, v)
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
      if (isalpha(*curr_token_pos)) {
        val[i] = *curr_token_pos;
        curr_token_pos++;
        break;
      }
      if (*curr_token_pos == '\0') {
        // retain shell session variable state here
        // add value to hash table
        // insert(key, val);
        state = NEXT;
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

void tokenv_to_argv(size_t argc, char **argv, semantic_token_t **tokenv) {
  if (argv == NULL || tokenv == NULL) {
    fprintf(stderr, "i.sh: no buffer when creating arg vector, code: %d\n",
            ERRNOBUFFER);
    exit(ERRNOBUFFER);
  }
  for (int i = 0; i < argc; i++) {
    // will fail due to last item being NULL
    if (argv[i] != NULL) {
      fprintf(stderr, "i.sh: arg vector pos should be empty, code: %d\n",
              ERRBUFFERINUSE);
      exit(ERRBUFFERINUSE);
    }

    if (tokenv[i] == NULL || tokenv[i]->buf == NULL) {
      fprintf(stderr,
              "i.sh: no token or token value buffer when setting arg vector, "
              "code: %d\n",
              ERRNOBUFFER);
      exit(ERRNOBUFFER);
    }
    // set pointer addresses of argv
    argv[i] = tokenv[i]->buf;
  }
}

// parser orchestroator pull together iterator + command + args
void simple_parser(const char *buf) {
  // real parsing logic on c goes here
  size_t iterator = 0;
  size_t arg_count = 0;
  // soft max on num args per command
  // used to derived argument types
  semantic_token_t *token_vector[MAX] = {0};
  // passed to exec command once references are resolved
  char *arg_vector[MAX] = {0};

  const char *input = buf;

  parse_iterator(&input, &iterator);
  // by convention use cmd ... args ... NULL terminate in NULL
  // return or set argc, command must end with NULL see execv
  parser_tokenize(input, token_vector, &arg_count);
  parse_expr(arg_count, token_vector);

  // needed? *token type encoded in the vector pass back
  // simplify below
  tokenv_to_argv(arg_count, arg_vector, token_vector);

  // handle builtins here
  if (strcmp(arg_vector[0], "exit") == MATCH)
    fexit(arg_count, (void **)token_vector);
  if (strcmp(arg_vector[0], "q") == MATCH)
    fexit(arg_count, (void **)token_vector);

  // iterator loop
  for (int i = 0; i < (iterator == 0 ? 1 : iterator); i++) {
    if (strcmp(arg_vector[0], "echo") == MATCH) {
      // exec_command(COMMAND
      // exec_command(EXPRESSION
      // exec_command(BUILTIN
      exec_command(BUILTIN, arg_count, arg_vector);
      break;
    }
    exec_command(COMMAND, arg_count, arg_vector);
  }
  destroy_tokens(arg_count, token_vector);
  return;
}

// fork exec
void exec_command(int type, size_t argc, char **argv) {
  pid_t pid;
  pid = fork();

  // need state machine here for evaluating cmd
  // expressions first from token list
  // command or builtin last
  switch (pid) {
  case -1:
    fprintf(stderr, "i.sh: fork, code: %d\n", EXIT_FAILURE);
    exit(EXIT_FAILURE);
    break;
  case 0:
    switch (type) {
    case BUILTIN:
      // do O(1) look up in hash table but talk for now
      for (size_t i = 0; i < MAX; i++) {
        if (builtins[i].name == argv[0])
          builtins[i].builtin(argc, (void **)argv);
      }
      break;
    case EXPRESSION:
      echo(argc, (void **)argv);
      break;
    default:
      // vp path aware
      if (execvp(argv[0], argv) == -1) {
        fprintf(stderr, "i.sh: command not found, code: %d\n", EXIT_FAILURE);
        _exit(EXIT_FAILURE);
      };
      exit(EXIT_SUCCESS);
      break;
    }
    break;
  default:
    // in sys/wait.h for the parent
    wait(NULL);
    break;
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
