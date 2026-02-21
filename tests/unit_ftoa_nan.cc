#include "unit_nanoprintf.h"

#include <cfloat>
#include <cmath>
#include <cstring>
#include <string>

namespace {
double npf_u64_to_dbl(uint64_t v) {
  double d;
  memcpy(&d, &v, 8);
  return d;
}

double npf_nan(bool negative, bool quiet, uint64_t extra_payload) {
  // compile-time check that double fits in uint64_t
#if FLT_RADIX != 2 || DBL_MAX_EXP > 1024 || DBL_MANT_DIG > 53 || CHAR_BIT != 8
  #error Unsupported double format
#endif
  static_assert(sizeof(double) <= sizeof(uint64_t));

  // IEEE 754 floating-point values are encoded as <sign><exp><mantissa>
  // NAN has <exp> set to the maximum value (all 1s), and non-0 <mantissa> (to distinguish it from infinities)
  // A NAN value can encode arbitrary data in the mantissa (as long as it is non-0).
  // "signalling NANs" have the highest mantissa bit set to 0.
  // "quiet NANs" have the highest mantissa bit set to 1.

  int const DBL_MANT_BITS = DBL_MANT_DIG - 1; // 1 digit is implicit
  int const DBL_EXP_BITS = sizeof(double) * CHAR_BIT - 1 - DBL_MANT_BITS;
  int const DBL_SIGN_POS = DBL_EXP_BITS + DBL_MANT_BITS;

  extra_payload &= (1ull << DBL_MANT_BITS) - 1;

  if (quiet) {
    extra_payload |= 1ull << (DBL_MANT_BITS - 1);
  }

  if (extra_payload == 0) {
    extra_payload = 1;
  }

  uint64_t const u = 0ull
    | ((negative ? 1ull : 0ull) << DBL_SIGN_POS) // sign
    | (((1ull << DBL_EXP_BITS) - 1) << DBL_MANT_BITS) // make nan/inf exponent
    | extra_payload;

  return npf_u64_to_dbl(u);
}

std::string print_double(char const *fmt, double d) {
  char buf[128];
  npf_snprintf(buf, sizeof(buf), fmt, d);
  return buf;
}
}

TEST_CASE("nan") {
  REQUIRE(print_double("%f", npf_nan(0, 1, 0)) == "nan");
  REQUIRE(print_double("%f", npf_nan(0, 1, 1)) == "nan");
  REQUIRE(print_double("%f", npf_nan(0, 0, 0)) == "nan");
  REQUIRE(print_double("%f", npf_nan(0, 0, 1)) == "nan");
  REQUIRE(print_double("%F", npf_nan(0, 1, 0)) == "NAN");
  REQUIRE(print_double("%F", npf_nan(0, 1, 1)) == "NAN");
  REQUIRE(print_double("%F", npf_nan(0, 0, 0)) == "NAN");
  REQUIRE(print_double("%F", npf_nan(0, 0, 1)) == "NAN");
  REQUIRE(print_double("%06f", npf_nan(0, 1, 0)) == "   nan");
  REQUIRE(print_double("% 6f", npf_nan(0, 1, 0)) == "   nan");
  REQUIRE(print_double("% 1f", npf_nan(0, 1, 0)) == " nan");
  REQUIRE(print_double("%-6f", npf_nan(0, 1, 0)) == "nan   ");
  REQUIRE(print_double("%#6f", npf_nan(0, 1, 0)) == "   nan");
  REQUIRE(print_double("%#06f", npf_nan(0, 1, 0)) == "   nan");
  REQUIRE(print_double("%0.6f", npf_nan(0, 1, 0)) == "nan");
  REQUIRE(print_double("% .1f", npf_nan(0, 1, 0)) == " nan");
  REQUIRE(print_double("%-.6f", npf_nan(0, 1, 0)) == "nan");
  REQUIRE(print_double("%#.6f", npf_nan(0, 1, 0)) == "nan");
  REQUIRE(print_double("%#0.6f", npf_nan(0, 1, 0)) == "nan");

  REQUIRE(print_double("%f", npf_nan(1, 1, 0)) == "-nan");
  REQUIRE(print_double("%f", npf_nan(1, 1, 1)) == "-nan");
  REQUIRE(print_double("%f", npf_nan(1, 0, 0)) == "-nan");
  REQUIRE(print_double("%f", npf_nan(1, 0, 1)) == "-nan");
  REQUIRE(print_double("%F", npf_nan(1, 1, 0)) == "-NAN");
  REQUIRE(print_double("%F", npf_nan(1, 1, 1)) == "-NAN");
  REQUIRE(print_double("%F", npf_nan(1, 0, 0)) == "-NAN");
  REQUIRE(print_double("%F", npf_nan(1, 0, 1)) == "-NAN");
  REQUIRE(print_double("%06f", npf_nan(1, 1, 0)) == "  -nan");
  REQUIRE(print_double("%+6f", npf_nan(0, 1, 0)) == "  +nan");
  REQUIRE(print_double("%+6f", npf_nan(1, 1, 0)) == "  -nan");
  REQUIRE(print_double("%#6f", npf_nan(1, 1, 0)) == "  -nan");
  REQUIRE(print_double("%#06f", npf_nan(1, 1, 0)) == "  -nan");
  REQUIRE(print_double("% 6f", npf_nan(1, 1, 0)) == "  -nan");
  REQUIRE(print_double("% 1f", npf_nan(1, 1, 0)) == "-nan");
  REQUIRE(print_double("%-6f", npf_nan(1, 1, 0)) == "-nan  ");
  REQUIRE(print_double("%0.6f", npf_nan(1, 1, 0)) == "-nan");
  REQUIRE(print_double("%+.6f", npf_nan(0, 1, 0)) == "+nan");
  REQUIRE(print_double("%+.6f", npf_nan(1, 1, 0)) == "-nan");
  REQUIRE(print_double("%#.6f", npf_nan(1, 1, 0)) == "-nan");
  REQUIRE(print_double("%#0.6f", npf_nan(1, 1, 0)) == "-nan");
  REQUIRE(print_double("% .1f", npf_nan(1, 1, 0)) == "-nan");
  REQUIRE(print_double("%-.6f", npf_nan(1, 1, 0)) == "-nan");
}

