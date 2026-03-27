#ifndef _BUILTINS_H
#define _BUILTINS_H 1
#include "ht.h"
#include <unistd.h>

typedef int (*handler_t)(size_t, void **);

ht_table_t bt_create_table(void);
size_t bt_get_fn_count(void);
void bt_init_builtins(ht_table_t);
enum match bt_is_builtin(char *);
handler_t bt_get_fn(ht_table_t, char *);
int echo(size_t, void **);
int quit(size_t, void **);
int unset(size_t, void **);
#endif
