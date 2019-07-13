#include "nanoprintf_in_unit_tests.h"

#include "CppUTest/TestHarness.h"

TEST_GROUP(npf_snprintf) { char buf[128]; };

TEST(npf_snprintf, ReturnsNumberOfPrintedCharactersWithoutNullTerm) {
    CHECK_EQUAL(0, npf_snprintf(buf, sizeof(buf), ""));
    CHECK_EQUAL(1, npf_snprintf(buf, sizeof(buf), "a"));
    CHECK_EQUAL(7, npf_snprintf(buf, sizeof(buf), "%s", "abcdefg"));
}

TEST(npf_snprintf, PrintsStringToBuf) {
    CHECK_EQUAL(11, npf_snprintf(buf, sizeof(buf), "hello %s", "world"));
    STRCMP_EQUAL("hello world", buf);
}

TEST(npf_snprintf, ReturnsLenIfBufIsNull) {
    CHECK_EQUAL(11, npf_snprintf(nullptr, 0, "hello %s", "world"));
}