TEST_CASE("inf") {
  REQUIRE(print_double("%0.6f", (double)INFINITY) == "inf");
  REQUIRE(print_double("%0.6f", (double)-INFINITY) == "-inf");
  REQUIRE(print_double("%+.6f", (double)INFINITY) == "+inf");
  REQUIRE(print_double("%+.6f", (double)-INFINITY) == "-inf");
  REQUIRE(print_double("%#.6f", (double)INFINITY) == "inf");
  REQUIRE(print_double("%#0.6f", (double)INFINITY) == "inf");
  REQUIRE(print_double("%#.6f", (double)-INFINITY) == "-inf");
  REQUIRE(print_double("%#0.6f", (double)-INFINITY) == "-inf");
  REQUIRE(print_double("% .1f", (double)INFINITY) == " inf");
  REQUIRE(print_double("%-.6f", (double)INFINITY) == "inf");
  REQUIRE(print_double("% .1f", (double)-INFINITY) == "-inf");
  REQUIRE(print_double("%-.6f", (double)-INFINITY) == "-inf");
  REQUIRE(print_double("%06f", (double)INFINITY) == "   inf");
  REQUIRE(print_double("%06f", (double)-INFINITY) == "  -inf");
  REQUIRE(print_double("%+6f", (double)INFINITY) == "  +inf");
  REQUIRE(print_double("%+6f", (double)-INFINITY) == "  -inf");
  REQUIRE(print_double("%#6f", (double)INFINITY) == "   inf");
  REQUIRE(print_double("%#6f", (double)-INFINITY) == "  -inf");
  REQUIRE(print_double("%#06f", (double)INFINITY) == "   inf");
  REQUIRE(print_double("%#06f", (double)-INFINITY) == "  -inf");
  REQUIRE(print_double("% 6f", (double)INFINITY) == "   inf");
  REQUIRE(print_double("% 6f", (double)-INFINITY) == "  -inf");
  REQUIRE(print_double("% 1f", (double)INFINITY) == " inf");
  REQUIRE(print_double("% 1f", (double)-INFINITY) == "-inf");
  REQUIRE(print_double("%-6f", (double)INFINITY) == "inf   ");
  REQUIRE(print_double("%-6f", (double)-INFINITY) == "-inf  ");
}

