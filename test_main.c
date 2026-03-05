#include "ish.h"
#include "unity/src/unity.h"
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
void has_iterator(parse_state_t);
void parser_set_token_type(semantic_token_t *);
void parser_set_token_val(char *, semantic_token_t *);
int is_expression(char *);
int is_command(char *);
void tokenv_to_argv(size_t, char **, semantic_token_t **);
int echo(size_t, void **);
int fexit(size_t, void **);
*/

// walk the buffer past the iterator if present and convert iterator to decimal
void test_has_iterator_should_parse_out_iterators_and_advance_buf(void) {
  // a "capture" is the potential iterator, captured as first word before ' '
  // this is done in the caller so provided here as separate

#define CASES 7
  const int expected[CASES] = {0, 0, 1, 7, 10, 100, 432};

  // "7 <mycommand> split in caller
  // are these strings pre-processed to remove a char?
  const char *cases[CASES][2] = {
      {"<mycommand>", "<mycommand>"}, {"0", "0 <mycommand>"},
      {"1", "1 <mycommand>"},         {"7", "7 <mycommand>"},
      {"10", "10 <mycommand>"},       {"100", "100 <mycommand>"},
      {"432", "432 <mycommand>"},
  };

  for (int i = 0; i < CASES; i++) {
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

void test_has_iterator_should_ignore_buf(void) {
  const char input[] = "<mycommand>";
  const char *buf = input;
  size_t *iterator = 0;
  parse_state_t state = {
      .buf = &buf, .kwlen = strnlen(buf, 20), .iterator = iterator};
  has_iterator(state);
  TEST_ASSERT_EQUAL_UINT(0, *iterator);
  TEST_ASSERT_EQUAL_STRING("<mycommand>", buf);
}

int main(void) {
  UNITY_BEGIN();

  RUN_TEST(test_has_iterator_should_parse_out_iterators_and_advance_buf);
  // RUN_TEST(test_has_iterator_should_ignore_buf);

  return UNITY_END();
}
