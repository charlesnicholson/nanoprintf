#define NANOPRINTF_CONVERSION_BUFFER_SIZE    512
#define NANOPRINTF_CONVERSION_FLOAT_TYPE    uint32_t
#define NPF_ETOA_REV_CUSTOM_CONFIG

#include "unit_etoa_rev.cc"

TEST_CASE("etoa_rev_32") { npf_eg_invariants("uint32_t"); }
