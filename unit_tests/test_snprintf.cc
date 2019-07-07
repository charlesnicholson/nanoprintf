#include "nanoprintf_in_unit_tests.h"

#include "CppUTest/TestHarness.h"

TEST_GROUP(npf_snprintf) { char buf[128]; };

TEST(npf_snprintf, PrintsStringToBuf) {
    CHECK_EQUAL(12, npf_snprintf(buf, sizeof(buf), "hello %s", "world"));
    STRCMP_EQUAL("hello world", buf);
}

TEST(npf_snprintf, ReturnsLenIfBufIsNull) {
    CHECK_EQUAL(12, npf_snprintf(nullptr, 0, "hello %s", "world"));
}
