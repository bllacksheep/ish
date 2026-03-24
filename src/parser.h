#ifndef _PARSER_H
#define _PARSER_H 1
#include "ht.h"
#include <unistd.h>
typedef struct semantic_token semantic_token_t;

ht_table_t parser_create_table(void);
void parser_destroy_tokens(size_t, semantic_token_t **);
void parser_copy_tokens(semantic_token_t **, semantic_token_t **, size_t);
void parser_simple_parser(const char *);
#endif
