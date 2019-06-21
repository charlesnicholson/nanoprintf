#include "nanoprintf_in_unit_tests.h"
#include "CppUTest/TestHarness.h"

TEST_GROUP(npf_snprintf) {};

TEST(npf_snprintf, NoFormatStrings) {
    char s[128];
    npf_snprintf(s, sizeof(s), "hello npf_snprintf");
    STRCMP_EQUAL("hello npf_snprintf", s);
}
