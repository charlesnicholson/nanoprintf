#define NANOPRINTF_USE_DIVISION_FREE_CONVERSION 1
#include "unit_nanoprintf.h"

#include <climits>
#include <cstdint>
#include <string>

static void require_divfree_utoa(
    std::string const &expected,
    npf_uint_t val,
    uint_fast8_t base,
    char case_adj = 'a' - 'A') {
  char buf[64];
  int const n = npf_utoa_rev(val, buf, base, case_adj);
  buf[n] = '\0';
  REQUIRE(n == (int)expected.size());
  REQUIRE(std::string{buf} == expected);
}

// First n in [lo, hi] (stepping by step) where npf_div10(n) != n / 10, or -1
// if the whole range is exact.
static int64_t first_div10_mismatch(uint64_t lo, uint64_t hi, uint64_t step) {
  for (uint64_t n = lo; n <= hi; n += step) {
    if (npf_div10((uint32_t)n) != (uint32_t)(n / 10u)) { return (int64_t)n; }
  }
  return -1;
}

TEST_CASE("npf_div10 is exact") {
  SUBCASE("exhaustive low range") {
    REQUIRE(first_div10_mismatch(0, 2000000, 1) == -1);
  }

  SUBCASE("power-of-ten boundaries") {
    for (uint64_t p = 10; p <= 1000000000u; p *= 10) {
      REQUIRE(first_div10_mismatch(p - 2, p + 2, 1) == -1);
    }
  }

  SUBCASE("power-of-two boundaries") {
    for (int k = 1; k < 32; ++k) {
      uint64_t const p = (uint64_t)1 << k;
      REQUIRE(first_div10_mismatch(p - 2, p + 2, 1) == -1);
    }
  }

  SUBCASE("top of range") {
    REQUIRE(first_div10_mismatch(0xFFFFF000u, 0xFFFFFFFFu, 1) == -1);
  }

  SUBCASE("strided full sweep") {
    REQUIRE(first_div10_mismatch(0, 0xFFFFFFFFu, 65521) == -1); // prime stride
  }
}

TEST_CASE("npf_utoa_rev division-free") {
  SUBCASE("base 10") {
    require_divfree_utoa("0", 0, 10);
    require_divfree_utoa("9", 9, 10);
    require_divfree_utoa("01", 10, 10);
    require_divfree_utoa("99", 99, 10);
    require_divfree_utoa("001", 100, 10);
    require_divfree_utoa("54321", 12345, 10);
    require_divfree_utoa("000001", 100000, 10);
  }

  SUBCASE("base 10 maxima") {
#if NANOPRINTF_USE_LARGE_FORMAT_SPECIFIERS == 1
#if UINTMAX_MAX == 18446744073709551615u
    require_divfree_utoa("51615590737044764481", UINTMAX_MAX, 10);
#else
#error Unknown UINTMAX_MAX here, please add another branch.
#endif
#else
#if UINT_MAX == 4294967295
    require_divfree_utoa("5927694924", UINT_MAX, 10);
#else
#error Unknown UINT_MAX here, please add another branch.
#endif
#endif
  }

  SUBCASE("base 8") {
    require_divfree_utoa("0", 0, 8);
    require_divfree_utoa("7", 7, 8);
    require_divfree_utoa("01", 010, 8);
    require_divfree_utoa("7654321", 01234567, 8);
#if NANOPRINTF_USE_LARGE_FORMAT_SPECIFIERS == 1
#if UINTMAX_MAX == 18446744073709551615u
    require_divfree_utoa("7777777777777777777771", UINTMAX_MAX, 8);
#endif
#else
#if UINT_MAX == 4294967295
    require_divfree_utoa("77777777773", UINT_MAX, 8);
#endif
#endif
  }

  SUBCASE("base 16") {
    require_divfree_utoa("0", 0, 16);
    require_divfree_utoa("f", 0xf, 16);
    require_divfree_utoa("4d3c2b1a", 0xa1b2c3d4, 16);
    require_divfree_utoa("FEDCBA98", 0x89abcdef, 16, 0);
#if NANOPRINTF_USE_LARGE_FORMAT_SPECIFIERS == 1
#if UINTMAX_MAX == 18446744073709551615u
    require_divfree_utoa("ffffffffffffffff", UINTMAX_MAX, 16);
#endif
#else
#if UINT_MAX == 4294967295
    require_divfree_utoa("ffffffff", UINT_MAX, 16);
#endif
#endif
  }
}
