#include "unit_nanoprintf.h"
#include "doctest.h"

TEST_CASE("npf_bin_len") {
  MESSAGE("Sizeof uintmax_t: " << sizeof(uintmax_t));
  MESSAGE("Sizeof npf_uint_t: " << sizeof(npf_uint_t));
#if !defined(NANOPRINTF_32_BIT_TESTS) && defined(_MSC_VER)
  unsigned long x;
  REQUIRE(_BitScanReverse64(&x, 0x100000000));
  MESSAGE("_BSR64(0x100000000): " << x);
#endif
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

#ifndef NANOPRINTF_32_BIT_TESTS
  CHECK(npf_bin_len(0x8000000000ULL) == 40);
  CHECK(npf_bin_len(0x800000000000ULL) == 48);
  CHECK(npf_bin_len(0x80000000000000ULL) == 56);
  CHECK(npf_bin_len(0x8000000000000000ULL) == 64);
#endif
}
