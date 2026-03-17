#ifndef _HT_H
#define _HT_H 1
#include <unistd.h>
// forward declared
typedef struct ht_table *ht_table_t;

const char *ht_get_item(ht_table_t, const char *);
int ht_put_item(ht_table_t, const char *, const void *);
int ht_del_item(ht_table_t, const char *);
ht_table_t ht_create_table(size_t);

#endif
