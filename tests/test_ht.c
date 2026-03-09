#include "ht_internal.h"
#include "unity.h"
#include <stdlib.h>

void setUp(void) {}

void tearDown(void) {
  if (ht_table != NULL)
    free(ht_table);
  ht_table = NULL;
}

/*
STATIC ht_item_t *item_lookup(const ht_table_t *, const char *, const size_t);
STATIC unsigned item_hash(const char *, const size_t, const unsigned);
STATIC size_t key_get_len(const char *);
*/

void test_ht_ensure_ht_table_initializes_as_null() {
  TEST_ASSERT_NULL(ht_table);
}

void test_ht_table_init_should_init_a_table_if_not_exist() {
  ht_table_t *expected = calloc(1, sizeof(ht_table_t));
  TEST_ASSERT_NOT_NULL_MESSAGE(expected, "failed to allocate a test table");

  ht_table_t *actual = table_init();
  TEST_ASSERT_NOT_NULL_MESSAGE(actual, "failed to initialize table");

  TEST_ASSERT_EQUAL_MEMORY(expected, actual, sizeof(ht_table_t));

  free(expected);
  // actual is freed in teardown
}

void test_ht_table_get_should_return_an_ht_table(void) {
  ht_table_t *expected = calloc(1, sizeof(ht_table_t));
  TEST_ASSERT_NOT_NULL_MESSAGE(expected, "failed to allocate a test table");

  ht_table_t *actual = table_get();
  TEST_ASSERT_NOT_NULL_MESSAGE(actual, "failed to get new table");

  TEST_ASSERT_EQUAL_MEMORY(expected, actual, sizeof(ht_table_t));

  free(expected);
  // actual is freed in teardown
}

void test_ht_item_lookup_should_return_an_ht_item(void) {}
void test_ht_item_hash_should_hash_an_item_key(void) {}
void test_ht_key_get_len_should_return_the_len_of_an_item_key(void) {}

int main(void) {
  UNITY_BEGIN();

  RUN_TEST(test_ht_ensure_ht_table_initializes_as_null);
  RUN_TEST(test_ht_table_init_should_init_a_table_if_not_exist);
  RUN_TEST(test_ht_table_get_should_return_an_ht_table);
  RUN_TEST(test_ht_item_lookup_should_return_an_ht_item);
  RUN_TEST(test_ht_item_hash_should_hash_an_item_key);
  RUN_TEST(test_ht_key_get_len_should_return_the_len_of_an_item_key);

  return UNITY_END();
}
