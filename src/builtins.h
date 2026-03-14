#ifndef _ISH_H
#define _ISH_H 1

#include <unistd.h>
#define MAX 100

typedef struct semantic_token semantic_token_t;
typedef struct builtin builtin_t;
typedef struct parse_state parse_state_t;

typedef struct parse_state {
  const char *capture;
  const char **buf;
  size_t *iterator;
  const size_t kwlen;
} parse_state_t;

typedef enum token {
  EXPRESSION,
  COMMAND,
  BUILTIN,
} semantic_type_t;

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

typedef int (*handler_t)(size_t, void **);

void shell_simple_parser(const char *);
void shell_execution_pipeline();
builtin_t *get_builtins(void);
ssize_t read_input(char *);
void repl();
void shell_execution_handler(size_t, char **);
void shell_run_command(handler_t callback, size_t argc, void **argv);
void mystrcspn(char **);
void shell_destroy_tokens(size_t, semantic_token_t **);
void destroy_args(size_t, char **);
void shell_parser_evaluate_expressions(size_t, semantic_token_t **);
void shell_parser_evaluate_iterator(const char **, size_t *);
void shell_parser_create_tokens(const char *, semantic_token_t **, size_t *);
void has_iterator(parse_state_t);
void shell_parser_set_token_type(semantic_token_t *);
void shell_parser_set_token_val(char *, semantic_token_t *);
int is_expression(char *);
int is_command(char *);
int is_builtin(char *);
void shell_parser_promote_tokens_to_argv(size_t *, char **,
                                         semantic_token_t **);
// builtins
int echo(size_t, void **);
int free_exit(size_t, void **);
int err_exit(char *, unsigned);
int unset(size_t, void **);
#endif
