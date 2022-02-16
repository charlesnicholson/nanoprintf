#include "unit_nanoprintf.h"
#include "doctest.h"

#include <cstring>
#include <string>
#include <iostream>

#if NANOPRINTF_CLANG_OR_GCC_PAST_4_6
  #pragma GCC diagnostic push
  #pragma GCC diagnostic ignored "-Wc++98-compat-pedantic"
  #pragma GCC diagnostic ignored "-Wc++98-c++11-compat-binary-literal"
  #pragma GCC diagnostic ignored "-Wold-style-cast"
#endif

TEST_CASE("ftoa_rev") {
  char buf[64];
  int frac_bytes;
  npf_format_spec_conversion_case_t const lower = NPF_FMT_SPEC_CONV_CASE_LOWER;
  memset(buf, 0, sizeof(buf));

  SUBCASE("zero") {
    REQUIRE(npf_ftoa_rev(buf, 0.f, 10, lower, &frac_bytes) == 2);
    REQUIRE(std::string{".0"} == buf);
  }
}
