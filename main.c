#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/wait.h>
#include <unistd.h>
#define MAX 100

typedef struct parse_state parse_state_t;
void parser(char *);
ssize_t read_input(char *);
void repl();
void exec_command(size_t, char **);
void mystrcspn(char **c);
void destroy_args(size_t, char **);
void parse_expr(size_t, char **);
void has_iterator(parse_state_t);

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
    return;
    break;
  default:
    parser(input);
    break;
  }
}

enum ERRORS {
  NOBYTES = 0,
  NOBUFFER,
  ITERSTATE,
  NOEXPR,
};

// read in the input and remove any newline at the end of the command
ssize_t read_input(char *buf) {
  if (buf == NULL) {
    perror("no input buffer");
    exit(NOBUFFER);
  }
  ssize_t n = read(STDIN_FILENO, buf, MAX);
  if (n == -1) {
    perror("fail to read input");
    exit(EXIT_FAILURE);
  }
  if (n > 0)
    buf[n] = '\0';
  if (n == NOBYTES) {
    perror("no bytes read from input");
    return NOBYTES;
  }
  // newline in input command must be removed or break
  mystrcspn(&buf);
  return n;
}

// len bruh
size_t mylen(char **c) {
  if (c == NULL || *c == NULL) {
    perror("no buffer to take len");
    exit(NOBUFFER);
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
    exit(NOBUFFER);
  }
  size_t len = 0;
  while ((*c)[len] != '\n')
    len++;
  (*c)[len] = '\0';
}

typedef struct parse_state {
  char *keyword;
  char **cmd_buffer;
  char *curr_buf_pos;
  size_t *iterator;
  size_t kwlen;
  size_t scale;
} parse_state_t;

void has_iterator(parse_state_t s) {
  char *kw;
  if (s.keyword == NULL || s.cmd_buffer == NULL || *s.cmd_buffer == NULL ||
      s.curr_buf_pos == NULL || s.iterator == NULL) {
    perror("no buffer to parse");
    exit(NOBUFFER);
  }
  if (*s.iterator != 0) {
    perror("iterator should always be 0 at start of parsing");
    exit(ITERSTATE);
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
    exit(NOBUFFER);
  }
  if (*iter != 0) {
    perror("iterator should always be 0");
    exit(ITERSTATE);
  }

  char capture[MAX + 1] = {0};
  char *p = *buf;
  size_t i = 0, s = 0;

  for (; *p != ' ' && *p != '\0'; capture[i++] = *p++)
    ;

  parse_state_t state = {
      .keyword = capture,
      .cmd_buffer = buf,
      .curr_buf_pos = p,
      .iterator = iter,
      .kwlen = i,
      .scale = s,
  };

  has_iterator(state);

  // skip space if present
  if ((*buf)[0] == ' ')
    (*buf)++;
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

// destroy args allocated with strdup
void destroy_args(size_t argc, char **argv) {
  for (int i = 0; i < argc; i++) {
    if (argv[i] != NULL)
      free(argv[i]);
  }
}

char *getval(char *k) {
  char dummy[] = "1234";
  char *p = malloc(strlen(dummy) + 1);
  if (p == NULL) {
    perror("no buffer");
    exit(NOBUFFER);
  }
  int i = 0;
  while (dummy[i] != '\0') {
    p[i] = dummy[i];
    i++;
  }
  p[i] = '\0';
  return p;
}

// parse x=1;y=2 expressions adding variable creation and reference x=$y
void parse_expr(size_t argc, char **argv) {
  if (argv == NULL || *argv == NULL) {
    perror("no expression buffer");
    exit(NOBUFFER);
  }
  if (argc == 0) {
    perror("no expressions to parse");
    exit(NOEXPR);
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

  char *arg = NULL;

  for (int i = 0; *argv != NULL; i++) {
    switch (state) {
    case IDLE:
      arg = *argv;
      if (isalpha(*arg)) {
        key[i] = *arg;
        arg++;
        state = CREATEKEY;
        break;
      }
      if (*arg == '$') {
        arg++;
        key[i] = *arg;
        state = GETVALUE;
        break;
      }
      state = ERROR;
      break;
    case GETVALUE:
      if (isalpha(*arg)) {
        key[i] = *arg;
        arg++;
        break;
      }
      if (*arg == '\0') {
        puts(getval(key));
        state = DONE;
        break;
      }
      break;
    case CREATEKEY:
      if (isalpha(*arg)) {
        key[i] = *arg;
        arg++;
        break;
      }
      if (*arg == '=') {
        // add key to hash table
        puts(key);
        // next increment will be to 0
        i = -1;
        state = CREATEVALUE;
        // skip '='
        arg++;
        break;
      }
      state = ERROR;
      break;
    case CREATEVALUE:
      if (isalpha(*arg)) {
        val[i] = *arg;
        arg++;
        break;
      }
      if (*arg == '\0') {
        // add value to hash table
        puts(val);
        state = NEXT;
        break;
      }
      if (*arg == '$') {
        arg++;
        val[i] = *arg;
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
      argv++;
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

// parser orchestroator pull together iterator + command + args
void parser(char *c) {
  // real parsing logic on c goes here
  size_t iterator = 0;
  size_t arg_count = 0;
  // soft max on num args per command
  char *arg_vector[MAX] = {0};

  parse_iterator(&c, &iterator);
  // by convention use cmd ... args ... NULL terminate in NULL
  // return or set argc, command must end with NULL see execv
  parse_args(c, arg_vector, &arg_count);
  parse_expr(arg_count, arg_vector);
  /*
  printf("num args %zu\n", arg_count);
  printf("iterator %u\n", iterator);
  fflush(stdout);
  */

  // handle builtins here
  if (strcmp(c, "exit") == 0)
    exit(0);
  if (strcmp(c, "q") == 0)
    exit(0);
  if (iterator) {
    for (int i = 0; i < iterator; i++) {
      exec_command(arg_count, arg_vector);
    }
    destroy_args(arg_count, arg_vector);
    return;
  }
  exec_command(arg_count, arg_vector);
  destroy_args(arg_count, arg_vector);
  return;
}

// fork exec
void exec_command(size_t argc, char **argv) {
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
