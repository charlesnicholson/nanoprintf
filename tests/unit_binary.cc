#include "unit_nanoprintf.h"

#include <cmath>
#include <limits>
#include <string>

#if NANOPRINTF_HAVE_GCC_WARNING_PRAGMAS
  #pragma GCC diagnostic push
  #if NANOPRINTF_CLANG
    #pragma GCC diagnostic ignored "-Wformat-pedantic"
    #pragma GCC diagnostic ignored "-Wformat-nonliteral"
  #endif
#endif

TEST_CASE("npf_bin_len") {
  CHECK(npf_bin_len(0) == 1);
  CHECK(npf_bin_len(1) == 1);
  CHECK(npf_bin_len(0b10) == 2);
  CHECK(npf_bin_len(0b100) == 3);
  CHECK(npf_bin_len(0b1000) == 4);
  CHECK(npf_bin_len(0b10000) == 5);
  CHECK(npf_bin_len(0b100000) == 6);
  CHECK(npf_bin_len(0b1000000) == 7);
  CHECK(npf_bin_len(0b10000000) == 8);
  CHECK(npf_bin_len(0b100000001) == 9);
  CHECK(npf_bin_len(0b1000000001) == 10);
  CHECK(npf_bin_len(0b10000000001) == 11);
  CHECK(npf_bin_len(0b100000000001) == 12);
  CHECK(npf_bin_len(0b1000000000000) == 13);
  CHECK(npf_bin_len(0b10000000000000) == 14);
  CHECK(npf_bin_len(0b100000000000000) == 15);
  CHECK(npf_bin_len(0b1000000000000000) == 16);
  CHECK(npf_bin_len(0x80000000UL) == 32);

#if NANOPRINTF_USE_LARGE_FORMAT_SPECIFIERS == 1
  CHECK(npf_bin_len(0x8000000000ULL) == 40);
  CHECK(npf_bin_len(0x800000000000ULL) == 48);
  CHECK(npf_bin_len(0x80000000000000ULL) == 56);
  CHECK(npf_bin_len(0x8000000000000000ULL) == 64);
#endif
}

namespace {
void require_equal(char const *expected, char const *fmt, ...) {
  char buf[256];

  std::string npf_result; {
    va_list args;
    va_start(args, fmt);
    npf_vsnprintf(buf, sizeof(buf), fmt, args);
    va_end(args);
    buf[sizeof(buf)-1] = '\0';
    npf_result = buf;
  }

  REQUIRE(npf_result == std::string{expected});
}
}

TEST_CASE("binary") {
  SUBCASE("plain") {
    require_equal(   "0", "%b", 0);
    require_equal(   "1", "%b", 1);
    require_equal(  "10", "%b", 0b10);
    require_equal(  "11", "%b", 0b11);
    require_equal( "100", "%b", 0b100);
    require_equal( "101", "%b", 0b101);
    require_equal( "110", "%b", 0b110);
    require_equal( "110", "%b", 0b110);
    require_equal( "111", "%b", 0b111);
    require_equal("1000", "%b", 0b1000);
    require_equal("10000", "%b", 0b10000);
    require_equal("100000", "%b", 0b100000);
    require_equal("1000000", "%b", 0b1000000);
    require_equal("10000000", "%b", 0b10000000);
    require_equal(   "10010001101000101011001111000", "%b", 0x12345678);
    require_equal( "1010101010101010101010101010101", "%b", 0x55555555);
    require_equal("10101010101010101010101010101010", "%b", 0xAAAAAAAA);
    require_equal("11111111111111111111111111111111", "%b", 0xFFFFFFFF);
  }

  SUBCASE("length") {
    require_equal("11111111", "%hhb", 0xFFFFFFFF); // char
    require_equal("1111111111111111", "%hb", 0xFFFFFFFF); // short
  }

#if NANOPRINTF_USE_PRECISION_FORMAT_SPECIFIERS == 1
  SUBCASE("precision") {
    require_equal(                "", "%.0b", 0);
    require_equal(               "0", "%.1b", 0);
    require_equal(              "00", "%.2b", 0);
    require_equal(             "000", "%.3b", 0);
    require_equal(            "0000", "%.4b", 0);
    require_equal(           "00000", "%.5b", 0);
    require_equal(          "000000", "%.6b", 0);
    require_equal(         "0000000", "%.7b", 0);
    require_equal(        "00000000", "%.8b", 0);
    require_equal(       "000000000", "%.9b", 0);
    require_equal(      "0000000000", "%.10b", 0);
    require_equal(     "00000000000", "%.11b", 0);
    require_equal(    "000000000000", "%.12b", 0);
    require_equal(   "0000000000000", "%.13b", 0);
    require_equal(  "00000000000000", "%.14b", 0);
    require_equal( "000000000000000", "%.15b", 0);
    require_equal("0000000000000000", "%.16b", 0);
    require_equal("00001111", "%.8b", 0b1111);

    require_equal("00000000000000000000000000000000", "%.32b", 0);
  }
#endif

#if NANOPRINTF_USE_FIELD_WIDTH_FORMAT_SPECIFIERS == 1
  SUBCASE("field width") {
    require_equal(   "0", "%1b", 0);
    require_equal("   0", "%4b", 0);
    require_equal("  11", "%4b", 0b11);
  }
#endif

#if NANOPRINTF_USE_LARGE_FORMAT_SPECIFIERS == 1
  SUBCASE("large") {
    require_equal("100000000000000000000000000000000", "%llb", 0x100000000ULL);
    require_equal("100000000000000000000000000000001", "%llb", 0x100000001ULL);
    require_equal("111111111111111111111111111111111111", "%llb", 0xFFFFFFFFFULL);

    require_equal("1000000000000000000000000000000000000000000000000000000000000000",
                  "%llb",
                  std::numeric_limits<long long int>::min());

    require_equal( "111111111111111111111111111111111111111111111111111111111111111",
                  "%llb",
                  std::numeric_limits<long long int>::max());
  }
#endif

  SUBCASE("alternate form") {
    require_equal("0", "%#b", 0);
    require_equal("0b1", "%#b", 1);
    require_equal("0B1", "%#B", 1);
    require_equal("0b110101", "%#b", 0b110101);
#if NANOPRINTF_USE_FIELD_WIDTH_FORMAT_SPECIFIERS == 1
    require_equal("       0", "%#8b", 0);
    require_equal("    0b11", "%#8b", 0b11);
#if NANOPRINTF_USE_PRECISION_FORMAT_SPECIFIERS == 1
    require_equal("    0b0001", "%#10.4b", 1);
#endif
#endif
  }
}
