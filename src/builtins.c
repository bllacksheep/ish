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

builtin_t builtins[MAX_NUM_BUILTINS] = {
    {echo, "echo"},
    {quit, "exit"},
    {quit, "q"},
    {unset, "unset"},
};

builtin_t *get_builtins(void) { return builtins; }

// free and exit
int quit(size_t argc, void **argv) {
  shell_destroy_tokens(argc, (semantic_token_t **)argv);
  exit(EXIT_SUCCESS);
  return 0;
}

int unset(size_t argc, void **argv) { return ht_del_var(argv[1]); }

int is_builtin(char *buf) {
  if (buf == NULL) {
    err_exit("no buffer in builtin", ERRNOBUFFER);
  }
  builtin_t *b = get_builtins();
  for (int i = 0; i < MAX_NUM_BUILTINS && b[i].name != NULL; i++) {
    if (strcmp(buf, b[i].name) == MATCH)
      return MATCH;
  }
  return !MATCH;
}

int echo(size_t argc, void **argv) {
  char **args = (char **)argv;
  for (int i = 1; i < argc; i++)
    printf("%s ", args[i]);
  putchar('\n');
  return 0;
}
