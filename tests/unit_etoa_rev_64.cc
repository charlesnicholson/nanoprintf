#define NANOPRINTF_CONVERSION_BUFFER_SIZE    512
#define NANOPRINTF_CONVERSION_FLOAT_TYPE    uint64_t
#define NPF_ETOA_REV_CUSTOM_CONFIG

#include "unit_etoa_rev.cc"

TEST_CASE("etoa_rev_64") { npf_eg_invariants("uint64_t"); }

/* A 64-bit intermediate holds a double's whole binary fraction, so a remainder of
   exactly one half really is a tie. An integer in [2^52, 2^53) gets to digit
   generation with no base-10 scaling of the integer part and no fraction bits at
   all, so the digits in the buffer are the entire expansion. */
TEST_CASE("etoa_rev_64: exact integer ties round to even") {
  memset(&espec, 0, sizeof(espec));
  espec.conv_spec = NPF_FMT_SPEC_CONV_FLOAT_SCI;
  espec.case_adjust = 'a' - 'A';
  espec.prec = 14;
  require_etoa_rev("4.50359962737050e+15", 4503599627370505.0);
  require_etoa_rev("9.00719925474098e+15", 9007199254740985.0);
  require_etoa_rev("4.50359962737052e+15", 4503599627370515.0); // odd kept digit
  require_etoa_rev("2.25179981368524e+15", 2251799813685245.0); // below 2^52
}
