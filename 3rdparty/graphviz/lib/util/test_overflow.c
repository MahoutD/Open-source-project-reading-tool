/// @file
/// @brief unit test for overflow.h

#include <assert.h>
#include <limits.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>

// were we asked to test the fallback C code?
#ifdef SUPPRESS_BUILTINS
#ifdef __GNUC__
#undef __GNUC__
#endif
#ifdef __clang__
#undef __clang__
#endif
#endif

#include <util/overflow.h>

/// a test case
typedef struct {
  int operand1;         ///< left-hand side
  int operand2;         ///< right-hand side
  bool should_overflow; ///< should addition be expected to overflow?
} testcase_t;

/// cases to run
static const testcase_t TESTS[] = {
    {.operand1 = 0, .operand2 = 0},
    {.operand1 = 0, .operand2 = 1},
    {.operand1 = 0, .operand2 = -1},
    {.operand1 = 0, .operand2 = 2},
    {.operand1 = 0, .operand2 = -2},
    {.operand1 = 0, .operand2 = INT_MAX},
    {.operand1 = 0, .operand2 = INT_MIN},
    {.operand1 = 0, .operand2 = INT_MAX - 1},
    {.operand1 = 0, .operand2 = INT_MIN + 1},

    {.operand1 = 1, .operand2 = 0},
    {.operand1 = 1, .operand2 = 1},
    {.operand1 = 1, .operand2 = -1},
    {.operand1 = 1, .operand2 = 2},
    {.operand1 = 1, .operand2 = -2},
    {.operand1 = 1, .operand2 = INT_MAX, .should_overflow = true},
    {.operand1 = 1, .operand2 = INT_MIN},
    {.operand1 = 1, .operand2 = INT_MAX - 1},
    {.operand1 = 1, .operand2 = INT_MIN + 1},

    {.operand1 = -1, .operand2 = 0},
    {.operand1 = -1, .operand2 = 1},
    {.operand1 = -1, .operand2 = -1},
    {.operand1 = -1, .operand2 = 2},
    {.operand1 = -1, .operand2 = -2},
    {.operand1 = -1, .operand2 = INT_MAX},
    {.operand1 = -1, .operand2 = INT_MIN, .should_overflow = true},
    {.operand1 = -1, .operand2 = INT_MAX - 1},
    {.operand1 = -1, .operand2 = INT_MIN + 1},

    {.operand1 = 2, .operand2 = 0},
    {.operand1 = 2, .operand2 = 1},
    {.operand1 = 2, .operand2 = -1},
    {.operand1 = 2, .operand2 = 2},
    {.operand1 = 2, .operand2 = -2},
    {.operand1 = 2, .operand2 = INT_MAX, .should_overflow = true},
    {.operand1 = 2, .operand2 = INT_MIN},
    {.operand1 = 2, .operand2 = INT_MAX - 1, .should_overflow = true},
    {.operand1 = 2, .operand2 = INT_MIN + 1},

    {.operand1 = -2, .operand2 = 0},
    {.operand1 = -2, .operand2 = 1},
    {.operand1 = -2, .operand2 = -1},
    {.operand1 = -2, .operand2 = 2},
    {.operand1 = -2, .operand2 = -2},
    {.operand1 = -2, .operand2 = INT_MAX},
    {.operand1 = -2, .operand2 = INT_MIN, .should_overflow = true},
    {.operand1 = -2, .operand2 = INT_MAX - 1},
    {.operand1 = -2, .operand2 = INT_MIN + 1, .should_overflow = true},

    {.operand1 = INT_MAX, .operand2 = 0},
    {.operand1 = INT_MAX, .operand2 = 1, .should_overflow = true},
    {.operand1 = INT_MAX, .operand2 = -1},
    {.operand1 = INT_MAX, .operand2 = 2, .should_overflow = true},
    {.operand1 = INT_MAX, .operand2 = -2},
    {.operand1 = INT_MAX, .operand2 = INT_MAX, .should_overflow = true},
    {.operand1 = INT_MAX, .operand2 = INT_MIN},
    {.operand1 = INT_MAX, .operand2 = INT_MAX - 1, .should_overflow = true},
    {.operand1 = INT_MAX, .operand2 = INT_MIN + 1},

    {.operand1 = INT_MIN, .operand2 = 0},
    {.operand1 = INT_MIN, .operand2 = 1},
    {.operand1 = INT_MIN, .operand2 = -1, .should_overflow = true},
    {.operand1 = INT_MIN, .operand2 = 2},
    {.operand1 = INT_MIN, .operand2 = -2, .should_overflow = true},
    {.operand1 = INT_MIN, .operand2 = INT_MAX},
    {.operand1 = INT_MIN, .operand2 = INT_MIN, .should_overflow = true},
    {.operand1 = INT_MIN, .operand2 = INT_MAX - 1},
    {.operand1 = INT_MIN, .operand2 = INT_MIN + 1, .should_overflow = true},

    {.operand1 = INT_MAX - 1, .operand2 = 0},
    {.operand1 = INT_MAX - 1, .operand2 = 1},
    {.operand1 = INT_MAX - 1, .operand2 = -1},
    {.operand1 = INT_MAX - 1, .operand2 = 2, .should_overflow = true},
    {.operand1 = INT_MAX - 1, .operand2 = -2},
    {.operand1 = INT_MAX - 1, .operand2 = INT_MAX, .should_overflow = true},
    {.operand1 = INT_MAX - 1, .operand2 = INT_MIN},
    {.operand1 = INT_MAX - 1, .operand2 = INT_MAX - 1, .should_overflow = true},
    {.operand1 = INT_MAX - 1, .operand2 = INT_MIN + 1},

    {.operand1 = INT_MIN + 1, .operand2 = 0},
    {.operand1 = INT_MIN + 1, .operand2 = 1},
    {.operand1 = INT_MIN + 1, .operand2 = -1},
    {.operand1 = INT_MIN + 1, .operand2 = 2},
    {.operand1 = INT_MIN + 1, .operand2 = -2, .should_overflow = true},
    {.operand1 = INT_MIN + 1, .operand2 = INT_MAX},
    {.operand1 = INT_MIN + 1, .operand2 = INT_MIN, .should_overflow = true},
    {.operand1 = INT_MIN + 1, .operand2 = INT_MAX - 1},
    {.operand1 = INT_MIN + 1, .operand2 = INT_MIN + 1, .should_overflow = true},
};

int main(void) {

  for (size_t i = 0; i < sizeof(TESTS) / sizeof(TESTS[0]); ++i) {

    const int op1 = TESTS[i].operand1;
    const int op2 = TESTS[i].operand2;

    int answer;
    const bool did_overflow = sadd_overflow(op1, op2, &answer);

    const bool should_overflow = TESTS[i].should_overflow;
    if (did_overflow != should_overflow) {
      fprintf(stderr, "%d + %d should%s have overflowed and it did%s\n", op1,
              op2, should_overflow ? "" : " not", did_overflow ? "" : " not");
      return EXIT_FAILURE;
    }

    if (!did_overflow && op1 + op2 != answer) {
      fprintf(stderr, "%d + %d is %d but result was %d\n", op1, op2, op1 + op2,
              answer);
      return EXIT_FAILURE;
    }
  }

  return EXIT_SUCCESS;
}
