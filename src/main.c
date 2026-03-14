#include "shell.h"

#ifndef TEST
int main(void) {

  while (1) {
    shell_repl();
  }

  return 0;
}
#endif
