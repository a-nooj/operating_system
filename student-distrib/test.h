/* test.h - bare-bones unit testing */

#ifndef _TEST_H
#define _TEST_H

#include "lib.h"
#include "file_descriptor.h"

// putting this in a header would normally be a bad idea
// but I want the actual test file to be as clean as possible
static int test_failed;

// for a test you want to run
#define S_TEST(name) static void name(void)

// for a test you usually want to ignore
// intended for tests which take input and could be annoying
// in order to run ignored tests, set RUN_IGNORED_TESTS to 1 in generate_tests
#define S_IGNORE(name)         \
static void name(void) __attribute__((unused)); \
static void name(void)

// for a test you know is going to fail, since the first failed test causes an exit
// this is the same as S_IGNORE, but it's intended to identify failing cases
#define S_FAIL S_IGNORE

// in case we need to switch print mechanisms
#define S_PRINT(str) write(1, str, strlen(str))

// this purposely doesn't exit on assertion failure
// so that function can run to completeion
#define S_ASSERT(exp, str)     \
do {                           \
	if (!(exp)) {              \
		S_PRINT(str);          \
		test_failed = 1;       \
	}                          \
} while (0)

// the first failed test will exit out
// this is so that the error message doesn't disappear
#define S_RUN_TEST(name)       \
do {                           \
	S_PRINT("Running test " #name "\n"); \
	test_failed = 0;           \
	name();                    \
	if (test_failed) {         \
		return;                \
	}                          \
} while (0)

#endif
