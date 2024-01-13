#include "unit_nanoprintf.h"

#include <cmath>
#include <string>

#if NANOPRINTF_HAVE_GCC_WARNING_PRAGMAS
  #pragma GCC diagnostic push
  #if NANOPRINTF_CLANG
    #pragma GCC diagnostic ignored "-Wformat-pedantic"
    #pragma GCC diagnostic ignored "-Wmissing-prototypes"
  #endif
#endif

void require_fsplit_abs(float f,
                        uint64_t expected_int_part,
                        uint64_t expected_frac_part,
                        int expected_frac_base10_neg_e) {
  uint64_t int_part, frac_part;
  int frac_neg_exp;
  REQUIRE(npf_fsplit_abs(f, &int_part, &frac_part, &frac_neg_exp));
  REQUIRE(int_part == expected_int_part);
  REQUIRE(frac_part == expected_frac_part);
  REQUIRE(frac_neg_exp == expected_frac_base10_neg_e);
}

TEST_CASE("npf_fsplit_abs") {
  require_fsplit_abs(0.f, 0, 0, 0);
  require_fsplit_abs(1.f, 1, 0, 0);
  require_fsplit_abs(-1.f, 1, 0, 0);
  require_fsplit_abs(123456.f, 123456, 0, 0);
  require_fsplit_abs(-123456.f, 123456, 0, 0);
  require_fsplit_abs(powf(2.0f, 63.f), 9223372036854775808ULL, 0, 0);

  SUBCASE("exponent too large") {
    uint64_t i, f;
    int f_neg_exp;
    REQUIRE(!npf_fsplit_abs(powf(2.0f, 64.f), &i, &f, &f_neg_exp));
  }

  // fractional base-10 negative exponent
  require_fsplit_abs(0.03125f, 0, 3125, 1);
  require_fsplit_abs(0.0078125f, 0, 78125, 2);
  require_fsplit_abs(2.4414062E-4f, 0, 244140625, 3);
  require_fsplit_abs(3.8146973E-6f, 0, 381469726, 5);

  // perfectly-representable fractions, adding 1 bit to mantissa each time.
  require_fsplit_abs(1.5f, 1, 5, 0);
  require_fsplit_abs(1.625f, 1, 625, 0);
  require_fsplit_abs(1.875f, 1, 875, 0);
  require_fsplit_abs(1.9375f, 1, 9375, 0);
  require_fsplit_abs(1.96875f, 1, 96875, 0);
  require_fsplit_abs(1.984375f, 1, 984375, 0);
  require_fsplit_abs(1.9921875f, 1, 9921875, 0);

  require_fsplit_abs(1.9960938f, 1, 99609375, 0); // first truncation divergence.

  // truncations, but continue adding mantissa bits
  require_fsplit_abs(1.9980469f, 1, 998046875, 0); // 1.998046875 is stored.
  require_fsplit_abs(1.9990234f, 1, 999023437, 0); // 1.9990234375 is stored.
  require_fsplit_abs(1.9995117f, 1, 999511718, 0); // 1.99951171875 is stored.
  require_fsplit_abs(1.9997559f, 1, 999755859, 0); // 1.999755859375 is stored.
  require_fsplit_abs(1.9998779f, 1, 999877929, 0); // 1.9998779296875 is stored.
  require_fsplit_abs(1.999939f,  1, 999938964, 0); // 1.99993896484375 is stored.
  require_fsplit_abs(1.9999695f, 1, 999969482, 0); // 1.999969482421875 is stored.
  require_fsplit_abs(1.9999847f, 1, 999984741, 0); // 1.9999847412109375 is stored.
  require_fsplit_abs(1.9999924f, 1, 999992370, 0); // 1.99999237060546875 is stored.
  require_fsplit_abs(1.9999962f, 1, 999996185, 0); // 1.999996185302734375 is stored.
  require_fsplit_abs(1.9999981f, 1, 999998092, 0); // 1.9999980926513671875 is stored.
  require_fsplit_abs(1.999999f,  1, 999999046, 0); // 1.99999904632568359375 is stored.
  require_fsplit_abs(1.9999995f, 1, 999999523, 0); // 1.999999523162841796875 is stored.
  require_fsplit_abs(1.9999998f, 1, 999999761, 0); // 1.9999997615814208984375 is stored.
  require_fsplit_abs(1.9999999f, 1, 999999880, 0); // 1.99999988079071044921875 is stored.
}
