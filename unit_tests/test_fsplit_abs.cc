#include "CppUTest/TestHarness.h"
#include "nanoprintf_in_unit_tests.h"

#include <cmath>

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

TEST(fsplit_abs, ReturnsZeroIfExponentTooLarge) {
    CHECK(npf__fsplit_abs(std::powf(2.0f, 63.f), &int_part, &frac_part));
    CHECK_EQUAL(0,
                npf__fsplit_abs(std::powf(2.0f, 64.f), &int_part, &frac_part));
}

// Perfectly-representable fractions, adding 1 bit to mantissa each time.

TEST(fsplit_abs, 532) {
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

TEST(fsplit_abs, 9921875) {
    CHECK(npf__fsplit_abs(1.9921875f, &int_part, &frac_part));
    CHECK_EQUAL(1, int_part);
    CHECK_EQUAL(9921875, frac_part);
}

// Divergent but split has full accuracy.

TEST(fsplit_abs, 9960938) {
    CHECK(npf__fsplit_abs(1.9960938f, &int_part, &frac_part));
    CHECK_EQUAL(1, int_part);
    CHECK_EQUAL(99609375, frac_part);
}

// Truncations, continue adding mantissa bits

TEST(fsplit_abs, 9980469_Truncates) {
    CHECK(npf__fsplit_abs(1.9980469f, &int_part, &frac_part));
    CHECK_EQUAL(1, int_part);
    CHECK_EQUAL(99804687, frac_part);  // 1.998046875 is stored.
}

TEST(fsplit_abs, 9990234_Truncates) {
    CHECK(npf__fsplit_abs(1.9990234f, &int_part, &frac_part));
    CHECK_EQUAL(1, int_part);
    CHECK_EQUAL(99902343, frac_part);  // 1.9990234375 is stored.
}

TEST(fsplit_abs, 9995117_Truncates) {
    CHECK(npf__fsplit_abs(1.9995117f, &int_part, &frac_part));
    CHECK_EQUAL(1, int_part);
    CHECK_EQUAL(99951171, frac_part);  // 1.99951171875 is stored.
}

TEST(fsplit_abs, 9997559_Truncates) {
    CHECK(npf__fsplit_abs(1.9997559f, &int_part, &frac_part));
    CHECK_EQUAL(1, int_part);
    CHECK_EQUAL(99975585, frac_part);  // 1.999755859375 is stored.
}

TEST(fsplit_abs, 9998779_Truncates) {
    CHECK(npf__fsplit_abs(1.9998779f, &int_part, &frac_part));
    CHECK_EQUAL(1, int_part);
    CHECK_EQUAL(99987792, frac_part);  // 1.9998779296875 is stored.
}

TEST(fsplit_abs, 999939_Truncates) {
    CHECK(npf__fsplit_abs(1.999939f, &int_part, &frac_part));
    CHECK_EQUAL(1, int_part);
    CHECK_EQUAL(99993896, frac_part);  // 1.99993896484375 is stored.
}

TEST(fsplit_abs, 9999695_Truncates) {
    CHECK(npf__fsplit_abs(1.9999695f, &int_part, &frac_part));
    CHECK_EQUAL(1, int_part);
    CHECK_EQUAL(99996948, frac_part);  // 1.999969482421875 is stored.
}

TEST(fsplit_abs, 9999847_Truncates) {
    CHECK(npf__fsplit_abs(1.9999847f, &int_part, &frac_part));
    CHECK_EQUAL(1, int_part);
    CHECK_EQUAL(99998474, frac_part);  // 1.9999847412109375 is stored.
}

TEST(fsplit_abs, 9999924_Truncates) {
    CHECK(npf__fsplit_abs(1.9999924f, &int_part, &frac_part));
    CHECK_EQUAL(1, int_part);
    CHECK_EQUAL(99999237, frac_part);  // 1.99999237060546875 is stored.
}

TEST(fsplit_abs, 9999962_Truncates) {
    CHECK(npf__fsplit_abs(1.9999962f, &int_part, &frac_part));
    CHECK_EQUAL(1, int_part);
    CHECK_EQUAL(99999618, frac_part);  // 1.999996185302734375 is stored.
}

TEST(fsplit_abs, 9999981_Truncates) {
    CHECK(npf__fsplit_abs(1.9999981f, &int_part, &frac_part));
    CHECK_EQUAL(1, int_part);
    CHECK_EQUAL(99999809, frac_part);  // 1.9999980926513671875 is stored.
}

TEST(fsplit_abs, 999999_Truncates) {
    CHECK(npf__fsplit_abs(1.999999f, &int_part, &frac_part));
    CHECK_EQUAL(1, int_part);
    CHECK_EQUAL(99999904, frac_part);  // 1.99999904632568359375 is stored.
}

TEST(fsplit_abs, 9999995_Truncates) {
    CHECK(npf__fsplit_abs(1.9999995f, &int_part, &frac_part));
    CHECK_EQUAL(1, int_part);
    CHECK_EQUAL(99999952, frac_part);  // 1.999999523162841796875 is stored.
}

TEST(fsplit_abs, 9999998_Truncates) {
    CHECK(npf__fsplit_abs(1.9999998f, &int_part, &frac_part));
    CHECK_EQUAL(1, int_part);
    CHECK_EQUAL(99999976, frac_part);  // 1.9999997615814208984375 is stored.
}

TEST(fsplit_abs, 9999999_Truncates) {
    CHECK(npf__fsplit_abs(1.9999999f, &int_part, &frac_part));
    CHECK_EQUAL(1, int_part);
    CHECK_EQUAL(99999988, frac_part);  //  1.99999988079071044921875 is stored.
}
