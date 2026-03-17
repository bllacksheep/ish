#include "builtins.h"
#include "errors.h"
#include "ht.h"
#include "shell.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/wait.h>
#include <unistd.h>

#define MAX_NUM_BUILTINS 10

typedef struct ht_builtin {
  char *key;
  int (*value)(size_t, void *[]);
} ht_item_t;

// decoupled from ht.c
static ht_table_t *ht_table = NULL;

// free and exit
int quit(size_t argc, void **argv) {
  shell_destroy_tokens(argc, (semantic_token_t **)argv);
  exit(EXIT_SUCCESS);
  return 0;
}

int unset(size_t argc, void **argv) { return ht_del_item(argv[1]); }

int is_builtin(char *buf) {}

int echo(size_t argc, void **argv) {
  char **args = (char **)argv;
  for (int i = 1; i < argc; i++)
    printf("%s ", args[i]);
  putchar('\n');
  return 0;
}

handler_t bt_get_fn(ht_table_t table, char *key) {
  void *handle = (void *)ht_get_item(table, key);
}

size_t bt_get_fn_count(void);

void bt_init_builtins(ht_table_t table) {
  ht_put_item(table, "echo", echo);
  ht_put_item(table, "unset", unset);
  ht_put_item(table, "quit", exit);
}
