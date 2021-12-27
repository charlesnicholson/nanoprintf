#include "unit_nanoprintf.h"
#include "doctest.h"

#include <string>
#include <iostream>

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
    require_equal("11111111111111111111111111111111", "%b", 0xFFFFFFFF);
    require_equal("10101010101010101010101010101010", "%b", 0xAAAAAAAA);
    require_equal( "1010101010101010101010101010101", "%b", 0x55555555);
    require_equal(   "10010001101000101011001111000", "%b", 0x12345678);
  }
}
