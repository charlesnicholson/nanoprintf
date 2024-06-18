#include "unit_nanoprintf.h"

#include <climits>
#include <string>

#if NANOPRINTF_HAVE_GCC_WARNING_PRAGMAS
  #pragma GCC diagnostic push
  #if NANOPRINTF_CLANG
    #pragma GCC diagnostic ignored "-Wformat-pedantic"
    #pragma GCC diagnostic ignored "-Wold-style-cast"
    #pragma GCC diagnostic ignored "-Wmissing-prototypes"
  #endif
#endif

void require_npf_utoa(
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

TEST_CASE("npf_utoa_rev") {
  SUBCASE("base 10") {
    require_npf_utoa("0", 0, 10);
    require_npf_utoa("1", 1, 10);
    require_npf_utoa("9", 9, 10);
    require_npf_utoa("01", 10, 10);
    require_npf_utoa("31", 13, 10);
    require_npf_utoa("89", 98, 10);
    require_npf_utoa("99", 99, 10);
    require_npf_utoa("001", 100, 10);
    require_npf_utoa("321", 123, 10);
    require_npf_utoa("999", 999, 10);
    require_npf_utoa("0001", 1000, 10);
    require_npf_utoa("4321", 1234, 10);
    require_npf_utoa("9999", 9999, 10);
    require_npf_utoa("00001", 10000, 10);
    require_npf_utoa("54321", 12345, 10);
    require_npf_utoa("99999", 99999, 10);
    require_npf_utoa("000001", 100000, 10);
  }

  SUBCASE("base 10 maxima") {
#if NANOPRINTF_USE_LARGE_FORMAT_SPECIFIERS == 1
#if UINTMAX_MAX == 18446744073709551615u
    require_npf_utoa("51615590737044764481", UINTMAX_MAX, 10);
#else
#error Unknown UINTMAX_MAX here, please add another branch.
#endif
#else
#if UINT_MAX == 4294967295
    require_npf_utoa("5927694924", UINT_MAX, 10);
#else
#error Unknown UINT_MAX here, please add another branch.
#endif
#endif
  }

  SUBCASE("base 8") {
    require_npf_utoa("0", 0, 8);
    require_npf_utoa("1", 1, 8);
    require_npf_utoa("7", 7, 8);
    require_npf_utoa("01", 010, 8);
    require_npf_utoa("31", 013, 8);
    require_npf_utoa("71", 017, 8);
    require_npf_utoa("02", 020, 8);
    require_npf_utoa("72", 027, 8);
    require_npf_utoa("03", 030, 8);
    require_npf_utoa("77", 077, 8);
    require_npf_utoa("001", 0100, 8);
    require_npf_utoa("777", 0777, 8);
    require_npf_utoa("0001", 01000, 8);
    require_npf_utoa("7777", 07777, 8);
    require_npf_utoa("00001", 010000, 8);
    require_npf_utoa("77777", 077777, 8);
    require_npf_utoa("000001", 0100000, 8);
    require_npf_utoa("7654321", 01234567, 8);
  }

  SUBCASE("base 8 maxima") {
#if NANOPRINTF_USE_LARGE_FORMAT_SPECIFIERS == 1
#if UINTMAX_MAX == 18446744073709551615u
    require_npf_utoa("7777777777777777777771", UINTMAX_MAX, 8);
#else
#error Unknown UINTMAX_MAX here, please add another branch.
#endif
#else
#if UINT_MAX == 4294967295
    require_npf_utoa("77777777773", UINT_MAX, 8);
#else
#error Unknown UINT_MAX here, please add another branch.
#endif
#endif
  }

  SUBCASE("base 16 lowercase") {
    require_npf_utoa("0", 0, 16);
    require_npf_utoa("1", 1, 16);
    require_npf_utoa("f", 0xf, 16);
    require_npf_utoa("01", 0x10, 16);
    require_npf_utoa("c3", 0x3c, 16);
    require_npf_utoa("ff", 0xff, 16);
    require_npf_utoa("001", 0x100, 16);
    require_npf_utoa("fff", 0xfff, 16);
    require_npf_utoa("0001", 0x1000, 16);
    require_npf_utoa("ffff", 0xffff, 16);
    require_npf_utoa("00001", 0x10000, 16);
    require_npf_utoa("fffff", 0xfffff, 16);
    require_npf_utoa("000001", 0x100000, 16);
    require_npf_utoa("4d3c2b1a", 0xa1b2c3d4, 16);
  }

  SUBCASE("base 16 uppercase") {
    require_npf_utoa("12345", 0x54321, 16, 0);
    require_npf_utoa("FEDCBA98", 0x89abcdef, 16, 0);
  }

  SUBCASE("base 16 maxima") {
#if NANOPRINTF_USE_LARGE_FORMAT_SPECIFIERS == 1
#if UINTMAX_MAX == 18446744073709551615u
    require_npf_utoa("ffffffffffffffff", UINTMAX_MAX, 16);
#else
#error Unknown UINTMAX_MAX here, please add another branch.
#endif
#else
#if UINT_MAX == 4294967295
    require_npf_utoa("ffffffff", UINT_MAX, 16);
#else
#error Unknown UINT_MAX here, please add another branch.
#endif
#endif
  }
}
