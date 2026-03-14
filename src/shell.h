#ifndef _SHELL_H
#define _SHELL_H 1
#include "parser.h"
#include <unistd.h>

void shell_repl();
void shell_destroy_tokens(size_t, semantic_token_t **);
void shell_set_shell_session_state(semantic_token_t **, size_t, size_t, size_t,
                                   size_t, size_t, char **);
#endif
