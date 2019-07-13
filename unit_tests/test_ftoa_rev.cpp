#include "nanoprintf_in_unit_tests.h"

#include "CppUTest/TestHarness.h"

#include <cstring>

TEST_GROUP(ftoa_rev) {
    void setup() override { memset(buf, 0, sizeof(buf)); }
    char buf[64];
    int frac_bytes;
    npf__format_spec_conversion_case_t const lower =
        NPF_FMT_SPEC_CONV_CASE_LOWER;
    npf__format_spec_conversion_case_t const upper =
        NPF_FMT_SPEC_CONV_CASE_UPPER;
};

TEST(ftoa_rev, Zero) {
    CHECK_EQUAL(2, npf__ftoa_rev(buf, 0.f, 10, lower, &frac_bytes));
    STRCMP_EQUAL(".0", buf);
}

