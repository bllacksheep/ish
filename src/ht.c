#include "ht.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define HT_MAX 1000
#define HT_MAX_KEY_LEN 20

enum ht_errors {
  ERRHTINIT = 100,
  ERRHTGET,
  ERRHTINS,
  ERRHTDEL,
  ERRHTNOKEYLEN,
  ERRHTNOTABLE,
  ERRHTNOBUF,
  ERRHTNOITEM,
};

typedef struct ht_item {
  char *key;
  char *value;
} ht_item_t;

typedef ht_item_t ht_table_t[HT_MAX];
static ht_table_t *ht_table = NULL;

static ht_table_t *table_init(void);
static ht_table_t *table_get(void);
static ht_item_t *item_lookup(const ht_table_t *, const char *, const size_t);
static unsigned item_hash(const char *, const size_t, const unsigned);
static size_t key_get_len(const char *);

static unsigned item_hash(const char *k, const size_t kl, const unsigned at) {
  return 0;
}

// take table, key and key length and return value
static ht_item_t *item_lookup(const ht_table_t *tbl, const char *item_key,
                              const size_t item_key_len) {

  if (tbl == NULL) {
    fprintf(stderr, "i.sh: ht no table, code: %d", ERRHTNOTABLE);
    exit(ERRHTNOTABLE);
  }

  if (item_key == NULL) {
    fprintf(stderr, "i.sh: ht no buffer, code: %d", ERRHTNOBUF);
    exit(ERRHTNOBUF);
  }

  if (item_key_len == 0) {
    fprintf(stderr, "i.sh: ht len 0, code: %d", ERRHTNOKEYLEN);
    exit(ERRHTNOKEYLEN);
  }

  unsigned attempt = 0;
  unsigned try = item_hash(item_key, item_key_len, attempt);

  ht_item_t *item = (ht_item_t *)&tbl[try];

  if (item == NULL) {
    fprintf(stderr, "i.sh: ht no item at index, code: %d", ERRHTNOITEM);
    exit(ERRHTNOITEM);
  }

  while (1) {
    if (attempt == HT_MAX)
      break;
    if (strcmp(item->key, item_key) == 0)
      return item;
    attempt++;
    // retry step by 1
    try = item_hash(item_key, item_key_len, attempt);
    item = (ht_item_t *)&tbl[try];
  }
  return NULL;
}

// initialize a table if not already initialized and return pointer to first
// elem
static ht_table_t *table_init(void) {
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

// return a table init if not exists
static ht_table_t *table_get(void) {
  if (ht_table == NULL)
    return table_init();
  return ht_table;
}

// caller must check is val NULL
const char *ht_get_var(const char *item_k) {
  if (item_k == NULL) {
    fprintf(stderr, "i.sh: ht no buffer, code: %d", ERRHTNOBUF);
    exit(ERRHTNOBUF);
  }

  ht_table_t *table = table_get();
  if (table == NULL) {
    fprintf(stderr, "i.sh: failed to get ht table, code: %d", ERRHTGET);
    exit(ERRHTGET);
  }

  size_t item_kl = key_get_len(item_k);
  ht_item_t *item = item_lookup(table, item_k, item_kl);
  if (item != NULL) {
    return item->value;
  }
  return NULL;
}

static size_t key_get_len(const char *k) {
  if (k == NULL) {
    fprintf(stderr, "i.sh: ht no buffer, code: %d", ERRHTNOBUF);
    exit(ERRHTNOBUF);
  }

  const size_t item_kl = strnlen(k, HT_MAX_KEY_LEN);

  if (item_kl == 0) {
    fprintf(stderr, "i.sh: failed to initialize key %s, code: %d", k, ERRHTINS);
    return EXIT_FAILURE;
  }
  return item_kl;
}

int ht_put_var(const char *item_k, const char *item_v) {

  if (item_k == NULL || item_v == NULL) {
    fprintf(stderr, "i.sh: ht no buffer, code: %d", ERRHTNOBUF);
    exit(ERRHTNOBUF);
  }

  ht_table_t *table = table_get();
  if (table == NULL) {
    fprintf(stderr, "i.sh: failed to get ht table, code: %d", ERRHTGET);
    exit(ERRHTGET);
  }

  size_t item_kl = key_get_len(item_k);
  ht_item_t *item = item_lookup(table, item_k, item_kl);

  if (item != NULL) {
    item->key = strdup(item_k);
    if (item->key == NULL) {
      fprintf(stderr, "i.sh: failed to initialize key %s, code: %d", item_k,
              ERRHTINS);
      return EXIT_FAILURE;
    }

    item->value = strdup(item_v);
    if (item->value == NULL) {
      fprintf(stderr, "i.sh: failed to initialize value %s, code: %d", item_v,
              ERRHTINS);
      return EXIT_FAILURE;
    }
  }
  return EXIT_SUCCESS;
}

// unset in shell, remove an item by freeing it's k,v and setting to NULL
int ht_del_var(const char *item_k) {
  ht_table_t *table = table_get();
  if (table == NULL) {
    fprintf(stderr, "i.sh: failed to initialize ht table, code: %d", ERRHTDEL);
    exit(ERRHTDEL);
  }

  return 0;
}
