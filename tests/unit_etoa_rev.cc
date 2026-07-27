#include "unit_nanoprintf.h"

#include <cmath>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <string>

// npf_etoa_rev composes into a reversed buffer, exactly like npf_ftoa_rev.
static npf_format_spec_t espec;

static void ememrev(char *lhs, char *rhs) {
  --rhs;
  while (lhs < rhs) {
    char c = *lhs;
    *lhs++ = *rhs;
    *rhs-- = c;
  }
}

static void require_etoa_rev(std::string const &expected, double dbl) {
  char buf[NPF_CBUF + 1];
  int const n = [&](){ int x = npf_etoa_rev(buf, &espec, dbl); return x < 0 ? -x : x; }();
  REQUIRE(n <= NANOPRINTF_CONVERSION_BUFFER_SIZE);
  ememrev(buf, &buf[n]);
  buf[n] = '\0';
  CHECK(std::string{buf} == expected);
  CHECK(n == (int)expected.size());
}

// ---------------------------------------------------------------------------
// Width-independent invariants. These hold for every NANOPRINTF_CONVERSION_FLOAT_TYPE
// because they compare nanoprintf against itself rather than against the system.

// Significant digits and base-10 exponent of an %e or %f rendering, with leading
// and trailing zeros removed so the two forms are directly comparable.
static bool npf_sig(char const *s, std::string &digits, int &exp10) {
  for (char const *p = s; *p; ++p) {
    char const c = (char)(*p | 32);
    if (c == 'n' || c == 'i' || c == 'r') { return false; } // nan / inf / err
  }
  std::string d;
  int point = -1, e = 0, esign = 1;
  char const *p = s;
  if (*p == '-' || *p == '+' || *p == ' ') { ++p; }
  for (; *p && *p != 'e' && *p != 'E'; ++p) {
    if (*p == '.') { point = (int)d.size(); }
    else if (*p >= '0' && *p <= '9') { d += *p; }
  }
  if (point < 0) { point = (int)d.size(); }
  if (*p == 'e' || *p == 'E') {
    ++p;
    if (*p == '-') { esign = -1; ++p; } else if (*p == '+') { ++p; }
    for (; *p >= '0' && *p <= '9'; ++p) { e = e * 10 + (*p - '0'); }
  }
  size_t lead = 0;
  while (lead + 1 < d.size() && d[lead] == '0') { ++lead; }
  size_t end = d.size();
  while (end > lead + 1 && d[end - 1] == '0') { --end; }
  digits = d.substr(lead, end - lead);
  exp10 = (digits == "0") ? 0 : (e * esign + (point - (int)lead - 1));
  return true;
}

/* An 8-bit NANOPRINTF_CONVERSION_FLOAT_TYPE resolves only about two significant
   digits (floor(8 * log10 2)), and %e and %f reach that second digit through
   different numbers of scaling steps, so they can legitimately disagree on it.
   Every wider intermediate agrees; the 08 variant pins concrete outputs instead.
   NPF_FTOA_MAN_BITS is an enum constant, so this is a run-time test the compiler
   folds rather than a preprocessor one. */
static bool const npf_eg_cross_form = (NPF_FTOA_MAN_BITS >= 16);

// %.Pe and %.(P-X)f must describe the same number with the same digits.
static void check_e_matches_f(double v, int prec) {
  if (!npf_eg_cross_form) { return; }
  char fmt[24], e[600], f[600];
  std::string de, df;
  int xe = 0, xf = 0;
  snprintf(fmt, sizeof fmt, "%%.%de", prec);
  npf_snprintf(e, sizeof e, fmt, v);
  if (!npf_sig(e, de, xe)) { return; }
  int const fp = prec - xe;
  if (fp < 0 || fp > 60 || std::fabs(v) >= 1e18) { return; }
  snprintf(fmt, sizeof fmt, "%%.%df", fp);
  npf_snprintf(f, sizeof f, fmt, v);
  if (!npf_sig(f, df, xf)) { return; }
  INFO("v=", v, " prec=", prec, " e=", e, " f=", f);
  CHECK(de == df);
  CHECK(xe == xf);
}

