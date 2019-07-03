#include "CppUTest/TestHarness.h"
#include "nanoprintf_in_unit_tests.h"

TEST_GROUP(ftoa_rev) {
    void setup() override { memset(buf, 0, sizeof(buf)); }
    char buf[64];
    npf__format_spec_conversion_case_t const lower =
        NPF_FMT_SPEC_CONV_CASE_LOWER;
    npf__format_spec_conversion_case_t const upper =
        NPF_FMT_SPEC_CONV_CASE_UPPER;
};

TEST(ftoa_rev, Zero) {
    CHECK_EQUAL(8, npf__ftoa_rev(buf, 0.f, 10, lower));
    STRCMP_EQUAL("000000.0", buf);
}

TEST(ftoa_rev, derp) {
    char buf[128];
    npf_snprintf(buf, 128, "%f", (double)1.1234f, 10, lower);
    //    printf("<%s>\n", buf);
    npf_snprintf(buf, 128, "%f", 1.0 / 0.0, 10, lower);
    //   printf("<%s>\n", buf);
    npf_snprintf(buf, 128, "%f", (double)1.1f, 10, lower);
    //  printf("<%s>\n", buf);
    npf__ftoa_rev(buf, 1.0f, 10, lower);
    npf__ftoa_rev(buf, 1.1234f, 10, lower);
    npf__ftoa_rev(buf, 0.f, 10, lower);
    npf__ftoa_rev(buf, -1.0f, 10, lower);
    npf__ftoa_rev(buf, 1000.0f, 10, lower);
    npf__ftoa_rev(buf, 12345.0f, 10, lower);
}
