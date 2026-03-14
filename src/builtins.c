#include "builtins.h"
#include "errors.h"
#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/wait.h>
#include <unistd.h>

builtin_t builtins[MAX] = {
    {echo, "echo"},
    {free_exit, "exit"},
    {free_exit, "q"},
    {unset, "unset"},
};

builtin_t *get_builtins(void) { return builtins; }

// free and exit
int free_exit(size_t argc, void **argv) {
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
  for (int i = 0; i < MAX && b[i].name != NULL; i++) {
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