// C11 7.21.6.1p8 for %g, built out of nanoprintf's own %e and %f.
static void check_g_matches_spec(double v, int prec) {
  if (!npf_eg_cross_form) { return; }
  char fmt[24], g[600], e[600], want[600];
  std::string de;
  int xe = 0;
  int const P = prec ? prec : 1;
  snprintf(fmt, sizeof fmt, "%%.%dg", prec);
  npf_snprintf(g, sizeof g, fmt, v);
  { std::string junk; int j = 0; if (!npf_sig(g, junk, j)) { return; } }
  snprintf(fmt, sizeof fmt, "%%.%de", P - 1);
  npf_snprintf(e, sizeof e, fmt, v);
  if (!npf_sig(e, de, xe)) { return; }

  if (xe >= -4 && xe < P) {
    int fp = P - 1 - xe;
    if (fp < 0) { fp = 0; }
    snprintf(fmt, sizeof fmt, "%%.%df", fp);
    npf_snprintf(want, sizeof want, fmt, v);
    if (char *dot = strchr(want, '.')) { // drop trailing fraction zeros, then the point
      char *q = want + strlen(want) - 1;
      while (q > dot && *q == '0') { *q-- = '\0'; }
      if (q == dot) { *q = '\0'; }
    }
  } else {
    char *const ep = strpbrk(e, "eE");
    std::string mant(e, (size_t)(ep - e));
    size_t const dot = mant.find('.');
    if (dot != std::string::npos) {
      size_t last = mant.size();
      while (last > dot + 1 && mant[last - 1] == '0') { --last; }
      mant.resize(last == dot + 1 ? dot : last);
    }
    snprintf(want, sizeof want, "%s%s", mant.c_str(), ep);
  }
  INFO("v=", v, " prec=", prec);
  CHECK(std::string{g} == std::string{want});
}

static uint64_t npf_eg_rng_state;
static uint64_t npf_eg_rng(void) {
  npf_eg_rng_state ^= npf_eg_rng_state << 13;
  npf_eg_rng_state ^= npf_eg_rng_state >> 7;
  npf_eg_rng_state ^= npf_eg_rng_state << 17;
  return npf_eg_rng_state;
}

// Shared by every NANOPRINTF_CONVERSION_FLOAT_TYPE variant of this file.
static void npf_eg_invariants(char const *tag) {
  INFO("variant ", tag);
  static double const structured[] = {
    0.0, -0.0, 1.0, -1.0, 0.5, 1.5, 2.5, 0.1, 1.0 / 3.0, 100.0, 1e5, 1e-5,
    999999.0, 1000000.0, 1234567.0, 999.95, 9.9999, 0.0001, 0.00001, 1e10, 1e-10,
    3.14159265358979, 0.05, 123456.0, 0.000123456, 1e15, 1e-15, 1e300, 1e-300,
    0.9999999999, 8.5, 0.0078125, 9.5, 99.5, 999.5, 0.999999, DBL_MAX, DBL_MIN,
  };
  for (double v : structured) {
    for (int p = 0; p <= 24; ++p) {
      check_e_matches_f(v, p);
      check_e_matches_f(-v, p);
      check_g_matches_spec(v, p);
      check_g_matches_spec(-v, p);
    }
  }

  npf_eg_rng_state = 0x243F6A8885A308D3ull;
  for (int i = 0; i < 20000; ++i) { // arbitrary finite bit patterns
    union { uint64_t u; double d; } x;
    x.u = npf_eg_rng();
    if (std::isnan(x.d) || std::isinf(x.d)) { continue; }
    int const p = (int)(npf_eg_rng() % 22);
    check_e_matches_f(x.d, p);
    check_g_matches_spec(x.d, p);
  }
  for (int i = 0; i < 20000; ++i) { // values inside a everyday magnitude band
    double const m = (double)(npf_eg_rng() % 1000000000u) / 1e8;
    double const v = m * std::pow(10.0, (double)((int)(npf_eg_rng() % 41) - 20));
    if (std::isnan(v) || std::isinf(v)) { continue; }
    int const p = (int)(npf_eg_rng() % 18);
    check_e_matches_f(v, p);
    check_g_matches_spec(v, p);
  }
}

#ifndef NPF_ETOA_REV_CUSTOM_CONFIG

struct npf_eg_case { char const *fmt; double val; char const *expected; };
#include "unit_eg.inc"

static void run_eg_cases(npf_eg_case const *c, size_t n) {
  char buf[600];
  for (size_t i = 0; i < n; ++i) {
    npf_snprintf(buf, sizeof buf, c[i].fmt, c[i].val);
    INFO("fmt=", c[i].fmt, " val=", c[i].val);
    CHECK(std::string{buf} == std::string{c[i].expected});
  }
}

