#include "ish.h"
#include "unity.h"
#include <stdlib.h>
#include <string.h>

void setUp(void) {}
void tearDown(void) {}
/*
void simple_parser(char *);
ssize_t read_input(char *);
void repl();
void exec_command(int, size_t, char **);
void mystrcspn(char **);
void destroy_tokens(size_t, semantic_token_t **);
void destroy_args(size_t, char **);
void parse_expr(size_t, semantic_token_t **);
void parser_tokenize(char *, semantic_token_t **, size_t *);
int is_expression(char *);
int is_command(char *);
void tokenv_to_argv(size_t, char **, semantic_token_t **);
int echo(size_t, void **);
int fexit(size_t, void **);
*/

void test_is_expression_should_verify_various_expression_type(void) {
#define SHOULD_VERIFY_EXPR_TYPES 3
  char *expression_cases[SHOULD_VERIFY_EXPR_TYPES] = {
      "$x",
      "x=1",
      "x=$y",
  };

  for (int i = 0; i < SHOULD_VERIFY_EXPR_TYPES; i++) {
    char *exp = expression_cases[i];
    TEST_ASSERT_EQUAL_UINT(MATCH, is_expression(exp));
  }

  char *non_expression_cases[SHOULD_VERIFY_EXPR_TYPES] = {
      "x",
      "ls",
      "<mycommand>",
  };

  for (int i = 0; i < SHOULD_VERIFY_EXPR_TYPES; i++) {
    char *exp = non_expression_cases[i];
    TEST_ASSERT_EQUAL_UINT(!MATCH, is_expression(exp));
  }
}

void test_get_builtins_should_returns_list_of_known_builtins(void) {
  builtin_t expected[MAX] = {
      {echo, "echo"},
      {fexit, "exit"},
      {fexit, "q"},
  };
  builtin_t *actual = get_builtins();

  TEST_ASSERT_EQUAL_MEMORY(expected, actual, sizeof(*actual) * MAX);
}

void test_is_builtin_should_verify_various_builtin_type(void) {
#define SHOULD_VERIFY_BUILTIN_TYPES 3
  char *cases[SHOULD_VERIFY_BUILTIN_TYPES] = {
      "echo",
      "exit",
      "q",
  };

  for (int i = 0; i < SHOULD_VERIFY_BUILTIN_TYPES; i++) {
    char *exp = cases[i];
    TEST_ASSERT_EQUAL_UINT(MATCH, is_builtin(exp));
  }
}

// void test_is_command_should_verify_various_command_type(void) { is_command();
// }

// token buf should always be unset, before being set
void test_parser_set_token_val_should_set_val_even_if_already_set(void) {

  char *expected = "1234";
  semantic_token_t token = {0};

#define SHOULD_VERIFY_SET_AND_RESET 2
  for (int i = 0; i < SHOULD_VERIFY_SET_AND_RESET; i++) {
    parser_set_token_val(expected, &token);
    TEST_ASSERT_EQUAL_STRING(expected, token.buf);
    expected = "12345";
  }
  free(token.buf);
}

void test_parser_set_token_type_should_set_type_based_on_input(void) {

  struct results {
    char *input;
    semantic_type_t expected;
  };

#define SHOULD_VERIFY_TYPES 5
  struct results cases[SHOULD_VERIFY_TYPES] = {
      {"ls", COMMAND},     {"echo", BUILTIN},    {"$x", EXPRESSION},
      {"x=1", EXPRESSION}, {"x=$y", EXPRESSION},
  };

  semantic_token_t token = {0};

  for (int i = 0; i < SHOULD_VERIFY_TYPES; i++) {
    char *buf = cases[i].input;
    semantic_type_t expected = cases[i].expected;

    parser_set_token_val(buf, &token);
    parser_set_token_type(&token);

    TEST_ASSERT_EQUAL_UINT(expected, token.type);
  }
  free(token.buf);
}

// walk the buffer past the iterator if present and convert iterator to decimal
void test_has_iterator_should_parse_out_iterators_and_advance_buf(void) {
  // a "capture" is the potential iterator, captured as first word before ' '
  // this is done in the caller so provided here as separate

#define SHOULD_ADVANCE_BUFFER_CASES 6
  const size_t expected[SHOULD_ADVANCE_BUFFER_CASES] = {0, 1, 7, 10, 100, 432};

  // "7 <mycommand> split in caller
  const char *cases[SHOULD_ADVANCE_BUFFER_CASES][2] = {
      {"0", "0 <mycommand>"},     {"1", "1 <mycommand>"},
      {"7", "7 <mycommand>"},     {"10", "10 <mycommand>"},
      {"100", "100 <mycommand>"}, {"432", "432 <mycommand>"},
  };

  for (int i = 0; i < SHOULD_ADVANCE_BUFFER_CASES; i++) {
    // char** are passed to avoid allocations
    const char *cap = cases[i][0];
    const char *buf = cases[i][1];
    size_t iterator = 0;

    parse_state_t state = {.capture = cap,
                           .buf = &buf,
                           .kwlen = strnlen(cap, 20),
                           .iterator = &iterator};
    has_iterator(state);
    TEST_ASSERT_EQUAL_UINT(expected[i], iterator);
    TEST_ASSERT_EQUAL_STRING(" <mycommand>", buf);
  }
}

// walk the buffer past the iterator if present and convert iterator to decimal
void test_has_iterator_should_ignore_buffer(void) {
#define SHOULD_IGNORE_BUFFER_CASES 1
  const size_t expected[SHOULD_IGNORE_BUFFER_CASES] = {0};

  // "7 <mycommand> split in caller
  const char *cases[SHOULD_IGNORE_BUFFER_CASES][2] = {
      {"<mycommand>", "<mycommand>"},
  };

  for (int i = 0; i < SHOULD_IGNORE_BUFFER_CASES; i++) {
    // char** are passed to avoid allocations
    const char *cap = cases[i][0];
    const char *buf = cases[i][1];
    size_t iterator = 0;

    parse_state_t state = {.capture = cap,
                           .buf = &buf,
                           .kwlen = strnlen(cap, 20),
                           .iterator = &iterator};
    has_iterator(state);
    TEST_ASSERT_EQUAL_UINT(expected[i], iterator);
    TEST_ASSERT_EQUAL_STRING("<mycommand>", buf);
  }
}

int main(void) {
  UNITY_BEGIN();

  RUN_TEST(test_has_iterator_should_parse_out_iterators_and_advance_buf);
  RUN_TEST(test_has_iterator_should_ignore_buffer);
  RUN_TEST(test_parser_set_token_val_should_set_val_even_if_already_set);
  RUN_TEST(test_parser_set_token_type_should_set_type_based_on_input);
  RUN_TEST(test_is_expression_should_verify_various_expression_type);
  RUN_TEST(test_get_builtins_should_returns_list_of_known_builtins);
  RUN_TEST(test_is_builtin_should_verify_various_builtin_type);

  return UNITY_END();
}
