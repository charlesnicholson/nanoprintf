#include "unit_nanoprintf.h"
#include "doctest.h"

TEST_CASE("npf_bin_len") {
  REQUIRE(npf_bin_len(0) == 1);
  REQUIRE(npf_bin_len(1) == 1);
  REQUIRE(npf_bin_len(0b10) == 2);
  REQUIRE(npf_bin_len(0b100) == 3);
  REQUIRE(npf_bin_len(0b1000) == 4);
  REQUIRE(npf_bin_len(0b10000) == 5);
  REQUIRE(npf_bin_len(0b100000) == 6);
  REQUIRE(npf_bin_len(0b1000000) == 7);
  REQUIRE(npf_bin_len(0b10000000) == 8);
  REQUIRE(npf_bin_len(0b100000001) == 9);
  REQUIRE(npf_bin_len(0b1000000001) == 10);
  REQUIRE(npf_bin_len(0b10000000001) == 11);
  REQUIRE(npf_bin_len(0b100000000001) == 12);
  REQUIRE(npf_bin_len(0b1000000000000) == 13);
  REQUIRE(npf_bin_len(0b10000000000000) == 14);
  REQUIRE(npf_bin_len(0b100000000000000) == 15);
  REQUIRE(npf_bin_len(0b1000000000000000) == 16);
  REQUIRE(npf_bin_len(0x80000000) == 32);

  if (sizeof(npf_uint_t) == 8) {
    REQUIRE(npf_bin_len(0x8000000000) == 40);
    REQUIRE(npf_bin_len(0x800000000000) == 48);
    REQUIRE(npf_bin_len(0x80000000000000) == 56);
    REQUIRE(npf_bin_len(0x8000000000000000) == 64);
  }
}
