#include "builtins.h"
#include "errors.h"
#include "ht.h"
#include "shell.h"
#include <stdio.h>
#include <stdlib.h>
#include <sys/wait.h>
#include <unistd.h>

#define MAX_NUM_BUILTINS 3

typedef struct bt_builtin {
  char *name;
  int (*handler)(size_t, void *[]);
} builtin_t;

static builtin_t builtins[MAX_NUM_BUILTINS] = {
    {"echo", echo},
    {"quit", quit},
    {"unset", unset},
};

builtin_t *bt_get_builtins(void) { return builtins; }

// free and exit
int quit(size_t argc, void **argv) {
  shell_destroy_tokens(argc, (semantic_token_t **)argv);
  exit(EXIT_SUCCESS);
  return 0;
}

int unset(size_t argc, void **argv);

int echo(size_t argc, void **argv) {
  char **args = (char **)argv;
  for (int i = 1; i < argc; i++)
    printf("%s ", args[i]);
  putchar('\n');
  return 0;
}

handler_t bt_get_fn(ht_table_t table, char *key) {
  void *handle = (void *)ht_get_item(table, key);
  return handle;
}

size_t bt_get_fn_count(void) {
  builtin_t *list_of_builtins = bt_get_builtins();
  return sizeof(*list_of_builtins);
}

void bt_init_builtins_table(ht_table_t table) {
  builtin_t *list_of_builtins = bt_get_builtins();
  for (size_t i = 0; i < MAX_NUM_BUILTINS; i++) {
    ht_put_item(table, list_of_builtins[i].name, list_of_builtins[i].handler,
                FUNCTION);
  }
}

void bt_create_table(void) {
  ht_table_t ht = ht_create_table(bt_get_fn_count());
  bt_init_builtins_table(ht);
  shell_set_shell_builtins(ht);
}
