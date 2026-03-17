#ifndef _BUILTINS_H
#define _BUILTINS_H 1
#include "ht.h"
#include <unistd.h>

typedef struct builtin builtin_t;

typedef int (*handler_t)(size_t, void **);

size_t bt_get_fn_count(void);
void bt_init_builtins(ht_table_t);
int echo(size_t, void **);
int quit(size_t, void **);
int unset(size_t, void **);
#endif
