#include "unity.h"

void setUp(void) {}
void tearDown(void) {}

void test_ht_table_get_should_return_an_ht_table(void) {}
void test_ht_item_lookup_should_return_an_ht_item(void) {}
void test_ht_item_hash_should_hash_an_item_key(void) {}
void test_ht_key_get_len_should_return_the_len_of_an_item_key(void) {}
void test_ht_table_init_should_initialize_a_table_if_not_exist_else_return_it(
    void) {}

int main(void) {
  UNITY_BEGIN();

  RUN_TEST(test_ht_table_get_should_return_an_ht_table);
  RUN_TEST(test_ht_item_lookup_should_return_an_ht_item);
  RUN_TEST(test_ht_item_hash_should_hash_an_item_key);
  RUN_TEST(test_ht_key_get_len_should_return_the_len_of_an_item_key);
  RUN_TEST(
      test_ht_table_init_should_initialize_a_table_if_not_exist_else_return_it);

  return UNITY_END();
}
