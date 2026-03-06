#ifndef _HT_H
#define _HT_H 1
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef struct ht_item {
  char *key;
  char *value;
} ht_item_t;

#define HT_MAX 1000

typedef ht_item_t ht_table_t[HT_MAX];

static ht_table_t *ht_init(void);
ht_table_t *ht_get_table(void);
int ht_get_var(char *);
int ht_put_var(char *, char *);
int ht_del_var(char *);
static int ht_set_key(ht_item_t *, char *);
static int ht_set_val(ht_item_t *, char *);

enum ht_errors {
  ERRHTINIT = 100,
  ERRHTGET,
  ERRHTINS,
  ERRHTDEL,
};

static ht_table_t *ht_table = NULL;

static ht_table_t *ht_init() {
  if (ht_table != NULL) {
    return ht_table;
  }
  ht_table = calloc(1, sizeof(ht_table_t));
  if (ht_table == NULL) {
    fprintf(stderr, "i.sh: failed to initialize ht table, code: %d", ERRHTINIT);
    exit(ERRHTINIT);
  }
  return ht_table;
}

ht_table_t *ht_get_table() {
  if (ht_table == NULL) {
    return ht_init();
  }
  return ht_table;
}

int ht_get_var(char *k) {
  ht_table_t *ht = ht_get_table();
  if (ht == NULL) {
    fprintf(stderr, "i.sh: failed to initialize ht table, code: %d", ERRHTGET);
    exit(ERRHTGET);
  }
  return EXIT_SUCCESS;
}

static int ht_set_val(ht_item_t *item, char *v) {
  if (item == NULL || v == NULL) {
    fprintf(stderr, "i.sh: failed to set value %s, code: %d", v, ERRHTINS);
    return EXIT_FAILURE;
  }

  // if not null, and the same value, overwrite, else .... something else
  item->value = strdup(v);
  if (item->value == NULL) {
    fprintf(stderr, "i.sh: failed to initialize value %s, code: %d", v,
            ERRHTINS);
    return EXIT_FAILURE;
  }
  return EXIT_SUCCESS;
}

static int ht_set_key(ht_item_t *item, char *k) {
  if (item == NULL || k == NULL) {
    fprintf(stderr, "i.sh: failed to set key %s, code: %d", k, ERRHTINS);
    return EXIT_FAILURE;
  }

  // if not null, and the same value, overwrite, else .... something else
  item->key = strdup(k);
  if (item->key == NULL) {
    fprintf(stderr, "i.sh: failed to initialize key %s, code: %d", k, ERRHTINS);
    return EXIT_FAILURE;
  }

  return EXIT_SUCCESS;
}

int ht_put_var(char *k, char *v) {
  ht_table_t *ht = ht_get_table();
  if (ht == NULL) {
    fprintf(stderr, "i.sh: failed to initialize ht table, code: %d", ERRHTINS);
    exit(ERRHTINS);
  }

  ht_item_t *ht_item = ht_lookup_var(key);

  if (ht_set_key(ht_item, k) != EXIT_SUCCESS) {
    return EXIT_FAILURE;
  }
  if (ht_set_val(ht_item, v) != EXIT_SUCCESS) {
    return EXIT_FAILURE;
  }

  return EXIT_SUCCESS;
}

int ht_del_var(char *k) {
  ht_table_t *ht = ht_get_table();
  if (ht == NULL) {
    fprintf(stderr, "i.sh: failed to initialize ht table, code: %d", ERRHTDEL);
    exit(ERRHTDEL);
  }

  return 0;
}

#endif
