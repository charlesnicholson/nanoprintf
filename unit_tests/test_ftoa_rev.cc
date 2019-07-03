#include "CppUTest/TestHarness.h"
#include "nanoprintf_in_unit_tests.h"

TEST_GROUP(ftoa_rev) {
    void setup() override { memset(buf, 0, sizeof(buf)); }
    char buf[64];
};

TEST(ftoa_rev, Zero) {
    CHECK_EQUAL(8, npf__ftoa_rev(buf, 0.f));
    STRCMP_EQUAL("000000.0", buf);
}

TEST(ftoa_rev, derp) {
    char buf[128];
    npf_snprintf(buf, 128, "%f", (double)1.1234f);
    //    printf("<%s>\n", buf);
    npf_snprintf(buf, 128, "%f", 1.0 / 0.0);
    //   printf("<%s>\n", buf);
    npf_snprintf(buf, 128, "%f", (double)1.1f);
    //  printf("<%s>\n", buf);
    npf__ftoa_rev(buf, 1.0f);
    npf__ftoa_rev(buf, 1.1234f);
    npf__ftoa_rev(buf, 0.f);
    npf__ftoa_rev(buf, -1.0f);
    npf__ftoa_rev(buf, 1000.0f);
    npf__ftoa_rev(buf, 12345.0f);
}
