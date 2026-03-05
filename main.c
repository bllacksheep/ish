#include "ish.h"

typedef enum token {
  EXPRESSION,
  COMMAND,
  BUILTIN,
} semantic_type_t;

enum ERRORS {
  ERRNOBYTES = 0,
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

typedef struct parse_state {
  char *keyword;
  char **cmd_buffer;
  size_t *iterator;
  size_t kwlen;
  size_t scale;
} parse_state_t;

typedef struct parse_state parse_state_t;
void simple_parser(char *);
ssize_t read_input(char *);
void repl();
void exec_command(int, size_t, char **);
void mystrcspn(char **c);
void destroy_tokens(size_t, semantic_token_t **);
void destroy_args(size_t, char **);
void parse_expr(size_t, semantic_token_t **);
void parser_tokenize(char *, semantic_token_t **, size_t *);
void has_iterator(parse_state_t);
void parser_set_token_type(semantic_token_t *token);
void parser_set_token_val(char *buf, semantic_token_t *token);
int is_expression(char *buf);
int is_command(char *buf);
void tokenv_to_argv(size_t, char **, semantic_token_t **);
int echo(size_t, void **);
int fexit(size_t, void **);

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
    perror("repl flush");
    exit(EXIT_FAILURE);
  };
  switch (read_input(input)) {
  case 1:
    if (input[0] == '\n')
      break;
    break;
  case 0:
    perror("no bytes, strange should never happen...");
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
    perror("no input buffer");
    exit(ERRNOBUFFER);
  }
  ssize_t n = read(STDIN_FILENO, buf, MAX);
  if (n == -1) {
    perror("fail to read input");
    exit(EXIT_FAILURE);
  }
  if (n > 0)
    buf[n] = '\0';
  if (n == ERRNOBYTES) {
    perror("no bytes read from input");
    return ERRNOBYTES;
  }
  // newline in input command must be removed or break
  mystrcspn(&buf);
  return n;
}

// len bruh
size_t mylen(char **c) {
  if (c == NULL || *c == NULL) {
    perror("no buffer to take len");
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
    perror("no buffer to remove '\n'");
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
void has_iterator(parse_state_t s) {
  char *kw;
  if (s.keyword == NULL || s.cmd_buffer == NULL || *s.cmd_buffer == NULL ||
      s.iterator == NULL) {
    perror("no buffer to parse");
    exit(ERRNOBUFFER);
  }
  if (*s.iterator != 0) {
    perror("iterator should always be 0 at start of parsing");
    exit(ERRITERSTATE);
  }
  kw = s.keyword;
  for (int j = 0; j < s.kwlen; j++) {
    if (kw[j] == '0' || kw[j] == '1' || kw[j] == '2' || kw[j] == '3' ||
        kw[j] == '4' || kw[j] == '5' || kw[j] == '6' || kw[j] == '7' ||
        kw[j] == '8' || kw[j] == '9') {
      *s.iterator = *s.iterator * 10 + (kw[j] - '0');
      (*s.cmd_buffer)++;
      s.scale++;
    }
  }
}

// parse out the number from the start of a command if exists
// step the command pointer forward to pos arg 1 the actual command name
void parse_iterator(char **buf, size_t *iter) {
  if (buf == NULL || *buf == NULL || iter == NULL) {
    perror("parse iterator");
    exit(ERRNOBUFFER);
  }
  if (*iter != 0) {
    perror("iterator should always be 0");
    exit(ERRITERSTATE);
  }

  char capture[MAX + 1] = {0};
  char *p = *buf;
  size_t i = 0, s = 0;

  for (; *p != ' ' && *p != '\0'; capture[i++] = *p++)
    ;

  parse_state_t state = {
      .keyword = capture,
      .cmd_buffer = buf,
      .iterator = iter,
      .kwlen = i,
      .scale = s,
  };

  has_iterator(state);

  // skip space if present
  if ((*buf)[0] == ' ')
    (*buf)++;
}

int is_expression(char *buf) {
  if (buf == NULL) {
    perror("no buffer in expression detection");
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
    perror("no buffer is builtin");
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
    perror("no buffer when setting type");
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
void parser_tokenize(char *buf, semantic_token_t **tokenv, size_t *argn) {
  if (buf == NULL || tokenv == NULL) {
    perror("parse args");
    exit(EXIT_FAILURE);
  }

  semantic_token_t **token_vec = tokenv;
  char capture[MAX + 1] = {0};
  char *p = buf;
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
      perror("alloc token");
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
    perror("no buffer when getting val");
    exit(ERRNOBUFFER);
  }
  return p;
}

// parse x=1;y=2 expressions adding variable creation and reference x=$y
void parse_expr(size_t argc, semantic_token_t **tokenv) {
  if (tokenv == NULL || *tokenv == NULL) {
    perror("no expression buffer");
    exit(ERRNOBUFFER);
  }
  if (argc == 0) {
    perror("no expressions to parse");
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
      perror("no buffer when setting idle parser state");
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
    perror("no buffer when creating arg vector");
    exit(ERRNOBUFFER);
  }
  for (int i = 0; i < argc; i++) {
    // will fail due to last item being NULL
    if (argv[i] != NULL) {
      perror("arg vector pos should be empty");
      exit(ERRBUFFERINUSE);
    }

    if (tokenv[i] == NULL || tokenv[i]->buf == NULL) {
      perror("no token or token value buffer when setting arg vector");
      exit(ERRNOBUFFER);
    }
    // set pointer addresses of argv
    argv[i] = tokenv[i]->buf;
  }
}

// parser orchestroator pull together iterator + command + args
void simple_parser(char *c) {
  // real parsing logic on c goes here
  size_t iterator = 0;
  size_t arg_count = 0;
  // soft max on num args per command
  // used to derived argument types
  semantic_token_t *token_vector[MAX] = {0};
  // passed to exec command once references are resolved
  char *arg_vector[MAX] = {0};

  parse_iterator(&c, &iterator);
  // by convention use cmd ... args ... NULL terminate in NULL
  // return or set argc, command must end with NULL see execv
  parser_tokenize(c, token_vector, &arg_count);
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
    perror("fork");
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
        perror("i.sh: command not found");
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

int main(void) {

  while (1) {
    repl();
  }

  return 0;
}
