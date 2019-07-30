#include "nanoprintf_in_unit_tests.h"

#include "CppUTest/TestHarness.h"

#include <cstring>

TEST_GROUP(utoa) { char buf[64]; };

#define NPF_UTOA_CHECK(EXPECT_STR, VAL, BASE, CASE)                       \
    do {                                                                  \
        int const n =                                                     \
            npf__utoa_rev(buf, VAL, BASE, NPF_FMT_SPEC_CONV_CASE_##CASE); \
        CHECK_EQUAL((int)strlen(EXPECT_STR), n);                          \
        buf[n] = 0;                                                       \
        STRCMP_EQUAL(EXPECT_STR, buf);                                    \
    } while (0)

#define NPF_UTOA_CHECK_OCT(EXPECT_STR, VAL) \
    NPF_UTOA_CHECK(EXPECT_STR, VAL, 8, LOWER)

#define NPF_UTOA_CHECK_B10(EXPECT_STR, VAL) \
    NPF_UTOA_CHECK(EXPECT_STR, VAL, 10, LOWER)

#define NPF_UTOA_CHECK_HEX(EXPECT_STR, VAL, CASE) \
    NPF_UTOA_CHECK(EXPECT_STR, VAL, 16, CASE)

// Base 10

TEST(utoa, 0_Base10) { NPF_UTOA_CHECK_B10("0", 0); }
TEST(utoa, 1_Base10) { NPF_UTOA_CHECK_B10("1", 1); }
TEST(utoa, 9_Base10) { NPF_UTOA_CHECK_B10("9", 9); }
TEST(utoa, 10_Base10) { NPF_UTOA_CHECK_B10("01", 10); }
TEST(utoa, 13_Base10) { NPF_UTOA_CHECK_B10("31", 13); }
TEST(utoa, 98_Base10) { NPF_UTOA_CHECK_B10("89", 98); }
TEST(utoa, 99_Base10) { NPF_UTOA_CHECK_B10("99", 99); }
TEST(utoa, 100_Base10) { NPF_UTOA_CHECK_B10("001", 100); }
TEST(utoa, 123_Base10) { NPF_UTOA_CHECK_B10("321", 123); }
TEST(utoa, 999_Base10) { NPF_UTOA_CHECK_B10("999", 999); }
TEST(utoa, 1000_Base10) { NPF_UTOA_CHECK_B10("0001", 1000); }
TEST(utoa, 1234_Base10) { NPF_UTOA_CHECK_B10("4321", 1234); }
TEST(utoa, 9999_Base10) { NPF_UTOA_CHECK_B10("9999", 9999); }
TEST(utoa, 10000_Base10) { NPF_UTOA_CHECK_B10("00001", 10000); }
TEST(utoa, 12345_Base10) { NPF_UTOA_CHECK_B10("54321", 12345); }
TEST(utoa, 99999_Base10) { NPF_UTOA_CHECK_B10("99999", 99999); }
TEST(utoa, 100000_Base10) { NPF_UTOA_CHECK_B10("000001", 100000); }

TEST(utoa, Max_Base10) {
#if NANOPRINTF_USE_LARGE_FORMAT_SPECIFIERS == 1
#if UINTMAX_MAX == 18446744073709551615u
    NPF_UTOA_CHECK_B10("51615590737044764481", UINTMAX_MAX);
#else
#error Unknown UINTMAX_MAX here, please add another branch.
#endif
#else
#if UINT_MAX == 4294967295
    NPF_UTOA_CHECK_B10("5927694924", UINT_MAX);
#else
#error Unknown UINT_MAX here, please add another branch.
#endif
#endif
}

// Octal

TEST(utoa, 0_Octal) { NPF_UTOA_CHECK_OCT("0", 0); }
TEST(utoa, 1_Octal) { NPF_UTOA_CHECK_OCT("1", 1); }
TEST(utoa, 7_Octal) { NPF_UTOA_CHECK_OCT("7", 7); }
TEST(utoa, 10_Octal) { NPF_UTOA_CHECK_OCT("01", 010); }
TEST(utoa, 13_Octal) { NPF_UTOA_CHECK_OCT("31", 013); }
TEST(utoa, 17_Octal) { NPF_UTOA_CHECK_OCT("71", 017); }
TEST(utoa, 20_Octal) { NPF_UTOA_CHECK_OCT("02", 020); }
TEST(utoa, 27_Octal) { NPF_UTOA_CHECK_OCT("72", 027); }
TEST(utoa, 30_Octal) { NPF_UTOA_CHECK_OCT("03", 030); }
TEST(utoa, 77_Octal) { NPF_UTOA_CHECK_OCT("77", 077); }
TEST(utoa, 100_Octal) { NPF_UTOA_CHECK_OCT("001", 0100); }
TEST(utoa, 777_Octal) { NPF_UTOA_CHECK_OCT("777", 0777); }
TEST(utoa, 1000_Octal) { NPF_UTOA_CHECK_OCT("0001", 01000); }
TEST(utoa, 7777_Octal) { NPF_UTOA_CHECK_OCT("7777", 07777); }
TEST(utoa, 10000_Octal) { NPF_UTOA_CHECK_OCT("00001", 010000); }
TEST(utoa, 77777_Octal) { NPF_UTOA_CHECK_OCT("77777", 077777); }
TEST(utoa, 100000_Octal) { NPF_UTOA_CHECK_OCT("000001", 0100000); }
TEST(utoa, 1234567_Octal) { NPF_UTOA_CHECK_OCT("7654321", 01234567); }

TEST(utoa, Max_Octal) {
#if NANOPRINTF_USE_LARGE_FORMAT_SPECIFIERS == 1
#if UINTMAX_MAX == 18446744073709551615u
    NPF_UTOA_CHECK_OCT("7777777777777777777771", UINTMAX_MAX);
#else
#error Unknown UINTMAX_MAX here, please add another branch.
#endif
#else
#if UINT_MAX == 4294967295
    NPF_UTOA_CHECK_OCT("77777777773", UINT_MAX);
#else
#error Unknown UINT_MAX here, please add another branch.
#endif
#endif
}

// Hex

TEST(utoa, 0_Hex) { NPF_UTOA_CHECK_HEX("0", 0, LOWER); }
TEST(utoa, 1_Hex) { NPF_UTOA_CHECK_HEX("1", 1, LOWER); }
TEST(utoa, f_Hex) { NPF_UTOA_CHECK_HEX("f", 0xf, LOWER); }
TEST(utoa, 10_Hex) { NPF_UTOA_CHECK_HEX("01", 0x10, LOWER); }
TEST(utoa, 3c_Hex) { NPF_UTOA_CHECK_HEX("c3", 0x3c, LOWER); }
TEST(utoa, ff_Hex) { NPF_UTOA_CHECK_HEX("ff", 0xff, LOWER); }
TEST(utoa, 100_Hex) { NPF_UTOA_CHECK_HEX("001", 0x100, LOWER); }
TEST(utoa, fff_Hex) { NPF_UTOA_CHECK_HEX("fff", 0xfff, LOWER); }
TEST(utoa, 1000_Hex) { NPF_UTOA_CHECK_HEX("0001", 0x1000, LOWER); }
TEST(utoa, ffff_Hex) { NPF_UTOA_CHECK_HEX("ffff", 0xffff, LOWER); }
TEST(utoa, 10000_Hex) { NPF_UTOA_CHECK_HEX("00001", 0x10000, LOWER); }
TEST(utoa, fffff_Hex) { NPF_UTOA_CHECK_HEX("fffff", 0xfffff, LOWER); }
TEST(utoa, 100000_Hex) { NPF_UTOA_CHECK_HEX("000001", 0x100000, LOWER); }
TEST(utoa, a1b2c3d4_Hex) { NPF_UTOA_CHECK_HEX("4d3c2b1a", 0xa1b2c3d4, LOWER); }

TEST(utoa, Max_Hex) {
#if NANOPRINTF_USE_LARGE_FORMAT_SPECIFIERS == 1
#if UINTMAX_MAX == 18446744073709551615u
    NPF_UTOA_CHECK_HEX("ffffffffffffffff", UINTMAX_MAX, LOWER);
#else
#error Unknown UINTMAX_MAX here, please add another branch.
#endif
#else
#if UINT_MAX == 4294967295
    NPF_UTOA_CHECK_HEX("ffffffff", UINT_MAX, LOWER);
#else
#error Unknown UINT_MAX here, please add another branch.
#endif
#endif
}

TEST(utoa, UpperDigits_Hex) { NPF_UTOA_CHECK_HEX("12345", 0x54321, UPPER); }
TEST(utoa, UpperAlpha_Hex) {
    NPF_UTOA_CHECK_HEX("FEDCBA98", 0x89abcdef, UPPER);
}
