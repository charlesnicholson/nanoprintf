#include "unit_nanoprintf.h"

#include <cmath>
#include <cstring>
#include <string>

#if NANOPRINTF_HAVE_GCC_WARNING_PRAGMAS
  #pragma GCC diagnostic push
  #pragma GCC diagnostic ignored "-Wunused-function"
  #if NANOPRINTF_CLANG
    #pragma GCC diagnostic ignored "-Wformat-pedantic"
    #pragma GCC diagnostic ignored "-Wmissing-prototypes"
    #pragma GCC diagnostic ignored "-Wold-style-cast"
  #endif
#endif

static npf_format_spec_t spec;

static void memrev(char *lhs, char *rhs) {
  --rhs;
  while (lhs < rhs) {
    char c = *lhs;
    *lhs++ = *rhs;
    *rhs-- = c;
  }
}

static void require_ftoa_rev(std::string const &expected, double dbl) {
  char buf[NANOPRINTF_CONVERSION_BUFFER_SIZE + 1];
  int const n = npf_ftoa_rev(buf, &spec, dbl);
  REQUIRE(n <= NANOPRINTF_CONVERSION_BUFFER_SIZE);
  memrev(buf, &buf[n]);
  buf[n] = '\0';
  CHECK(std::string{buf} == std::string{expected});
  CHECK(n == (int)expected.size());
}

static void require_ftoa_rev_bin(char const *expected, npf_double_bin_t bin) {
  double dbl;
  memcpy(&dbl, &bin, sizeof(dbl));
  require_ftoa_rev(expected, dbl);
}

TEST_CASE("ftoa_rev") {
  memset(&spec, 0, sizeof(spec));

  SUBCASE("special values") {
    require_ftoa_rev("NAN", (double)+NAN);
    require_ftoa_rev("NAN", (double)-NAN);
    require_ftoa_rev("INF", (double)+INFINITY);
    require_ftoa_rev("INF", (double)-INFINITY);
    require_ftoa_rev("ERR", DBL_MAX);
    spec.prec = NANOPRINTF_CONVERSION_BUFFER_SIZE - 2;
    require_ftoa_rev("ERR", 10.);
    spec.prec += 1;
    require_ftoa_rev("ERR", 9.);
    spec.case_adjust = 'a' - 'A'; // lowercase
    require_ftoa_rev("err", 0.);
  }

  SUBCASE("zero and decimal separator") {
    require_ftoa_rev("0", +0.);
    require_ftoa_rev("0", -0.);
    spec.alt_form = '#';
    require_ftoa_rev("0.", 0.);
    spec.prec = 1;
    require_ftoa_rev("0.0", 0.);
  }

  SUBCASE("rounding") {
    require_ftoa_rev("9", 8.5);
    require_ftoa_rev("10", 9.5);
    require_ftoa_rev("49", 48.5);
    require_ftoa_rev("50", 49.5);
    require_ftoa_rev("99", 98.5);
    require_ftoa_rev("100", 99.5);

    require_ftoa_rev("0", 0.40625);
    require_ftoa_rev("1", 0.5);

    spec.prec = 1;
    require_ftoa_rev("0.3", 0.34375);
    require_ftoa_rev("0.3", 0.25);
    require_ftoa_rev("0.9", 0.9375);
    require_ftoa_rev("1.0", 0.96875);

    spec.prec = 4;
    require_ftoa_rev("0.9375", 0.9375);
    require_ftoa_rev("0.9688", 0.96875);
  }
}
