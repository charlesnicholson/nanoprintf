#include "nanoprintf_in_unit_tests.h"

#include "CppUTest/TestHarness.h"

#include <cmath>

TEST_GROUP(fsplit_abs) {
    uint64_t i, f;
    int f_neg_exp;
};

TEST(fsplit_abs, Zero) {
    CHECK(npf__fsplit_abs(0.f, &i, &f, &f_neg_exp));
    CHECK_EQUAL(0, i);
    CHECK_EQUAL(0, f);
    CHECK_EQUAL(0, f_neg_exp);
}

TEST(fsplit_abs, One) {
    CHECK(npf__fsplit_abs(1.f, &i, &f, &f_neg_exp));
    CHECK_EQUAL(1, i);
    CHECK_EQUAL(0, f);
    CHECK_EQUAL(0, f_neg_exp);
}

TEST(fsplit_abs, NegativeOne) {
    CHECK(npf__fsplit_abs(-1.f, &i, &f, &f_neg_exp));
    CHECK_EQUAL(1, i);
    CHECK_EQUAL(0, f);
    CHECK_EQUAL(0, f_neg_exp);
}

TEST(fsplit_abs, LargePositiveInteger) {
    CHECK(npf__fsplit_abs(123456.f, &i, &f, &f_neg_exp));
    CHECK_EQUAL(123456, i);
    CHECK_EQUAL(0, f);
    CHECK_EQUAL(0, f_neg_exp);
}

TEST(fsplit_abs, LargeNegativeInteger) {
    CHECK(npf__fsplit_abs(-123456.f, &i, &f, &f_neg_exp));
    CHECK_EQUAL(123456, i);
    CHECK_EQUAL(0, f);
    CHECK_EQUAL(0, f_neg_exp);
}

TEST(fsplit_abs, ReturnsZeroIfExponentTooLarge) {
    CHECK(npf__fsplit_abs(powf(2.0f, 63.f), &i, &f, &f_neg_exp));
    CHECK_EQUAL(0, npf__fsplit_abs(powf(2.0f, 64.f), &i, &f, &f_neg_exp));
}

// Fractional negative base 10 exponent (how many 0's between . and non-zero)

TEST(fsplit_abs, FracBase10NegExp_One) {
    CHECK(npf__fsplit_abs(0.03125f, &i, &f, &f_neg_exp));
    CHECK_EQUAL(1, f_neg_exp);
}

TEST(fsplit_abs, FracBase10NegExp_Two) {
    CHECK(npf__fsplit_abs(0.0078125f, &i, &f, &f_neg_exp));
    CHECK_EQUAL(2, f_neg_exp);
}

TEST(fsplit_abs, FracBase10NegExp_Three) {
    CHECK(npf__fsplit_abs(2.4414062E-4f, &i, &f, &f_neg_exp));
    CHECK_EQUAL(3, f_neg_exp);
}

TEST(fsplit_abs, FracBase10NegExp_Five) {
    CHECK(npf__fsplit_abs(3.8146973E-6f, &i, &f, &f_neg_exp));
    CHECK_EQUAL(5, f_neg_exp);
}

// Perfectly-representable fractions, adding 1 bit to mantissa each time.

TEST(fsplit_abs, 5) {
    CHECK(npf__fsplit_abs(1.5f, &i, &f, &f_neg_exp));
    CHECK_EQUAL(1, i);
    CHECK_EQUAL(5, f);
}

TEST(fsplit_abs, 625) {
    CHECK(npf__fsplit_abs(1.625f, &i, &f, &f_neg_exp));
    CHECK_EQUAL(1, i);
    CHECK_EQUAL(625, f);
}

TEST(fsplit_abs, 875) {
    CHECK(npf__fsplit_abs(1.875f, &i, &f, &f_neg_exp));
    CHECK_EQUAL(1, i);
    CHECK_EQUAL(875, f);
}

TEST(fsplit_abs, 9375) {
    CHECK(npf__fsplit_abs(1.9375f, &i, &f, &f_neg_exp));
    CHECK_EQUAL(1, i);
    CHECK_EQUAL(9375, f);
}

TEST(fsplit_abs, 96875) {
    CHECK(npf__fsplit_abs(1.96875f, &i, &f, &f_neg_exp));
    CHECK_EQUAL(1, i);
    CHECK_EQUAL(96875, f);
}

TEST(fsplit_abs, 984375) {
    CHECK(npf__fsplit_abs(1.984375f, &i, &f, &f_neg_exp));
    CHECK_EQUAL(1, i);
    CHECK_EQUAL(984375, f);
}

TEST(fsplit_abs, 9921875) {
    CHECK(npf__fsplit_abs(1.9921875f, &i, &f, &f_neg_exp));
    CHECK_EQUAL(1, i);
    CHECK_EQUAL(9921875, f);
}

