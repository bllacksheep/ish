#ifndef _SHELL_H
#define _SHELL_H 1
#include "ht.h"
#include "parser.h"
#include <unistd.h>

void init_shell();
void shell_start_repl();
void shell_destroy_tokens(size_t, semantic_token_t **);
// should probsbly be command state now
void shell_set_command_state(semantic_token_t **, size_t, size_t, size_t,
                             size_t, size_t, char **);

void shell_set_shell_builtins(ht_table_t);
semantic_token_t **shell_get_token_vector(void);
ht_table_t shell_get_token_table(void);
#endif
