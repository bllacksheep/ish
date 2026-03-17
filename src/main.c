#include "builtins.h"
#include "ht.h"
#include "shell.h"

#ifndef TEST
int main(void) {
  ht_table_t bt = ht_create_table(bt_get_fn_count());

  bt_init_builtins(bt);
  shell_set_shell_builtins(bt);

  // shell state needs be propagated here
  while (1) {
    shell_start_repl();
  }

  return 0;
}
#endif
