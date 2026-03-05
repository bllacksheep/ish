#ifndef _ISH_H
#define _ISH_H 1

#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/wait.h>
#include <unistd.h>
#define MAX 100

typedef struct semantic_token semantic_token_t;
typedef struct builtin builtin_t;
typedef struct parse_state parse_state_t;

void simple_parser(char *);
ssize_t read_input(char *);
void repl();
void exec_command(int, size_t, char **);
void mystrcspn(char **);
void destroy_tokens(size_t, semantic_token_t **);
void destroy_args(size_t, char **);
void parse_expr(size_t, semantic_token_t **);
void parser_tokenize(char *, semantic_token_t **, size_t *);
void has_iterator(parse_state_t);
void parser_set_token_type(semantic_token_t *);
void parser_set_token_val(char *, semantic_token_t *);
int is_expression(char *);
int is_command(char *);
void tokenv_to_argv(size_t, char **, semantic_token_t **);
int echo(size_t, void **);
int fexit(size_t, void **);
#endif
