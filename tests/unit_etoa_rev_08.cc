#define NANOPRINTF_CONVERSION_BUFFER_SIZE    512
#define NANOPRINTF_CONVERSION_FLOAT_TYPE    uint8_t
#define NPF_ETOA_REV_CUSTOM_CONFIG

#include "unit_etoa_rev.cc"

// An 8-bit intermediate resolves about two significant digits, so the cross-form
// invariants are compiled out here (see NPF_EG_CROSS_FORM_CHECKS). Pin the concrete
// outputs instead: the point is that the narrowest possible intermediate still
// produces well-formed output with a correct exponent, not that it is accurate.
TEST_CASE("etoa_rev_08") {
  memset(&espec, 0, sizeof(espec));
  espec.conv_spec = NPF_FMT_SPEC_CONV_FLOAT_SCI;
  espec.case_adjust = 'a' - 'A';

  SUBCASE("exponent is right even when the digits run out") {
    require_etoa_rev("0e+00", 0.);
    require_etoa_rev("1e+00", 1.);
    require_etoa_rev("1e+01", 10.);
    require_etoa_rev("1e-01", 0.1);
    require_etoa_rev("3e+02", 255.);
    require_etoa_rev("1e+03", 1024.);
    espec.prec = 1;
    require_etoa_rev("1.0e+00", 1.);
    require_etoa_rev("1.5e+00", 1.5);
    require_etoa_rev("2.6e+02", 255.);
    espec.prec = 0;
  }

  SUBCASE("mantissa overflow still rounds and carries") {
    require_etoa_rev("3e+02", (npf_ftoa_man_t)-1 + 0.5); // 255.5
    espec.prec = 2;
    require_etoa_rev("2.56e+02", (npf_ftoa_man_t)-1 + 0.5);
    espec.prec = 0;
  }

  SUBCASE("shortest picks a style from the same exponent") {
    espec.conv_spec = NPF_FMT_SPEC_CONV_FLOAT_SHORTEST;
    espec.prec = 6;
    require_etoa_rev("0", 0.);
    require_etoa_rev("1", 1.);
    require_etoa_rev("100", 100.);
    require_etoa_rev("9.875e-06", 1e-5); // 8 bits cannot resolve 1e-5
    espec.prec = 0;
  }

  npf_eg_invariants("uint8_t");
}
