#include "unit_nanoprintf.h"

#include <climits>
#include <cstring>
#include <string>

#if NANOPRINTF_HAVE_GCC_WARNING_PRAGMAS
  #pragma GCC diagnostic push
  #if NANOPRINTF_CLANG
    #pragma GCC diagnostic ignored "-Wformat-pedantic"
    #pragma GCC diagnostic ignored "-Wmissing-prototypes"
    #pragma GCC diagnostic ignored "-Wold-style-cast"
  #endif
#endif

void require_npf_itoa(char const *expected, npf_int_t val) {
  char buf[64];
  int const n = npf_itoa_rev(buf, val);
  buf[n] = '\0';
  REQUIRE(n == (int)strlen(expected));
  REQUIRE(std::string{buf} == std::string{expected});
}

TEST_CASE("npf_itoa_rev") {
  require_npf_itoa("0", 0);
  require_npf_itoa("1", 1);
  require_npf_itoa("9", 9);
  require_npf_itoa("01", 10);
  require_npf_itoa("24", 42);
  require_npf_itoa("99", 99);
  require_npf_itoa("001", 100);
  require_npf_itoa("321", 123);
  require_npf_itoa("999", 999);
  require_npf_itoa("0123456", 6543210);

  SUBCASE("max values") {
#if NANOPRINTF_USE_LARGE_FORMAT_SPECIFIERS == 1
#if INTMAX_MAX == 9223372036854775807ll
    require_npf_itoa("7085774586302733229", INTMAX_MAX);
#else
#error Unknown INTMAX_MAX here, please add another branch.
#endif
#else
#if INT_MAX == 2147483647
    require_npf_itoa("7463847412", INT_MAX);
#else
#error Unknown INT_MAX here, please add another branch.
#endif
#endif
  }

  SUBCASE("min values") {
#if NANOPRINTF_USE_LARGE_FORMAT_SPECIFIERS == 1
    require_npf_itoa("8085774586302733229", INTMAX_MIN);
#else
#if INT_MIN == -2147483648
    require_npf_itoa("8463847412", INT_MIN);
#else
#error Unknown INT_MIN here, please add another branch.
#endif
#endif
  }

  SUBCASE("negative values have minus sign stripped") {
    require_npf_itoa("1", -1);
    require_npf_itoa("5987987", -7897895);
    require_npf_itoa("12345", -54321);
  }
}
