#ifndef _BUILTINS_H
#define _BUILTINS_H 1
#include <unistd.h>

typedef struct builtin {
  int (*builtin)(size_t, void *[]);
  char *name;
} builtin_t;

typedef int (*handler_t)(size_t, void **);

builtin_t *get_builtins(void);
int echo(size_t, void **);
int quit(size_t, void **);
int unset(size_t, void **);
#endif
