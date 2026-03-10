#include <unistd.h>

#define HT_MAX 1000
#define HT_MAX_KEY_LEN 25
#define HT_PRIME_1 31u
#define HT_PRIME_2 37u

#ifdef TEST
#define STATIC
#else
#define STATIC static
#endif

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

#ifdef TEST
// so the test file can find it
extern ht_table_t *ht_table;
#endif

STATIC ht_table_t *table_init(void);
STATIC ht_table_t *table_get(void);
STATIC ht_item_t *item_lookup(const ht_table_t *, const char *, const size_t);
STATIC unsigned item_hash(const char *, const size_t, const unsigned);
STATIC long long unsigned hash(const char *, const size_t, const unsigned);
STATIC size_t key_get_len(const char *);
