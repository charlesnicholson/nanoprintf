#include "CppUTest/TestHarness.h"
#include "nanoprintf_in_unit_tests.h"

TEST_GROUP(fsplit_abs) { uint64_t int_part, frac_part; };

TEST(fsplit_abs, Zero) {
    CHECK(npf__fsplit_abs(0.f, &int_part, &frac_part));
    CHECK_EQUAL(0, int_part);
    CHECK_EQUAL(0, frac_part);
}

TEST(fsplit_abs, One) {
    CHECK(npf__fsplit_abs(1.f, &int_part, &frac_part));
    CHECK_EQUAL(1, int_part);
    CHECK_EQUAL(0, frac_part);
}

TEST(fsplit_abs, NegativeOne) {
    CHECK(npf__fsplit_abs(-1.f, &int_part, &frac_part));
    CHECK_EQUAL(1, int_part);
    CHECK_EQUAL(0, frac_part);
}

TEST(fsplit_abs, LargePositiveInteger) {
    CHECK(npf__fsplit_abs(123456.f, &int_part, &frac_part));
    CHECK_EQUAL(123456, int_part);
    CHECK_EQUAL(0, frac_part);
}

TEST(fsplit_abs, LargeNegativeInteger) {
    CHECK(npf__fsplit_abs(-123456.f, &int_part, &frac_part));
    CHECK_EQUAL(123456, int_part);
    CHECK_EQUAL(0, frac_part);
}

TEST_GROUP(ftoa_rev){};

TEST(ftoa_rev, derp) {
    char buf[128];
    npf__ftoa_rev(buf, 1.0f);
    npf__ftoa_rev(buf, 1.1234f);
    npf__ftoa_rev(buf, 0.f);
    npf__ftoa_rev(buf, -1.0f);
    npf__ftoa_rev(buf, 1000.0f);
    npf__ftoa_rev(buf, 12345.0f);
}
