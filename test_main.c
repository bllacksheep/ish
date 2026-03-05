#include "ish.h"
#include "unity/src/unity.h"

void setUp(void) {}

void tearDown(void) {}

void test_function_should_do_abc(void) {}

int main(void) {
  UNITY_BEGIN();

  RUN_TEST(test_function_should_do_abc);

  return UNITY_END();
}
