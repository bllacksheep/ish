#include "builtins.h"
#include "ht.h"
#include "shell.h"

void init_shell() {
  // create shell builtins table
  bt_create_table();
  // create parser table
  parser_create_table();
}

#ifndef TEST
int main(void) {
  init_shell();
  // shell state needs be propagated here
  while (1) {
    shell_start_repl();
  }

  return 0;
}
#endif
