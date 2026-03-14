#include "ht.h"
#include "errors.h"
#include "ht_internal.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static char HT_TOMBSTONE_SENTINEL = 0;
static void *const HT_TOMBSTONE = &HT_TOMBSTONE_SENTINEL;

STATIC ht_table_t *ht_table = NULL;

// E n-1 s[i]**n-1-i
// i = 0
// hash = hash * prime + char
// exactly equal to above
// return a hash of a key of len len based on given prime
STATIC long long unsigned hash(const char *key, const size_t len,
                               const unsigned prime) {
  unsigned long long hash = 0;
  for (size_t i = 0; i < len; i++) {
    hash += (hash * prime) + key[i];
  }
  return hash % prime;
}

// uint64_t is down cast to unint32_t since table size is only 100
STATIC unsigned item_hash(const char *item_key, const size_t item_key_len,
                          const unsigned attempt) {
  const unsigned long long hash_a = hash(item_key, item_key_len, HT_PRIME_1);
  const unsigned long long hash_b = hash(item_key, item_key_len, HT_PRIME_2);
  return (hash_a + (attempt * (hash_b + 1))) % HT_MAX;
}

STATIC const ht_item_t *item_lookup_slot(const ht_table_t *tbl,
                                         const char *item_key,
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

  ht_item_t *base = (ht_item_t *)tbl;
  const ht_item_t *item = &base[try];

  if (item == NULL) {
    fprintf(stderr, "i.sh: ht no item at index, code: %d", ERRHTNOITEM);
    exit(ERRHTNOITEM);
  }

  while (1) {
    if (attempt == HT_MAX - 1)
      break;
    // if NULL won't be beyond this point
    if (item->key == NULL)
      return item;
    if (item->key == HT_TOMBSTONE)
      return item;
    // if tombstone keep looking
    if (strncmp(item->key, item_key, HT_MAX_KEY_LEN) == 0)
      return item;
    attempt++;
    // retry step by 1
    try = item_hash(item_key, item_key_len, attempt);
    item = &base[try];
  }
  return NULL;
}

// initialize a table if not already initialized and return pointer to first
// elem
STATIC ht_table_t *table_init(void) {
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
STATIC ht_table_t *table_get(void) {
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
  const ht_item_t *item = item_lookup_slot(table, item_k, item_kl);
  if (item != NULL) {
    if (item->value == NULL)
      return strdup("");
  }
  return strdup(item->value);
}

// take safe len of key k
STATIC size_t key_get_len(const char *k) {
  if (k == NULL) {
    fprintf(stderr, "i.sh: ht no buffer, code: %d", ERRHTNOBUF);
    exit(ERRHTNOBUF);
  }

  const size_t kl_safe = strnlen(k, HT_MAX_KEY_LEN);
  const size_t kl_unsafe = strnlen(k, 100);

  if (kl_unsafe > kl_safe) {
    fprintf(stderr,
            "i.sh: key provided is longer than max %d chars long, code: %d",
            HT_MAX_KEY_LEN, ERRHTINS);
    return EXIT_FAILURE;
  }

  if (kl_safe == 0) {
    fprintf(stderr, "i.sh: failed to initialize key %s, code: %d", k, ERRHTINS);
    return EXIT_FAILURE;
  }
  return kl_safe;
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
  // drop const to make mutable here only
  ht_item_t *item = (ht_item_t *)item_lookup_slot(table, item_k, item_kl);

  if (item == NULL)
    return EXIT_FAILURE;

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
  return EXIT_SUCCESS;
}

// unset in shell, remove an item by freeing it's k,v and setting to NULL
int ht_del_var(const char *item_k) {
  if (item_k == NULL) {
    fprintf(stderr, "i.sh: no item key to delete, code: %d", ERRHTDEL);
    exit(ERRHTDEL);
  }

  ht_table_t *table = table_get();

  if (table == NULL) {
    fprintf(stderr, "i.sh: failed to initialize ht table, code: %d", ERRHTDEL);
    exit(ERRHTDEL);
  }

  size_t item_kl = key_get_len(item_k);
  // drop const to make mutable here only
  ht_item_t *item = (ht_item_t *)item_lookup_slot(table, item_k, item_kl);

  if (item == NULL || item->key == NULL)
    return EXIT_FAILURE;

  item->key = HT_TOMBSTONE;
  item->value = HT_TOMBSTONE;

  return EXIT_SUCCESS;
}
