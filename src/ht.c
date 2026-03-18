#include "ht.h"
#include "errors.h"
#include "ht_internal.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static char HT_TOMBSTONE_SENTINEL = 0;
static void *const HT_TOMBSTONE = &HT_TOMBSTONE_SENTINEL;

// E n-1 s[i]**n-1-i
//
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

STATIC const ht_item_t *item_lookup_slot(const ht_table_t ht,
                                         const char *item_key,
                                         const size_t item_key_len) {

  if (ht == NULL) {
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

  ht_item_t *base = (ht_item_t *)ht;
  const ht_item_t *item = &base[try];

  if (item == NULL) {
    fprintf(stderr, "i.sh: ht no item at index, code: %d", ERRHTNOITEM);
    exit(ERRHTNOITEM);
  }

  void *handler = NULL;
  char *string = NULL;

  while (1) {
    if (attempt == HT_MAX - 1) {
      return NULL;
      break;
    }
    // if NULL won't be beyond this point

    switch (item->type) {
    case FUNCTION:
      handler = (void *)item->value.handler;
      if (handler == NULL || handler == HT_TOMBSTONE)
        return item;
      break;
    case STRING:
      string = (void *)item->value.string;
      if (string == NULL || string == HT_TOMBSTONE)
        return item;
      if (strncmp(item->key, item_key, HT_MAX_KEY_LEN) == 0)
        return item;
      break;
    default:
      fprintf(stderr, "i.sh: ht slot lookup error, code: %d", ERRHTINIT);
      break;
    }
    attempt++;
    // retry step by 1
    try = item_hash(item_key, item_key_len, attempt);
    item = &base[try];
  }
  return NULL;
}

// ht_table_t is now declared as a pointer type in .h
ht_table_t ht_create_table(size_t count) {
  ht_table_t ht_table = calloc(1, sizeof(struct ht_table));

  if (ht_table == NULL) {
    fprintf(stderr, "i.sh: failed to initialize ht table, code: %d", ERRHTINIT);
    exit(ERRHTINIT);
  }
  ht_table->count = count;
  ht_table->items = calloc(count, sizeof(ht_item_t *));
  if (ht_table->items == NULL) {
    fprintf(stderr, "i.sh: failed to initialize ht table, code: %d", ERRHTINIT);
    exit(ERRHTINIT);
  }

  for (size_t i = 0; i < ht_table->count; i++) {
    // each items[i] 0x0 expects a pointer to type of size ht_item_t
    ht_table->items[i] = calloc(1, sizeof(ht_item_t));
  }
  return ht_table;
}

// caller must check is val NULL
const char *ht_get_item(ht_table_t table, const char *item_k) {
  if (item_k == NULL) {
    fprintf(stderr, "i.sh: ht no buffer, code: %d", ERRHTNOBUF);
    exit(ERRHTNOBUF);
  }

  if (table == NULL) {
    fprintf(stderr, "i.sh: failed to get ht table, code: %d", ERRHTGET);
    exit(ERRHTGET);
  }

  size_t item_kl = key_get_len(item_k);
  const ht_item_t *item = item_lookup_slot(table, item_k, item_kl);

  switch (item->type) {
  case FUNCTION:
    return item->value.handler;
    break;
  case STRING:
    return item->value.string;
    break;
  default:
    return NULL;
    break;
  }
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

int ht_put_item(ht_table_t table, const char *item_k, const void *item_v,
                const type_t type) {

  if (item_k == NULL || item_v == NULL) {
    fprintf(stderr, "i.sh: ht no buffer, code: %d", ERRHTNOBUF);
    exit(ERRHTNOBUF);
  }

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

  switch (type) {
  case FUNCTION:
    item->type = type;
    item->value.handler = item_v;
    break;
  case STRING:
    item->type = type;
    item->value.string = item_v;
    break;
  default:
    fprintf(stderr, "i.sh: issue in get item with type");
    break;
  }

  return EXIT_SUCCESS;
}

// unset in shell, remove an item by freeing it's k,v and setting to NULL
int ht_del_item(ht_table_t table, const char *item_k) {
  if (item_k == NULL) {
    fprintf(stderr, "i.sh: no item key to delete, code: %d", ERRHTDEL);
    exit(ERRHTDEL);
  }

  if (table == NULL) {
    fprintf(stderr, "i.sh: failed to initialize ht table, code: %d", ERRHTDEL);
    exit(ERRHTDEL);
  }

  size_t item_kl = key_get_len(item_k);
  // drop const to make mutable here only
  ht_item_t *item = (ht_item_t *)item_lookup_slot(table, item_k, item_kl);

  if (item == NULL || item->key == NULL)
    return EXIT_FAILURE;

  switch (item->type) {
  case FUNCTION:
    item->value.handler = HT_TOMBSTONE;
    break;
  case STRING:
    item->value.string = HT_TOMBSTONE;
    break;
  default:
    return EXIT_FAILURE;
    break;
  }

  return EXIT_SUCCESS;
}
