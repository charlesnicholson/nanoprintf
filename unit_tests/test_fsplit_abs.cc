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

// Perfectly-representable fractions

TEST(fsplit_abs, 5) {
    CHECK(npf__fsplit_abs(1.5f, &int_part, &frac_part));
    CHECK_EQUAL(1, int_part);
    CHECK_EQUAL(5, frac_part);
}

TEST(fsplit_abs, 625) {
    CHECK(npf__fsplit_abs(1.625f, &int_part, &frac_part));
    CHECK_EQUAL(1, int_part);
    CHECK_EQUAL(625, frac_part);
}

TEST(fsplit_abs, 875) {
    CHECK(npf__fsplit_abs(1.875f, &int_part, &frac_part));
    CHECK_EQUAL(1, int_part);
    CHECK_EQUAL(875, frac_part);
}

TEST(fsplit_abs, 9375) {
    CHECK(npf__fsplit_abs(1.9375f, &int_part, &frac_part));
    CHECK_EQUAL(1, int_part);
    CHECK_EQUAL(9375, frac_part);
}

TEST(fsplit_abs, 96875) {
    CHECK(npf__fsplit_abs(1.96875f, &int_part, &frac_part));
    CHECK_EQUAL(1, int_part);
    CHECK_EQUAL(96875, frac_part);
}

TEST(fsplit_abs, 984375) {
    CHECK(npf__fsplit_abs(1.984375f, &int_part, &frac_part));
    CHECK_EQUAL(1, int_part);
    CHECK_EQUAL(984375, frac_part);
}

TEST(fsplit_abs, 9921875_Truncates) {
    CHECK(npf__fsplit_abs(1.9921875f, &int_part, &frac_part));
    CHECK_EQUAL(1, int_part);
    CHECK_EQUAL(992187, frac_part);
}
