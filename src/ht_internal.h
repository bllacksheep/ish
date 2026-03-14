#include <unistd.h>

#define HT_MAX 1009
#define HT_MAX_KEY_LEN 25
#define HT_PRIME_1 31u
#define HT_PRIME_2 37u

#ifdef TEST
#define STATIC
#else
#define STATIC static
#endif

typedef struct ht_item {
  const char *key;
  const char *value;
} ht_item_t;

typedef ht_item_t ht_table_t[HT_MAX];

#ifdef TEST
// so the test file can find it
extern ht_table_t *ht_table;
#endif

STATIC ht_table_t *table_init(void);
STATIC ht_table_t *table_get(void);
STATIC const ht_item_t *item_lookup_slot(const ht_table_t *, const char *,
                                         const size_t);
STATIC unsigned item_hash(const char *, const size_t, const unsigned);
STATIC long long unsigned hash(const char *, const size_t, const unsigned);
STATIC size_t key_get_len(const char *);
