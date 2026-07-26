#include "unit_nanoprintf.h"

#include "npf_f_paths.h"

#include <cmath>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <string>

/* %f has two implementations (see npf_f_paths.h). Everywhere both produce a number
   they must produce the same one; the only licensed divergence is at the "err"
   boundary, where the shared generator can fill the whole conversion buffer while
   the fused one stops during generation. So a difference is a failure unless the
   fused path is the one that gave up. */

namespace {

bool is_err(char const *s) { return strstr(s, "err") || strstr(s, "ERR"); }

int renders_more; // fused said err, unified produced digits

void compare(char const *fmt, double v) {
  char a[600], b[600];
  int const ra = npf_f_fused(a, sizeof a, fmt, v);
  int const rb = npf_f_unified(b, sizeof b, fmt, v);
  if (is_err(a) && !is_err(b)) { ++renders_more; return; }
  INFO("fmt=", fmt, " v=", v);
  CHECK(std::string{a} == std::string{b});
  CHECK(ra == rb);
}

uint64_t rng_state;
uint64_t rng() {
  rng_state ^= rng_state << 13;
  rng_state ^= rng_state >> 7;
  rng_state ^= rng_state << 17;
  return rng_state;
}

char const *const kPatterns[] = {
  "%%.%df", "%%.%dF", "%%#.%df", "%%+.%df", "%% .%df",
  "%%20.%df", "%%-20.%df", "%%020.%df", "%%1.%df", "%%0.%df", "%%40.%df",
};
constexpr size_t kNumPatterns = sizeof kPatterns / sizeof *kPatterns;

} // namespace

TEST_CASE("the two %f paths agree") {
  char fmt[24];
  renders_more = 0;

  static double const vals[] = {
    0.0, -0.0, 1.0, -1.0, 0.5, 1.5, 2.5, 0.1, 1.0 / 3.0, 100.0, 1e5, 1e-5,
    999999.0, 1e6, 1234567.0, 999.95, 9.9999, 1e-4, 0.0001, 3.14159265358979,
    0.05, 123456.0, 0.000123456, 1e15, 1e-15, 5e-324, 1e-300, 1e300, 0.9999999999,
    8.5, 0.0078125, 9.5, 99.5, 999.5, 0.999999, 0.0625, 1e20, 1e-20, 42.5, 255.0,
    1024.5, 0.001, DBL_MAX, DBL_MIN,
  };
  for (double v : vals) {
    for (int p = 0; p <= 40; ++p) {
      for (size_t k = 0; k < kNumPatterns; ++k) {
        snprintf(fmt, sizeof fmt, kPatterns[k], p);
        compare(fmt, v);
        compare(fmt, -v);
      }
    }
  }

  compare("%f", (double)INFINITY);
  compare("%f", -(double)INFINITY);
  compare("%F", (double)NAN);
  compare("%.3f", (double)NAN);

  rng_state = 0x1234567089ABCDEFull;
  for (int i = 0; i < 150000; ++i) { // arbitrary finite bit patterns
    union { uint64_t u; double d; } x;
    x.u = rng();
    if (std::isnan(x.d) || std::isinf(x.d)) { continue; }
    snprintf(fmt, sizeof fmt, kPatterns[rng() % kNumPatterns], (int)(rng() % 45));
    compare(fmt, x.d);
  }
  for (int i = 0; i < 150000; ++i) { // everyday magnitudes
    double const m = (double)(rng() % 1000000000u) / 1e8;
    double const v = m * std::pow(10.0, (double)((int)(rng() % 61) - 30));
    if (std::isnan(v) || std::isinf(v)) { continue; }
    snprintf(fmt, sizeof fmt, kPatterns[rng() % kNumPatterns], (int)(rng() % 45));
    compare(fmt, v);
  }

  // The shared generator should be reaching cases the fused one declines, otherwise
  // the sweep has drifted away from the buffer boundary and stopped covering it.
  CHECK(renders_more > 0);
}