TEST_CASE("etoa_rev: exhaustive value and precision sweep") {
  run_eg_cases(npf_eg_cases, sizeof npf_eg_cases / sizeof *npf_eg_cases);
  run_eg_cases(npf_eg_cases_double_only,
               sizeof npf_eg_cases_double_only / sizeof *npf_eg_cases_double_only);
}

TEST_CASE("etoa_rev: invariants") { npf_eg_invariants("default"); }

TEST_CASE("etoa_rev") {
  memset(&espec, 0, sizeof(espec));
  espec.conv_spec = NPF_FMT_SPEC_CONV_FLOAT_SCI;
  espec.case_adjust = 'a' - 'A'; // lowercase, i.e. 'e' rather than 'E'

  SUBCASE("special values") {
    require_etoa_rev("nan", (double)+NAN);
    require_etoa_rev("nan", (double)-NAN);
    require_etoa_rev("inf", (double)+INFINITY);
    require_etoa_rev("inf", (double)-INFINITY);
    espec.case_adjust = 0;
    require_etoa_rev("NAN", (double)NAN);
    require_etoa_rev("INF", (double)INFINITY);
    require_etoa_rev("1E+00", 1.);
    espec.case_adjust = 'a' - 'A';
  }

  SUBCASE("precision that cannot fit reports err") {
    // The longest output is "d.<prec>e+ddd", so prec must leave 7 bytes of room.
    espec.prec = NPF_CBUF - 8;
    require_etoa_rev(std::string("1.") +
                     std::string((size_t)espec.prec, '0') + "e+00", 1.);
    espec.prec = NPF_CBUF - 7;
    require_etoa_rev("err", 1.);
    espec.prec = NANOPRINTF_CONVERSION_BUFFER_SIZE;
    require_etoa_rev("err", 1.);
    espec.case_adjust = 0;
    require_etoa_rev("ERR", 1.);
    espec.case_adjust = 'a' - 'A';
    espec.prec = 0;
  }

  SUBCASE("zero has exponent zero, not the subnormal exponent") {
    require_etoa_rev("0e+00", 0.);
    require_etoa_rev("0e+00", -0.);
    espec.prec = 3;
    require_etoa_rev("0.000e+00", 0.);
    espec.prec = 0;
  }

  SUBCASE("exponent is at least two digits and grows to three") {
    require_etoa_rev("1e+00", 1.);
    require_etoa_rev("1e+01", 10.);
    require_etoa_rev("1e-01", 0.1);
    require_etoa_rev("1e+09", 1e9);
    require_etoa_rev("1e+10", 1e10);
    require_etoa_rev("1e+99", 1e99);
    require_etoa_rev("1e+100", 1e100);
    require_etoa_rev("1e-100", 1e-100);
  }

  SUBCASE("rounding carries out of the leading digit") {
    require_etoa_rev("1e+01", 9.5);   // 9.5 -> 10 bumps the exponent
    require_etoa_rev("9e-01", 0.95);  // the double is 0.9499999..., so no carry
    require_etoa_rev("1e+00", 0.96);
    require_etoa_rev("1e+02", 99.5);
    require_etoa_rev("1e+03", 999.5);
    espec.prec = 2;
    require_etoa_rev("9.50e+00", 9.5);
    require_etoa_rev("1.00e+03", 999.5);
    espec.prec = 0;
  }

  SUBCASE("shortest strips trailing zeros but keeps integer zeros") {
    espec.conv_spec = NPF_FMT_SPEC_CONV_FLOAT_SHORTEST;
    espec.prec = 6;
    require_etoa_rev("0", 0.);
    require_etoa_rev("1", 1.);
    require_etoa_rev("100", 100.);       // must not become "1"
    require_etoa_rev("1.5", 1.5);
    require_etoa_rev("1e-05", 1e-5);     // X < -4 selects e style
    require_etoa_rev("0.0001", 1e-4);    // X == -4 selects f style
    require_etoa_rev("1e+06", 1e6);      // X == P selects e style
    require_etoa_rev("100000", 1e5);     // X == P-1 selects f style
    espec.prec = 0;
    require_etoa_rev("1", 1.);           // precision 0 means 1 significant digit
    espec.prec = 6;
    espec.conv_spec = NPF_FMT_SPEC_CONV_FLOAT_SCI;
  }
}

#endif // NPF_ETOA_REV_CUSTOM_CONFIG
