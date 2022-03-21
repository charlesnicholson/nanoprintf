#include "unit_nanoprintf.h"

#include <cstring>
#include <string>

#if NANOPRINTF_HAVE_GCC_WARNING_PRAGMAS
  #pragma GCC diagnostic push
  #if NANOPRINTF_CLANG
    #pragma GCC diagnostic ignored "-Wformat-pedantic"
  #endif
#endif

TEST_CASE("ftoa_rev") {
  char buf[64];
  int frac_bytes;
  memset(buf, 0, sizeof(buf));

  SUBCASE("zero") {
    REQUIRE(npf_ftoa_rev(buf, 0.f, 0, &frac_bytes) == 2);
    REQUIRE(std::string{".0"} == buf);
  }
}