// Divergent but split has full accuracy.

TEST(fsplit_abs, 9960938) {
    CHECK(npf__fsplit_abs(1.9960938f, &i, &f, &f_neg_exp));
    CHECK_EQUAL(1, i);
    CHECK_EQUAL(99609375, f);
}

// Truncations, continue adding mantissa bits

TEST(fsplit_abs, 9980469_Truncates) {
    CHECK(npf__fsplit_abs(1.9980469f, &i, &f, &f_neg_exp));
    CHECK_EQUAL(1, i);
    CHECK_EQUAL(99804687, f);  // 1.998046875 is stored.
}

TEST(fsplit_abs, 9990234_Truncates) {
    CHECK(npf__fsplit_abs(1.9990234f, &i, &f, &f_neg_exp));
    CHECK_EQUAL(1, i);
    CHECK_EQUAL(99902343, f);  // 1.9990234375 is stored.
}

TEST(fsplit_abs, 9995117_Truncates) {
    CHECK(npf__fsplit_abs(1.9995117f, &i, &f, &f_neg_exp));
    CHECK_EQUAL(1, i);
    CHECK_EQUAL(99951171, f);  // 1.99951171875 is stored.
}

TEST(fsplit_abs, 9997559_Truncates) {
    CHECK(npf__fsplit_abs(1.9997559f, &i, &f, &f_neg_exp));
    CHECK_EQUAL(1, i);
    CHECK_EQUAL(99975585, f);  // 1.999755859375 is stored.
}

TEST(fsplit_abs, 9998779_Truncates) {
    CHECK(npf__fsplit_abs(1.9998779f, &i, &f, &f_neg_exp));
    CHECK_EQUAL(1, i);
    CHECK_EQUAL(99987792, f);  // 1.9998779296875 is stored.
}

TEST(fsplit_abs, 999939_Truncates) {
    CHECK(npf__fsplit_abs(1.999939f, &i, &f, &f_neg_exp));
    CHECK_EQUAL(1, i);
    CHECK_EQUAL(99993896, f);  // 1.99993896484375 is stored.
}

TEST(fsplit_abs, 9999695_Truncates) {
    CHECK(npf__fsplit_abs(1.9999695f, &i, &f, &f_neg_exp));
    CHECK_EQUAL(1, i);
    CHECK_EQUAL(99996948, f);  // 1.999969482421875 is stored.
}

TEST(fsplit_abs, 9999847_Truncates) {
    CHECK(npf__fsplit_abs(1.9999847f, &i, &f, &f_neg_exp));
    CHECK_EQUAL(1, i);
    CHECK_EQUAL(99998474, f);  // 1.9999847412109375 is stored.
}

TEST(fsplit_abs, 9999924_Truncates) {
    CHECK(npf__fsplit_abs(1.9999924f, &i, &f, &f_neg_exp));
    CHECK_EQUAL(1, i);
    CHECK_EQUAL(99999237, f);  // 1.99999237060546875 is stored.
}

TEST(fsplit_abs, 9999962_Truncates) {
    CHECK(npf__fsplit_abs(1.9999962f, &i, &f, &f_neg_exp));
    CHECK_EQUAL(1, i);
    CHECK_EQUAL(99999618, f);  // 1.999996185302734375 is stored.
}

TEST(fsplit_abs, 9999981_Truncates) {
    CHECK(npf__fsplit_abs(1.9999981f, &i, &f, &f_neg_exp));
    CHECK_EQUAL(1, i);
    CHECK_EQUAL(99999809, f);  // 1.9999980926513671875 is stored.
}

TEST(fsplit_abs, 999999_Truncates) {
    CHECK(npf__fsplit_abs(1.999999f, &i, &f, &f_neg_exp));
    CHECK_EQUAL(1, i);
    CHECK_EQUAL(99999904, f);  // 1.99999904632568359375 is stored.
}

TEST(fsplit_abs, 9999995_Truncates) {
    CHECK(npf__fsplit_abs(1.9999995f, &i, &f, &f_neg_exp));
    CHECK_EQUAL(1, i);
    CHECK_EQUAL(99999952, f);  // 1.999999523162841796875 is stored.
}

TEST(fsplit_abs, 9999998_Truncates) {
    CHECK(npf__fsplit_abs(1.9999998f, &i, &f, &f_neg_exp));
    CHECK_EQUAL(1, i);
    CHECK_EQUAL(99999976, f);  // 1.9999997615814208984375 is stored.
}

TEST(fsplit_abs, 9999999_Truncates) {
    CHECK(npf__fsplit_abs(1.9999999f, &i, &f, &f_neg_exp));
    CHECK_EQUAL(1, i);
    CHECK_EQUAL(99999988, f);  //  1.99999988079071044921875 is stored.
}
