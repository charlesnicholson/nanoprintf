#include "nanoprintf_in_unit_tests.h"

#include "CppUTest/TestHarness.h"

#include <cstring>

TEST_GROUP(itoa) { char buf[64]; };

#define NPF_ITOA_CHECK(EXPECT_STR, VAL)          \
    do {                                         \
        int const n = npf__itoa_rev(buf, VAL);   \
        CHECK_EQUAL((int)strlen(EXPECT_STR), n); \
        buf[n] = 0;                              \
        STRCMP_EQUAL(EXPECT_STR, buf);           \
    } while (0)

TEST(itoa, 0) { NPF_ITOA_CHECK("0", 0); }
TEST(itoa, 1) { NPF_ITOA_CHECK("1", 1); }
TEST(itoa, 9) { NPF_ITOA_CHECK("9", 9); }
TEST(itoa, 10) { NPF_ITOA_CHECK("01", 10); }
TEST(itoa, 42) { NPF_ITOA_CHECK("24", 42); }
TEST(itoa, 99) { NPF_ITOA_CHECK("99", 99); }
TEST(itoa, 100) { NPF_ITOA_CHECK("001", 100); }
TEST(itoa, 123) { NPF_ITOA_CHECK("321", 123); }
TEST(itoa, 999) { NPF_ITOA_CHECK("999", 999); }
TEST(itoa, 6543210) { NPF_ITOA_CHECK("0123456", 6543210); }

TEST(itoa, Max) {
#if NANOPRINTF_USE_LARGE_FORMAT_SPECIFIERS == 1
#if INTMAX_MAX == 9223372036854775807ll
    NPF_ITOA_CHECK("7085774586302733229", INTMAX_MAX);
#else
#error Unknown INTMAX_MAX here, please add another branch.
#endif
#else
#if INT_MAX == 2147483647
    NPF_ITOA_CHECK("7463847412", INT_MAX);
#else
#error Unknown INT_MAX here, please add another branch.
#endif
#endif
}

TEST(itoa, Neg1) { NPF_ITOA_CHECK("1", -1); }
TEST(itoa, NegativeSignStripped) { NPF_ITOA_CHECK("12345", -54321); }

TEST(itoa, Min) {
#if NANOPRINTF_USE_LARGE_FORMAT_SPECIFIERS == 1
    NPF_ITOA_CHECK("8085774586302733229", INTMAX_MIN);
#else
#if INT_MIN == -2147483648
    NPF_ITOA_CHECK("8463847412", INT_MIN);
#else
#error Unknown INT_MIN here, please add another branch.
#endif
#endif
}
