// TODO: we should run the tests at least twice, once with the default buffer size,
// and once with the extended one; or, we should move the test cases requiring
// high precision to a different test file.
// Also, here we test with a wide NANOPRINTF_CONVERSION_FLOAT_TYPE, but we
// should also test with a narrower one (at least with numbers that can fit in
// there
#define NANOPRINTF_CONVERSION_FLOAT_TYPE     uint64_t
#define NANOPRINTF_CONVERSION_BUFFER_SIZE    60
#define NANOPRINTF_VISIBILITY_STATIC
#define NANOPRINTF_USE_FIELD_WIDTH_FORMAT_SPECIFIERS    1
#define NANOPRINTF_USE_PRECISION_FORMAT_SPECIFIERS      1
#define NANOPRINTF_USE_FLOAT_FORMAT_SPECIFIERS          1
#define NANOPRINTF_USE_LARGE_FORMAT_SPECIFIERS          1
#define NANOPRINTF_USE_BINARY_FORMAT_SPECIFIERS         1
#define NANOPRINTF_USE_WRITEBACK_FORMAT_SPECIFIERS      1
#define NANOPRINTF_USE_FLOAT_DEC_FORMAT_SPECIFIER       1
#define NANOPRINTF_USE_FLOAT_SCI_FORMAT_SPECIFIER       1
#define NANOPRINTF_USE_FLOAT_SHORTEST_FORMAT_SPECIFIER  1
#define NANOPRINTF_USE_FLOAT_HEX_FORMAT_SPECIFIER       0
#define NANOPRINTF_USE_SMALL_FORMAT_SPECIFIERS          1
#define NANOPRINTF_USE_ALT_FORM_FLAG                    1
#define NANOPRINTF_IMPLEMENTATION
#include "nanoprintf.h"
#include <math.h>
#include <stdbool.h>
#include <stdio.h>
#include <string.h>

#define PR(...)     pr(__LINE__, 0, __VA_ARGS__)
#define PRC(...)    pr(__LINE__, 1, __VA_ARGS__)

void pr(int line, bool check_sys, const char *expected, const char *format, ...)
{
  va_list list;
  va_start(list, format);

#if 0
  printf("%6d:\"", line);
  vprintf(format, list);
  puts("\"");
#else
  va_list list2;
  va_start(list2, format);
  static char buffer_std[4096];
  static char buffer_npf[4096];
  int n_std = -1;
  if (check_sys) {
    n_std = vsnprintf(buffer_std, sizeof(buffer_std), format, list);
  }
  int n_npf = npf_vsnprintf(buffer_npf, sizeof(buffer_npf), format, list2);
  if((expected != NULL && strcmp(buffer_npf, expected) != 0)
      || (check_sys && strcmp(buffer_npf, buffer_std) != 0)) {
    printf("!%d: |%s|%s|%s|\n", line, buffer_npf, buffer_std, expected ? expected : "<none>");
  } else {
    //printf("*%d: |%s|%s|%s|\n", line, buffer_npf, buffer_std, expected ? expected : "<none>");
  }
  int len = strlen(buffer_npf);
  if(len != n_npf ||
      (check_sys && len != n_std)) {
    printf("!%d (len): |%s|%s|%s|, %d %d %d\n", line, buffer_npf, buffer_std, expected ? expected : "<none>", len, n_npf, n_std);
  }
  va_end(list2);
#endif

  va_end(list);
}

#if defined(__clang__) || defined(__GNUC__) || defined(__GNUG__)
  #define NPF_MAYBE_UNUSED    __attribute__((unused))
#elif defined(_MSC_VER)
  #define NPF_MAYBE_UNUSED
#else
  #define NPF_MAYBE_UNUSED
#endif

#if defined(__cpp_static_assert)
  #define NPF_STATIC_ASSERT_MSG(cond, msg)    static_assert(cond, msg)
#elif defined(__STDC_VERSION__) && __STDC_VERSION__ >= 201112L
  #define NPF_STATIC_ASSERT_MSG(cond, msg)    _Static_assert(cond, msg)
#else
  #define NPF_MAYBE_UNUSED
  #define NPF_CAT_HELPER(a, b)    a ## b
  #define NPF_CAT(a, b)    NPF_CAT(a, b)
  // with __COUNTER__, it works even with multiple assertions on a single line
  #if defined(__COUNTER__)
    #define NPF_STATIC_ASSERT_MSG(cond, msg) \
      static NPF_MAYBE_UNUSED const char *NPF_CAT(npf_static_assert, __COUNTER__)[(cond) * 2 - 1] = { msg }
  #else
    #define NPF_STATIC_ASSERT_MSG(cond, msg) \
      static NPF_MAYBE_UNUSED const char *NPF_CAT(npf_static_assert, __LINE__)[(cond) * 2 - 1] = { msg }
  #endif
#endif
#define NPF_STATIC_ASSERT(cond)    NPF_STATIC_ASSERT_MSG(cond, #cond)

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
  //static_assert(sizeof(double) <= sizeof(uint64_t)); // TODO: static_assert is not supported in old C standards; make a suitable NPF macro to work around that, using _Static_assert if possible

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

static void test_e(void);
static void test_f(void);
static void test_g(void);

int main(void) {
  // We only test against common float implementations: IEEE 754 f32 and f64
  NPF_STATIC_ASSERT(FLT_RADIX == 2);
  #define NPF_IMPLIES(a, b)    (!(a) || (b))

#if DBL_MANT_DIG == 53
  #define NPF_DBL_BITS    64
  #define FF(if_64, if_32)    if_64
#elif DBL_MANT_DIG == 23
  #define NPF_DBL_BITS    32
  #define FF(if_64, if_32)    if_32
#else
  #error Unsupported double size
#endif

  NPF_STATIC_ASSERT(NPF_IMPLIES(NPF_DBL_BITS == 64, sizeof(double) == 8));
  NPF_STATIC_ASSERT(NPF_IMPLIES(NPF_DBL_BITS == 64, DBL_MIN_EXP == -1021)); // minimum negative integer such that FLT_RADIX raised to one less than that power is a normalized floating-point number
  NPF_STATIC_ASSERT(NPF_IMPLIES(NPF_DBL_BITS == 64, DBL_MAX_EXP == 1024)); // maximum integer such that FLT_RADIX raised to one less than that power is a representable finite floating-point number
  NPF_STATIC_ASSERT(NPF_IMPLIES(NPF_DBL_BITS == 64, DBL_MIN == 0x1p-1022));
  NPF_STATIC_ASSERT(NPF_IMPLIES(NPF_DBL_BITS == 64, DBL_MAX == 0x1.FFFFFFFFFFFFFp+1023));
  NPF_STATIC_ASSERT(NPF_IMPLIES(NPF_DBL_BITS == 64, 0x1p-1074 > 0)); // has subnormals
  NPF_STATIC_ASSERT(NPF_IMPLIES(NPF_DBL_BITS == 64, 0x1p-1074 < DBL_MIN)); // has subnormals

  NPF_STATIC_ASSERT(NPF_IMPLIES(NPF_DBL_BITS == 32, sizeof(double) == 4));
  NPF_STATIC_ASSERT(NPF_IMPLIES(NPF_DBL_BITS == 32, DBL_MIN_EXP == -125)); // minimum negative integer such that FLT_RADIX raised to one less than that power is a normalized floating-point number
  NPF_STATIC_ASSERT(NPF_IMPLIES(NPF_DBL_BITS == 32, DBL_MAX_EXP == 128)); // maximum integer such that FLT_RADIX raised to one less than that power is a representable finite floating-point number
  NPF_STATIC_ASSERT(NPF_IMPLIES(NPF_DBL_BITS == 32, DBL_MIN == 0x1p-126));
  NPF_STATIC_ASSERT(NPF_IMPLIES(NPF_DBL_BITS == 32, DBL_MAX == 0x1.FFFFFFFEp+127));
  NPF_STATIC_ASSERT(NPF_IMPLIES(NPF_DBL_BITS == 32, 0x1p-149 > 0)); // has subnormals
  NPF_STATIC_ASSERT(NPF_IMPLIES(NPF_DBL_BITS == 32, 0x1p-149 < DBL_MIN)); // has subnormals

  NPF_STATIC_ASSERT(sizeof(double) == 8); // just because the tests aren't compatible yet; NPF already is

#if NANOPRINTF_USE_FLOAT_SCI_FORMAT_SPECIFIER == 1
  test_e();
#endif
#if NANOPRINTF_USE_FLOAT_DEC_FORMAT_SPECIFIER == 1
  test_f();
#endif
#if NANOPRINTF_USE_FLOAT_SHORTEST_FORMAT_SPECIFIER == 1
  test_g();
#endif
  printf("Done.");
  return 0;
}

void test_e(void)
{
  ////////////////

  // TODO:
  // All these tests are tailored towards IEEE 754 binary64.
  // Similar tests should also be done for IEEE 754 binary32 at least.
  // Also, they assume that the dynamic integer scaling uses u64, so that we have no loss of precision.
  // Other tests should be done, separately, for different integer scaling settings.

  // TODO: all these tests are done assuming that the options are the ones defined
  // at the top of the file. When integrating this in the existing test suites,
  // we should conditionally compile the various tests depending on the actual options,
  // both regarding the enabled flags, and regarding the expected output when it depends
  // on the scaling integer bitsize. This has already been taken care of, but probably only partially.

  { // misc tests
    PRC("1.3e+01", "%.1e", 12.5);
    PRC("1.4e+01", "%.1e", 13.5);
    PRC("1.5e+01", "%.1e", 14.5);
    PRC("1.6e+01", "%.1e", 15.5);

    PRC("1.235e+01", "%.3e", 12.345);
    PRC("1.000000750000000e-36", "%.15e", 1.00000075e-36);
    PRC("1.0000007500000000e-36", "%.16e", 1.00000075e-36);
    PRC("1.234444444444444e+30", "%.15e", 1.2344444444444445e30);
    PRC("1.2344444444444445e+30", "%.16e", 1.2344444444444445e30);

    PRC("1e+01", "%.0e", 9.87654321e0);
    PRC("9.9e+00", "%.1e", 9.87654321e0);
    PRC("9.88e+00", "%.2e", 9.87654321e0);
    PRC("9.877e+00", "%.3e", 9.87654321e0);
    PRC("9.8765e+00", "%.4e", 9.87654321e0);
    PRC("9.87654e+00", "%.5e", 9.87654321e0);
    PRC("9.876543e+00", "%.6e", 9.87654321e0);
    PRC("9.8765432e+00", "%.7e", 9.87654321e0);
    PRC("9.87654321e+00", "%.8e", 9.87654321e0);
    PRC("9.876543210e+00", "%.9e", 9.87654321e0);
    PRC("1e+03", "%.0e", 9.87654321e2);
    PRC("9.9e+02", "%.1e", 9.87654321e2);
    PRC("9.88e+02", "%.2e", 9.87654321e2);
    PRC("9.877e+02", "%.3e", 9.87654321e2);
    PRC("9.8765e+02", "%.4e", 9.87654321e2);
    PRC("9.87654e+02", "%.5e", 9.87654321e2);
    PRC("9.876543e+02", "%.6e", 9.87654321e2);
    PRC("9.8765432e+02", "%.7e", 9.87654321e2);
    PRC("9.87654321e+02", "%.8e", 9.87654321e2);
    PRC("9.876543210e+02", "%.9e", 9.87654321e2);
    PRC("1e+26", "%.0e", 9.87654321e25);
    PRC("9.9e+25", "%.1e", 9.87654321e25);
    PRC("9.88e+25", "%.2e", 9.87654321e25);
    PRC("9.877e+25", "%.3e", 9.87654321e25);
    PRC("9.8765e+25", "%.4e", 9.87654321e25);
    PRC("9.87654e+25", "%.5e", 9.87654321e25);
    PRC("9.876543e+25", "%.6e", 9.87654321e25);
    PRC("9.8765432e+25", "%.7e", 9.87654321e25);
    PRC("9.87654321e+25", "%.8e", 9.87654321e25);
    PRC("9.876543210e+25", "%.9e", 9.87654321e25);
    PRC("1e-03", "%.0e", 9.87654321e-4);
    PRC("9.9e-04", "%.1e", 9.87654321e-4);
    PRC("9.88e-04", "%.2e", 9.87654321e-4);
    PRC("9.877e-04", "%.3e", 9.87654321e-4);
    PRC("9.8765e-04", "%.4e", 9.87654321e-4);
    PRC("9.87654e-04", "%.5e", 9.87654321e-4);
    PRC("9.876543e-04", "%.6e", 9.87654321e-4);
    PRC("9.8765432e-04", "%.7e", 9.87654321e-4);
    PRC("9.87654321e-04", "%.8e", 9.87654321e-4);
    PRC("9.876543210e-04", "%.9e", 9.87654321e-4);
    PRC("1e-85", "%.0e", 9.87654321e-86);
    PRC("9.9e-86", "%.1e", 9.87654321e-86);
    PRC("9.88e-86", "%.2e", 9.87654321e-86);
    PRC("9.877e-86", "%.3e", 9.87654321e-86);
    PRC("9.8765e-86", "%.4e", 9.87654321e-86);
    PRC("9.87654e-86", "%.5e", 9.87654321e-86);
    PRC("9.876543e-86", "%.6e", 9.87654321e-86);
    PRC("9.8765432e-86", "%.7e", 9.87654321e-86);
    PRC("9.87654321e-86", "%.8e", 9.87654321e-86);
    PRC("9.876543210e-86", "%.9e", 9.87654321e-86);

    PRC("0.000000e+00", "%e", 0.0);
    PRC("1.234500e+01", "%e", 12.345);
    PRC("1.234500e+15", "%e", 1.2345e15);
    PRC("1.234500e-10", "%e", 1.2345e-10);
    PRC("-1.234500e+01", "%e", -12.345);
    PRC("-1.234500e+15", "%e", -1.2345e15);
    PRC("-1.234500e-10", "%e", -1.2345e-10);
    PRC("1.235e+01", "%.3e", 12.345);
    PRC("1.234500000e+01", "%.9e", 12.345);
    PRC("1.004500000e+02", "%.9e", 100.45);
    PRC("1.004500000e+01", "%.9e", 10.045);
    PRC("1.004500000e+00", "%.9e", 1.0045);

    PRC("1.234500000e-13", "%.9e", 1.2345e-13);
    PRC("1.234500000e-12", "%.9e", 1.2345e-12);
    PRC("1.234500000e-11", "%.9e", 1.2345e-11);
    PRC("1.234500000e-10", "%.9e", 1.2345e-10);
    PRC("1.234500000e-09", "%.9e", 1.2345e-9);
    PRC("1.234500000e-08", "%.9e", 1.2345e-8);
    PRC("1.234500000e-07", "%.9e", 1.2345e-7);
    PRC("1.234500000e-06", "%.9e", 1.2345e-6);
    PRC("1.234500000e-05", "%.9e", 1.2345e-5);
    PRC("1.234500000e-04", "%.9e", 1.2345e-4);
    PRC("1.234500000e-03", "%.9e", 1.2345e-3);
    PRC("1.234500000e-02", "%.9e", 1.2345e-2);
    PRC("1.234500000e-01", "%.9e", 1.2345e-1);
    PRC("1.234500000e+00", "%.9e", 1.2345e0);
    PRC("1.234500000e+01", "%.9e", 1.2345e+1);
    PRC("1.234500000e+02", "%.9e", 1.2345e+2);
    PRC("1.234500000e+03", "%.9e", 1.2345e+3);
    PRC("1.234500000e+04", "%.9e", 1.2345e+4);
    PRC("1.234500000e+05", "%.9e", 1.2345e+5);
    PRC("1.234500000e+06", "%.9e", 1.2345e+6);
    PRC("1.234500000e+07", "%.9e", 1.2345e+7);
    PRC("1.234500000e+08", "%.9e", 1.2345e+8);
    PRC("1.234500000e+09", "%.9e", 1.2345e+9);
    PRC("1.234500000e+10", "%.9e", 1.2345e+10);
    PRC("1.234500000e+11", "%.9e", 1.2345e+11);
    PRC("1.234500000e+12", "%.9e", 1.2345e+12);
    PRC("1.234500000e+13", "%.9e", 1.2345e+13);

    PRC("1.234490000e-13", "%.9e", 1.23449e-13);
    PRC("1.234490000e-12", "%.9e", 1.23449e-12);
    PRC("1.234490000e-11", "%.9e", 1.23449e-11);
    PRC("1.234490000e-10", "%.9e", 1.23449e-10);
    PRC("1.234490000e-09", "%.9e", 1.23449e-9);
    PRC("1.234490000e-08", "%.9e", 1.23449e-8);
    PRC("1.234490000e-07", "%.9e", 1.23449e-7);
    PRC("1.234490000e-06", "%.9e", 1.23449e-6);
    PRC("1.234490000e-05", "%.9e", 1.23449e-5);
    PRC("1.234490000e-04", "%.9e", 1.23449e-4);
    PRC("1.234490000e-03", "%.9e", 1.23449e-3);
    PRC("1.234490000e-02", "%.9e", 1.23449e-2);
    PRC("1.234490000e-01", "%.9e", 1.23449e-1);
    PRC("1.234490000e+00", "%.9e", 1.23449e0);
    PRC("1.234490000e+01", "%.9e", 1.23449e+1);
    PRC("1.234490000e+02", "%.9e", 1.23449e+2);
    PRC("1.234490000e+03", "%.9e", 1.23449e+3);
    PRC("1.234490000e+04", "%.9e", 1.23449e+4);
    PRC("1.234490000e+05", "%.9e", 1.23449e+5);
    PRC("1.234490000e+06", "%.9e", 1.23449e+6);
    PRC("1.234490000e+07", "%.9e", 1.23449e+7);
    PRC("1.234490000e+08", "%.9e", 1.23449e+8);
    PRC("1.234490000e+09", "%.9e", 1.23449e+9);
    PRC("1.234490000e+10", "%.9e", 1.23449e+10);
    PRC("1.234490000e+11", "%.9e", 1.23449e+11);
    PRC("1.234490000e+12", "%.9e", 1.23449e+12);
    PRC("1.234490000e+13", "%.9e", 1.23449e+13);

    PRC("5.678000000e-100", "%.9e", 5.678e-100);
    PRC("5.678000000e+100", "%.9e", 5.678e+100);
    PRC("1.000000000e-127", "%.9e", 1e-127);
    PRC("1.000000000e-128", "%.9e", 1e-128);
    PRC("1.000000000e-150", "%.9e", 1e-150);
    PRC("1.000000000e-199", "%.9e", 1e-199);
    PRC("1.000000000e-200", "%.9e", 1e-200);
    PRC("1.000000000e-255", "%.9e", 1e-255);
    PRC("1.000000000e-256", "%.9e", 1e-256);
    PRC("1.000000000e+127", "%.9e", 1e+127);
    PRC("1.000000000e+128", "%.9e", 1e+128);
    PRC("1.000000000e+150", "%.9e", 1e+150);
    PRC("1.000000000e+199", "%.9e", 1e+199);
    PRC("1.000000000e+200", "%.9e", 1e+200);
    PRC("1.000000000e+255", "%.9e", 1e+255);
    PRC("1.000000000e+256", "%.9e", 1e+256);
    PRC("9.875000000e-200", "%.9e", 9.875e-200);
    PRC("9.875000000e+200", "%.9e", 9.875e+200);

    PRC("1.000e+01", "%.3e", 9.999999999999999);
    PRC("9.999e+00", "%.3e", 9.9995);
    PRC("1.0000000000e+01", "%.10e", 9.999999999999999);
    PRC("1.00000000000000e+01", "%.14e", 9.999999999999999);
    PRC("3.444e+00", "%.3e", 3.4444444445);
    PRC("3.444444445e+00", "%.9e", 3.4444444445);
    PRC("3.4444444445e+00", "%.10e", 3.4444444445);
    PRC("3.444e+20", "%.3e", 3.4444444445e20);
    PRC("3.444444445e+20", "%.9e", 3.4444444445e20);
    PRC("3.4444444445e+20", "%.10e", 3.4444444445e20);
    PRC("3.444e-35", "%.3e", 3.4444444445e-35);
    PRC("3.444444444e-35", "%.9e", 3.4444444445e-35);
    PRC("3.4444444445e-35", "%.10e", 3.4444444445e-35);

    PRC("4.940656458412465e-324", "%.15e", +0x1p-1074); // smallest f64 subnormal, ~=4.9406564584124654e-324
    PRC("-4.940656458412465e-324", "%.15e", -0x1p-1074); // smallest f64 subnormal, ~=4.9406564584124654e-324
    PRC("2.225073858507201e-308", "%.15e", +0x0.fffffffffffffp-1022); // largest f64 subnormal, ~= 2.2250738585072009e-308
    PRC("-2.225073858507201e-308", "%.15e", -0x0.fffffffffffffp-1022); // largest f64 subnormal, ~= 2.2250738585072009e-308
    PRC("2.225073858507201e-308", "%.15e", +0x1p-1022); // smallest f64 normal, ~= 2.2250738585072014e-308
    PRC("-2.225073858507201e-308", "%.15e", -0x1p-1022); // smallest f64 normal, ~= 2.2250738585072014e-308
    PRC("9.999999999999999e-309", "%.15e", 1.e-308);
    PRC("1.000000000000000e+308", "%.15e", 1.e+308);
    PRC("1.797693134862316e+308", "%.15e", +0x1.fffffffffffffp1023); // largest f64 normal, ~1.7976931348623157e308

    PRC("4.9406564584124654e-324", "%.16e", +0x1p-1074);
    PRC("-4.9406564584124654e-324", "%.16e", -0x1p-1074);
    PRC("2.2250738585072009e-308", "%.16e", +0x0.fffffffffffffp-1022);
    PRC("-2.2250738585072009e-308", "%.16e", -0x0.fffffffffffffp-1022);
    PRC("2.2250738585072014e-308", "%.16e", +0x1p-1022);
    PRC("-2.2250738585072014e-308", "%.16e", -0x1p-1022);
    PR ("9.9999999999999990e-309", "%.16e", 1.e-308); // "9.9999999999999990e-309" is round-trip, though a more accurate string would be "9.9999999999999991e-309"
    PR ("9.9999999999999999e+307", "%.16e", 1.e+308); // "9.9999999999999999e+307" is round-trip, though a more accurate string would be "1.0000000000000000e+308"
    PRC("1.7976931348623157e+308", "%.16e", +0x1.fffffffffffffp1023);

    PRC("1.00000000000000002e-03", "%.17e", 1.e-3);
    PRC("1.00000000000000000e+03", "%.17e", 1.e+3);

    if (NANOPRINTF_CONVERSION_BUFFER_SIZE >= 24) {
      PRC("1.00000000000000008e-30", "%.17e", 1.e-30);
      PRC("1.00000000000000002e+30", "%.17e", 1.e+30);
      PRC("1.000000000000000021e-03", "%.18e", 1.e-3);
      PRC("1.000000000000000000e+03", "%.18e", 1.e+3);
    }
    if (NANOPRINTF_CONVERSION_BUFFER_SIZE >= 25) {
      PR ( "4.94065645841246539e-324", "%.17e", +0x1p-1074);               // round-trip, though accurate is "4.94065645841246544e-324"
      PR ("-4.94065645841246539e-324", "%.17e", -0x1p-1074);               // acc. "-4.94065645841246544e-324"
      PR ( "2.22507385850720086e-308", "%.17e", +0x0.fffffffffffffp-1022); // acc.  "2.22507385850720089e-308"
      PR ("-2.22507385850720086e-308", "%.17e", -0x0.fffffffffffffp-1022); // acc. "-2.22507385850720089e-308"
      PR ( "2.22507385850720136e-308", "%.17e", +0x1p-1022);               // acc.  "2.22507385850720138e-308"
      PR ("-2.22507385850720136e-308", "%.17e", -0x1p-1022);               // acc. "-2.22507385850720138e-308"
      PR ( "9.99999999999999897e-309", "%.17e", 1.e-308);                  // acc.  "9.99999999999999909e-309"
      PR ( "9.99999999999999989e+307", "%.17e", 1.e+308);                  // acc.  "1.00000000000000001e+308"
      PR ( "1.79769313486231567e+308", "%.17e", +0x1.fffffffffffffp1023);  // acc.  "1.79769313486231571e+308"
    }
    if (NANOPRINTF_CONVERSION_BUFFER_SIZE >= 26) {
      PR ( "4.940656458412465387e-324", "%.18e", +0x1p-1074);               // acc.  "4.940656458412465442e-324"
      PR ("-4.940656458412465387e-324", "%.18e", -0x1p-1074);               // acc. "-4.940656458412465442e-324"
      PR ( "2.225073858507200865e-308", "%.18e", +0x0.fffffffffffffp-1022); // acc.  "2.225073858507200889e-308"
      PR ("-2.225073858507200865e-308", "%.18e", -0x0.fffffffffffffp-1022); // acc. "-2.225073858507200889e-308"
      PR ( "2.225073858507201360e-308", "%.18e", +0x1p-1022);               // acc.  "2.225073858507201383e-308"
      PR ("-2.225073858507201360e-308", "%.18e", -0x1p-1022);               // acc. "-2.225073858507201383e-308"
      PR ( "9.999999999999998966e-309", "%.18e", 1.e-308);                  // acc.  "9.999999999999999093e-309"
      PR ( "9.999999999999999890e+307", "%.18e", 1.e+308);                  // acc.  "1.000000000000000011e+308"
      PR ( "1.797693134862315669e+308", "%.18e", +0x1.fffffffffffffp1023);  // acc.  "1.797693134862315708e+308"
    }

    PR ("nan", "%.9e", npf_nan(0, 1, 0));
    PR ("nan", "%.9e", npf_nan(0, 1, 1));
    PR ("nan", "%.9e", npf_nan(0, 0, 0));
    PR ("nan", "%.9e", npf_nan(0, 0, 1));
    PR ("-nan", "%.9e", npf_nan(1, 1, 0));
    PR ("-nan", "%.9e", npf_nan(1, 1, 1));
    PR ("-nan", "%.9e", npf_nan(1, 0, 0));
    PR ("-nan", "%.9e", npf_nan(1, 0, 1));
    PR ("inf", "%.9e", 1/0.0);
    PR ("-inf", "%.9e", -1/0.0);
    PR ("NAN", "%.9E", npf_nan(0, 1, 0));
    PR ("INF", "%.9E", 1/0.0);

    PRC("4.512941713298720e-307", "%.15e", 23723333333333333);
    PRC("4.5129417132987197e-307", "%.16e", 23723333333333333);

    PRC("1e+00", "%.0e", 1.0);
    PRC("1.0e+00", "%.1e", 1.0);
    PRC("1.000e+00", "%.3e", 1.0001);
    PRC("1.001e+00", "%.3e", 1.001);
#if NANOPRINTF_USE_ALT_FORM_FLAG == 1
    PRC("1.e+00", "%#.0e", 1.0);
    PRC("1.0e+00", "%#.1e", 1.0);
    PRC("1.010000e+00", "%#e", 1.01);
    PRC("1.000e+00", "%#.3e", 1.0001);
    PRC("1.001e+00", "%#.3e", 1.001);
#endif
  }

  { // basic tests
    PRC("0e+00"  , "%.0e", 0.0);
    PRC("0E+00"  , "%.0E", 0.0);
    PRC("-0e+00" , "%.0e", -0.0);
    PRC("-0E+00" , "%.0E", -0.0);

    PRC("0.0e+00" , "%.1e", 0.0);
    PRC("0.00e+00" , "%.2e", 0.0);
  }
  { // flags/fields combinations
#if NANOPRINTF_USE_ALT_FORM_FLAG == 1
    PRC("0.e+00"  , "%#.0e", 0.0);
    PRC("0.0e+00" , "%#.1e", 0.0);
    PRC("0.00e+00"   , "%#.2e", 0.0);
#endif

#if NANOPRINTF_USE_FIELD_WIDTH_FORMAT_SPECIFIERS == 1
    PRC("0.0e+00"  , "%0.1e", 0.0);
    PRC("0.000e+00           " , "%-20.3e", 0.0);
#endif

#if NANOPRINTF_USE_ALT_FORM_FLAG == 1
  #if NANOPRINTF_USE_FIELD_WIDTH_FORMAT_SPECIFIERS == 1
    PRC("000000000000.000e+00" , "%#020.3e", 0.0);
    PRC("0.000e+00           " , "%#-20.3e", 0.0);
    PRC("           0.000e+00" , "%#*.3e", 20, 0.0);
    PRC("0.000e+00           " , "%#*.3e", -20, 0.0);
  #endif
    PRC("+0.000e+00"           , "%#+.3e", 0.0);
    PRC("-0.000e+00"           , "%#+.3e", -0.0);
    PRC(" 0.000e+00"           , "%# .3e", 0.0);
#endif

#if NANOPRINTF_USE_FIELD_WIDTH_FORMAT_SPECIFIERS == 1
    PRC("000000000000.000e+00" , "%020.3e", 0.0);
    PRC("000000000000.000e+00" , "%0*.3e", 20, 0.0);
    PRC("0.000e+00           " , "%0*.3e", -20, 0.0);
    PRC("+00000000000.000e+00" , "%+020.3e", 0.0);
    PRC("-00000000000.000e+00" , "%+020.3e", -0.0);
    PRC(" 00000000000.000e+00" , "% 020.3e", 0.0);
    PRC("0.000e+00           " , "%-20.3e", 0.0);
    PRC("+0.000e+00          " , "%-+20.3e", 0.0);
    PRC("+0.000e+00          " , "%-+*.3e", 20, 0.0);
#endif
    PRC("+0.000e+00" , "%-+.3e", 0.0);
    PRC("-0.000e+00" , "%-+.3e", -0.0);
    PRC("0.000e+00" , "%-.3e", 0.0);

#if NANOPRINTF_USE_FIELD_WIDTH_FORMAT_SPECIFIERS == 1
    PRC("+00000000000.000e+00" , "%+020.3e", 0.0);
    PRC("          +0.000e+00" , "%+20.3e", 0.0);
    PRC("          +0.000e+00" , "%+*.3e", 20, 0.0);
    PRC("+0.000e+00          " , "%+*.3e", -20, 0.0);
#endif
    PRC("+0.000e+00" , "%+.3e", 0.0);
    PRC("-0.000e+00" , "%+.3e", -0.0);
    PRC("+0.000e+00" , "%+ .3e", 0.0);
  }

  { // nan/inf
    PRC("inf"        , "%e", +INFINITY);
    PRC("-inf"       , "%e", -INFINITY);
    PRC("nan"        , "%e", NAN); // should use npf_nan()

#if NANOPRINTF_USE_ALT_FORM_FLAG == 1
    PRC("inf"        , "%#e", +INFINITY);
    PRC("-inf"       , "%#e", -INFINITY);
    PRC("nan"        , "%#e", NAN); // should use npf_nan()

    PRC("INF"        , "%#E", +INFINITY);

    PR ("       inf" , "%#010e", +INFINITY); // NOTE: some implementations are buggy and print "0000000inf" in this and similar cases
#endif

    PRC("inf"        , "%.8e", +INFINITY);
#if NANOPRINTF_USE_FIELD_WIDTH_FORMAT_SPECIFIERS == 1
    PRC("       inf" , "%10.8e", +INFINITY);
    PR ("       inf" , "%010.8e", +INFINITY);
#endif
    PRC("inf"        , "%.0e", +INFINITY);
#if NANOPRINTF_USE_ALT_FORM_FLAG == 1
    PR ("inf"        , "%#.0e", +INFINITY); // NOTE: some implementations are buggy and print "i.nf" in this and similar cases
#endif
#if NANOPRINTF_USE_FIELD_WIDTH_FORMAT_SPECIFIERS == 1
    PRC("inf       " , "%-10.0e", +INFINITY);
#endif
    PRC("+inf"       , "%+.0e", +INFINITY);
    PRC(" inf"       , "% .0e", +INFINITY);
#if NANOPRINTF_USE_FIELD_WIDTH_FORMAT_SPECIFIERS == 1
    PRC("       inf" , "%10.0e", +INFINITY);
  #if NANOPRINTF_USE_ALT_FORM_FLAG == 1
    PR ("       inf" , "%#10.0e", +INFINITY);
  #endif
#endif
  }

  { // long double
    PR ("0.000000e+00" , "%Le", 0.0L);
    PR ("1.234000e+10" , "%Le", 1.234e10L);
    PR ("8.592486554000000e+299" , "%.15Le", 8.592486554e299L);
    PR ("-4.509274e-03" , "%Le", -4.509274e-3L);
    PR ("3.721269e-250" , "%Le", 3.721269e-250L);
    PR ("5e-21" , "%.0Le", 543.789e-23L);
    PR ("5.437890E-21" , "%LE", 543.789e-23L);
  }

  { // misc values
    PRC("2.230000e-308" , "%e", 2.23e-308),
    PRC("8.900295e-308" , "%e", 0x1p-1020);
    PRC("4.450148e-308" , "%e", 0x1p-1021);
    PRC("2.225074e-308" , "%e", 0x1p-1022);
    PRC("1.112537e-308" , "%e", 0x1p-1023);
    PRC("5.562685e-309" , "%e", 0x1p-1024);
    PRC("2.781342e-309" , "%e", 0x1p-1025);
    PRC("1.390671e-309" , "%e", 0x1p-1026);
    PRC("6.953356e-310" , "%e", 0x1p-1027);
    PRC("3.476678e-310" , "%e", 0x1p-1028);
    PRC("1.738339e-310" , "%e", 0x1p-1029);
    PRC("3.162020e-322" , "%e", 0x1p-1068);
    PRC("1.581010e-322" , "%e", 0x1p-1069);
    PRC("7.905050e-323" , "%e", 0x1p-1070);
    PRC("3.952525e-323" , "%e", 0x1p-1071);
    PRC("1.976263e-323" , "%e", 0x1p-1072);
    PRC("9.881313e-324" , "%e", 0x1p-1073);
    PRC("4.940656e-324" , "%e", 0x1p-1074);
    PRC("1.123558e+307" , "%e", 0x1p+1020);
    PRC("2.247116e+307" , "%e", 0x1p+1021);
    PRC("4.494233e+307" , "%e", 0x1p+1022);
    PRC("8.988466e+307" , "%e", 0x1p+1023);
    PRC("1.175494e-38" , "%e", 0x1p-126);
    PRC("5.877472e-39" , "%e", 0x1p-127);
    PRC("2.938736e-39" , "%e", 0x1p-128);
    PRC("1.469368e-39" , "%e", 0x1p-129);
    PRC("7.346840e-40" , "%e", 0x1p-130);
    PRC("1.401298e-45" , "%e", 0x1p-149);
    PRC("2.126765e+37" , "%e", 0x1p+124);
    PRC("4.253530e+37" , "%e", 0x1p+125);
    PRC("8.507059e+37" , "%e", 0x1p+126);
    PRC("1.701412e+38" , "%e", 0x1p+127);
    PRC("2.000000e+00" , "%e", 0x1p+1);
    PRC("4.000000e+00" , "%e", 0x1p+2);
    PRC("2.247116e+307" , "%e", 0x1.fffffffffffffp+1020);
    PRC("1.701412e+38" , "%e", 0x1.fffffffep+126);
    PRC("1.000000e+00" , "%e", 0x1p0);
    PRC("1.500000e+00" , "%e", 0x1.8p0);
    PRC("1.750000e+00" , "%e", 0x1.cp0);
    PRC("1.875000e+00" , "%e", 0x1.ep0);
    PRC("1.937500e+00" , "%e", 0x1.fp0);
    PRC("1.968750e+00" , "%e", 0x1.f8p0);
    PRC("1.996094e+00" , "%e", 0x1.ffp0);
    PRC("1.999756e+00" , "%e", 0x1.fffp0);
    PRC("1.999985e+00" , "%e", 0x1.ffffp0);
    PRC("1.999999e+00" , "%e", 0x1.fffffp0);
    PRC("2.000000e+00" , "%e", 0x1.ffffffp0);
    PRC("2.000000e+00" , "%e", 0x1.fffffffp0);
    PRC("2.000000e+00" , "%e", 0x1.fffffffep0);
    PRC("2.000000e+00" , "%e", 0x1.fffffffefp0);
    PRC("2.000000e+00" , "%e", 0x1.fffffffeffp0);
    PRC("2.000000e+00" , "%e", 0x1.fffffffefffp0);
    PRC("2.000000e+00" , "%e", 0x1.fffffffeffffp0);
    PRC("2.000000e+00" , "%e", 0x1.fffffffefffffp0);
    PRC("1.693850e+38" , "%e", 0xF.EDCBA97054328p+123);
    PRC("2.891098e-14" , "%e", 0x1.04682F746CA3Dp-45);
    PRC("-1.137778e+00" , "%e", -0x1.23456788ABCDEp0);
    PRC("-4.056451e+31" , "%e", -0x1.ffffp104);
    PRC("-5.647985e+219" , "%e", -0x1.ffffp729);
    PRC("-3.134504e-301" , "%e", -0x1.ADE83p-999);
    PRC("-5.053532e-36" , "%e", -0x1.ADE83p-118);
  }

  { // case
    PRC("2.230000E-308" , "%E", 2.23e-308),
    PRC("-5.831600E+40" , "%E", -5.8316e40);
  }

#if NANOPRINTF_USE_ALT_FORM_FLAG == 1
  { // hash
    PRC("0.000000e+00" , "%#e", 0.0);
    PRC("1.000000e+00" , "%#e", 1.0);
  #if NANOPRINTF_USE_FIELD_WIDTH_FORMAT_SPECIFIERS == 1
    PRC("000000004.382000e+03" , "%#020e", 4.382e3);
    PRC("4.382000e+03        " , "%#-20e", 4.382e3);
    PRC("        4.382000e+03" , "%# 20e", 4.382e3);
    PRC("       +4.382000e+03" , "%#+20e", 4.382e3);
    PRC("             +4.e+03" , "%#+20.0e", 4.382e3);
    PRC("            +4.4e+03" , "%#+20.1e", 4.382e3);
    PRC("           +4.38e+03" , "%#+20.2e", 4.382e3);
    PRC("000000000004.382e+03" , "%#020.3e", 4.382e3);
    PRC("    4.38200e+03" , "%#*.*e", 15, 5, 4.382e3);
    PRC("4.3820000e+03       " , "%#*.*e", -20, 7, 4.382e3);
    PRC("4.382000e+03        " , "%#*.*e", -20, -7, 4.382e3);
  #endif

    PRC("-0.000000e+00" , "%#e", -0.0);
    PRC("-1.000000e+00" , "%#e", -1.0);
  #if NANOPRINTF_USE_FIELD_WIDTH_FORMAT_SPECIFIERS == 1
    PRC("-00000004.382000e+03" , "%#020e", -4.382e3);
    PRC("-4.382000e+03       " , "%#-20e", -4.382e3);
    PRC("       -4.382000e+03" , "%# 20e", -4.382e3);
    PRC("       -4.382000e+03" , "%#+20e", -4.382e3);
    PRC("-00000004.382000e+03" , "%#020e", -4.382e3);
    PRC("-4.382000e+03       " , "%#-20e", -4.382e3);
    PRC("       -4.382000e+03" , "%# 20e", -4.382e3);
    PRC("       -4.382000e+03" , "%#+20e", -4.382e3);
    PRC("             -4.e+03" , "%#+20.0e", -4.382e3);
    PRC("            -4.4e+03" , "%#+20.1e", -4.382e3);
    PRC("           -4.38e+03" , "%#+20.2e", -4.382e3);
    PRC("-00000000004.382e+03" , "%#020.3e", -4.382e3);
    PRC("   -4.38200e+03" , "%#*.*e", 15, 5, -4.382e3);
    PRC("-4.3820000e+03      " , "%#*.*e", -20, 7, -4.382e3);
    PRC("-4.382000e+03       " , "%#*.*e", -20, -7, -4.382e3);
  #endif
  }
#endif

  { // space
    PRC(" 0.000000e+00" , "% e", 0.0);
    PRC(" 1.000000e+00" , "% e", 1.0);
#if NANOPRINTF_USE_FIELD_WIDTH_FORMAT_SPECIFIERS == 1
    PRC("        0.000000e+00" , "% 20e", 0.0);
    PRC("        1.000000e+00" , "% 20e", 1.0);
    PRC(" 00000005.499210e-02" , "% 020e", 5.49921e-2);
    PRC(" 5.499210e-02       " , "% -20e", 5.49921e-2);
  #if NANOPRINTF_USE_ALT_FORM_FLAG == 1
    PRC("        5.499210e-02" , "%# 20e", 5.49921e-2);
  #endif
    PRC("       +5.499210e-02" , "% +20e", 5.49921e-2);
    PRC("              +5e-02" , "% +20.0e", 5.49921e-2);
    PRC("            +5.5e-02" , "% +20.1e", 5.49921e-2);
    PRC("           +5.50e-02" , "% +20.2e", 5.49921e-2);
    PRC(" 00000000005.499e-02" , "% 020.3e", 5.49921e-2);
    PRC("    5.49921e-02" , "% *.*e", 15, 5, 5.49921e-2);
    PRC(" 5.4992100e-02      " , "% *.*e", -20, 7, 5.49921e-2);
    PRC(" 5.499210e-02       " , "% *.*e", -20, -7, 5.49921e-2);
#endif

    PRC("-0.000000e+00" , "% e", -0.0);
    PRC("-1.000000e+00" , "% e", -1.0);
#if NANOPRINTF_USE_FIELD_WIDTH_FORMAT_SPECIFIERS == 1
    PRC("       -0.000000e+00" , "% 20e", -0.0);
    PRC("       -1.000000e+00" , "% 20e", -1.0);
    PRC("-00000005.499210e-02" , "% 020e", -5.49921e-2);
    PRC("-5.499210e-02       " , "% -20e", -5.49921e-2);
  #if NANOPRINTF_USE_ALT_FORM_FLAG == 1
    PRC("       -5.499210e-02" , "%# 20e", -5.49921e-2);
  #endif
    PRC("       -5.499210e-02" , "% +20e", -5.49921e-2);
    PRC("              -5e-02" , "% +20.0e", -5.49921e-2);
    PRC("            -5.5e-02" , "% +20.1e", -5.49921e-2);
    PRC("           -5.50e-02" , "% +20.2e", -5.49921e-2);
    PRC("-00000000005.499e-02" , "% 020.3e", -5.49921e-2);
    PRC("   -5.49921e-02" , "% *.*e", 15, 5, -5.49921e-2);
    PRC("-5.4992100e-02      " , "% *.*e", -20, 7, -5.49921e-2);
    PRC("-5.499210e-02       " , "% *.*e", -20, -7, -5.49921e-2);
#endif
  }

  { // plus
    PRC("+0.000000e+00" , "%+e", 0.0);
    PRC("+1.000000e+00" , "%+e", 1.0);
#if NANOPRINTF_USE_FIELD_WIDTH_FORMAT_SPECIFIERS == 1
    PRC("+00000006.392740e+15" , "%+020e", 6.39274e15);
    PRC("+6.392740e+15       " , "%+-20e", 6.39274e15);
    PRC("       +6.392740e+15" , "%+ 20e", 6.39274e15);
  #if NANOPRINTF_USE_ALT_FORM_FLAG == 1
    PRC("       +6.392740e+15" , "%#+20e", 6.39274e15);
    PRC("             +6.e+15" , "%#+20.0e", 6.39274e15);
    PRC("            +6.4e+15" , "%#+20.1e", 6.39274e15);
    PRC("           +6.39e+15" , "%#+20.2e", 6.39274e15);
  #endif
    PRC("+00000000006.393e+15" , "%+020.3e", 6.39274e15);
    PRC("   +6.39274e+15" , "%+*.*e", 15, 5, 6.39274e15);
    PRC("+6.3927400e+15      " , "%+*.*e", -20, 7, 6.39274e15);
    PRC("+6.392740e+15       " , "%+*.*e", -20, -7, 6.39274e15);
#endif
    PRC("-0.000000e+00" , "%+e", -0.0);
    PRC("-1.000000e+00" , "%+e", -1.0);
#if NANOPRINTF_USE_FIELD_WIDTH_FORMAT_SPECIFIERS == 1
    PRC("-00000006.392740e+15" , "%+020e", -6.39274e15);
    PRC("-6.392740e+15       " , "%+-20e", -6.39274e15);
    PRC("       -6.392740e+15" , "%+ 20e", -6.39274e15);
  #if NANOPRINTF_USE_ALT_FORM_FLAG == 1
    PRC("       -6.392740e+15" , "%#+20e", -6.39274e15);
    PRC("             -6.e+15" , "%#+20.0e", -6.39274e15);
    PRC("            -6.4e+15" , "%#+20.1e", -6.39274e15);
    PRC("           -6.39e+15" , "%#+20.2e", -6.39274e15);
  #endif
    PRC("-00000000006.393e+15" , "%+020.3e", -6.39274e15);
    PRC("   -6.39274e+15" , "%+*.*e", 15, 5, -6.39274e15);
    PRC("-6.3927400e+15      " , "%+*.*e", -20, 7, -6.39274e15);
    PRC("-6.392740e+15       " , "%+*.*e", -20, -7, -6.39274e15);
#endif
  }

  { // minus
#if NANOPRINTF_USE_FIELD_WIDTH_FORMAT_SPECIFIERS == 1
    PRC("0.000000e+00                  " , "%-30e", 0.0);
    PRC("1.000000e+00                  " , "%-30e", 1.0);
    PRC("2.983500e-27        " , "%-020e", 2.9835e-27);
    PRC("+2.983500e-27       " , "%+-20e", 2.9835e-27);
    PRC(" 2.983500e-27       " , "%- 20e", 2.9835e-27);
  #if NANOPRINTF_USE_ALT_FORM_FLAG == 1
    PRC("2.983500e-27        " , "%#-20e", 2.9835e-27);
    PRC("3.e-27              " , "%#-20.0e", 2.9835e-27);
    PRC("3.0e-27             " , "%#-20.1e", 2.9835e-27);
    PRC("2.98e-27            " , "%#-20.2e", 2.9835e-27);
  #endif
    PRC("2.983e-27           " , "%-020.3e", 2.9835e-27);
    PRC("2.98350e-27    " , "%-*.*e", 15, 5, 2.9835e-27);
    PRC("2.9835000e-27       " , "%-*.*e", -20, 7, 2.9835e-27);
    PRC("2.983500e-27        " , "%-*.*e", -20, -7, 2.9835e-27);

    PRC("-0.000000e+00                 " , "%-30e", -0.0);
    PRC("-1.000000e+00                 " , "%-30e", -1.0);
    PRC("-2.983500e-27       " , "%-020e", -2.9835e-27);
    PRC("-2.983500e-27       " , "%+-20e", -2.9835e-27);
    PRC("-2.983500e-27       " , "%- 20e", -2.9835e-27);
  #if NANOPRINTF_USE_ALT_FORM_FLAG == 1
    PRC("-2.983500e-27       " , "%#-20e", -2.9835e-27);
    PRC("-3.e-27             " , "%#-20.0e", -2.9835e-27);
    PRC("-3.0e-27            " , "%#-20.1e", -2.9835e-27);
    PRC("-2.98e-27           " , "%#-20.2e", -2.9835e-27);
  #endif
    PRC("-2.983e-27          " , "%-020.3e", -2.9835e-27);
    PRC("-2.98350e-27   " , "%-*.*e", 15, 5, -2.9835e-27);
    PRC("-2.9835000e-27      " , "%-*.*e", -20, 7, -2.9835e-27);
    PRC("-2.983500e-27       " , "%-*.*e", -20, -7, -2.9835e-27);
#endif
  }

  { // zero
#if NANOPRINTF_USE_FIELD_WIDTH_FORMAT_SPECIFIERS == 1
    PRC("00000000000.000000e+00" , "%022e", 0.0);
    PRC("00000000001.000000e+00" , "%022e", 1.0);
    PRC("00000000003.347600e-01" , "%022e", 3.3476e-1);
    PRC("3.347600e-01          " , "%0-22e", 3.3476e-1);
    PRC(" 0000000003.347600e-01" , "%0 22e", 3.3476e-1);
  #if NANOPRINTF_USE_ALT_FORM_FLAG == 1
    PRC("00000000003.347600e-01" , "%#022e", 3.3476e-1);
    PRC("00000000000000003.e-01" , "%#022.0e", 3.3476e-1);
    PRC("0000000000000003.3e-01" , "%#022.1e", 3.3476e-1);
    PRC("000000000000003.35e-01" , "%#022.2e", 3.3476e-1);
  #endif
    PRC("00000000000003.348e-01" , "%022.3e", 3.3476e-1);
    PRC("000000000000003.34760e-01" , "%0*.*e", 25, 5, 3.3476e-1);
    PRC("3.3476000e-01                 " , "%0*.*e", -30, 7, 3.3476e-1);
    PRC("3.347600e-01                  " , "%0*.*e", -30, -7, 3.3476e-1);

    PRC("-0000000000.000000e+00" , "%022e", -0.0);
    PRC("-0000000001.000000e+00" , "%022e", -1.0);
    PRC("-0000000003.347600e-01" , "%022e", -3.3476e-1);
    PRC("-3.347600e-01         " , "%0-22e", -3.3476e-1);
    PRC("-0000000003.347600e-01" , "%0 22e", -3.3476e-1);
  #if NANOPRINTF_USE_ALT_FORM_FLAG == 1
    PRC("-0000000003.347600e-01" , "%#022e", -3.3476e-1);
    PRC("-0000000000000003.e-01" , "%#022.0e", -3.3476e-1);
    PRC("-000000000000003.3e-01" , "%#022.1e", -3.3476e-1);
    PRC("-00000000000003.35e-01" , "%#022.2e", -3.3476e-1);
  #endif
    PRC("-0000000000003.348e-01" , "%022.3e", -3.3476e-1);
    PRC("-00000000000003.34760e-01" , "%0*.*e", 25, 5, -3.3476e-1);
    PRC("-3.3476000e-01                " , "%0*.*e", -30, 7, -3.3476e-1);
    PRC("-3.347600e-01                 " , "%0*.*e", -30, -7, -3.3476e-1);
#endif
  }

#if NANOPRINTF_USE_FIELD_WIDTH_FORMAT_SPECIFIERS == 1
  { // width
    PRC("0.00000e+00" , "%8.5e", 0.0);
    PRC("    0.00000e+00" , "%15.5e", 0.0);
    PRC("1.00000e+00" , "%8.5e", 1.0);
    PRC("    1.00000e+00" , "%15.5e", 1.0);
    PRC("6.00000e-01" , "%-8.5e", 0.6);
    PRC("6.00000e-01    " , "%-15.5e", 0.6);
    PRC("0.00000e+00" , "%*.5e", 8, 0.0);
    PRC("    0.00000e+00" , "%*.5e", 15, 0.0);
    PRC("1.00000e+00" , "%*.5e", 8, 1.0);
    PRC("    1.00000e+00" , "%*.5e", 15, 1.0);
    PRC("6.00000e-01" , "%*.5e", -8, 0.6);
    PRC("6.00000e-01    " , "%*.5e", -15, 0.6);

    PRC("-0.00000e+00" , "%8.5e", -0.0);
    PRC("   -0.00000e+00" , "%15.5e", -0.0);
    PRC("-1.00000e+00" , "%8.5e", -1.0);
    PRC("   -1.00000e+00" , "%15.5e", -1.0);
    PRC("-6.00000e-01" , "%-8.5e", -0.6);
    PRC("-6.00000e-01   " , "%-15.5e", -0.6);
    PRC("-0.00000e+00" , "%*.5e", 8, -0.0);
    PRC("   -0.00000e+00" , "%*.5e", 15, -0.0);
    PRC("-1.00000e+00" , "%*.5e", 8, -1.0);
    PRC("   -1.00000e+00" , "%*.5e", 15, -1.0);
    PRC("-6.00000e-01" , "%*.5e", -8, -0.6);
    PRC("-6.00000e-01   " , "%*.5e", -15, -0.6);
  }
#endif

  { // precision
    PRC("0.00000e+00" , "%.5e", 0.0);
    PRC("0.00000000e+00" , "%.8e", 0.0);
    PRC("1.00000e+00" , "%.5e", 1.0);
    PRC("1.00000000e+00" , "%.8e", 1.0);
    PRC("2.00000e-01" , "%.5e", 0.2);
    PRC("2.00000000e-01" , "%.8e", 0.2);
    PRC("0.00000000e+00" , "%.*e", 8, 0.0);
    PRC("0.000000000000000e+00" , "%.*e", 15, 0.0);
    PRC("1.00000000e+00" , "%.*e", 8, 1.0);
    PRC("1.000000000000000e+00" , "%.*e", 15, 1.0);
    PRC("2.000000e-01" , "%.*e", -8, 0.2);
    PRC("2.000000e-01" , "%.*e", -15, 0.2);

    PRC("-0.00000e+00" , "%.5e", -0.0);
    PRC("-0.00000000e+00" , "%.8e", -0.0);
    PRC("-1.00000e+00" , "%.5e", -1.0);
    PRC("-1.00000000e+00" , "%.8e", -1.0);
    PRC("-2.00000e-01" , "%.5e", -0.2);
    PRC("-2.00000000e-01" , "%.8e", -0.2);
    PRC("-0.00000000e+00" , "%.*e", 8, -0.0);
    PRC("-0.000000000000000e+00" , "%.*e", 15, -0.0);
    PRC("-1.00000000e+00" , "%.*e", 8, -1.0);
    PRC("-1.000000000000000e+00" , "%.*e", 15, -1.0);
    PRC("-2.000000e-01" , "%.*e", -8, -0.2);
    PRC("-2.000000e-01" , "%.*e", -15, -0.2);
  }

  { // exponent
    PRC("1.000000e-100" , "%e", 1e-100);
    PRC("1.000000e-99"  , "%e", 1e-99);
    PRC("1.000000e-10"  , "%e", 1e-10);
    PRC("1.000000e-09"  , "%e", 1e-9);
    PRC("1.000000e-01"  , "%e", 1e-1);
    PRC("1.000000e+00"  , "%e", 1e0);
    PRC("1.000000e+01"  , "%e", 1e+1);
    PRC("1.000000e+09"  , "%e", 1e+9);
    PRC("1.000000e+10"  , "%e", 1e+10);
    PRC("1.000000e+99"  , "%e", 1e+99);
    PRC("1.000000e+100" , "%e", 1e+100);
  }
  { // rounding
    PRC("1.00000e-01" , "%.5e" , 0.1);
    PRC("1.0000000000e-01" , "%.10e", 0.1);
    PRC("1.000000000000000e-01" , "%.15e", 0.1);
    PRC("1.0000000000000001e-01" , "%.16e", 0.1);
    PRC("1.00000000000000006e-01" , "%.17e", 0.1);
    PRC("-2.00000e-01" , "%.5e" , -0.2);
    PRC("-2.0000000000e-01" , "%.10e", -0.2);
    PRC("-2.000000000000000e-01" , "%.15e", -0.2);
    PRC("-2.0000000000000001e-01" , "%.16e", -0.2);
    PRC("-2.00000000000000011e-01" , "%.17e", -0.2);
    PRC("6.00000e-01" , "%.5e" , 0.6);
    PRC("6.0000000000e-01" , "%.10e", 0.6);
    PRC("6.000000000000000e-01" , "%.15e", 0.6);
    PRC("5.9999999999999998e-01" , "%.16e", 0.6);
    PRC("5.99999999999999978e-01" , "%.17e", 0.6);

    PRC("5e+00" , "%.0e", 4.99999999);
    PRC("3e+00" , "%.0e", 2.55555555);
    PRC("7e+00" , "%.0e", 7.44444444);
    PRC("5.0e+00" , "%.1e", 4.99999999);
    PRC("2.6e+00" , "%.1e", 2.55555555);
    PRC("7.4e+00" , "%.1e", 7.44444444);
    PRC("5.00e+00" , "%.2e", 4.99999999);
    PRC("2.56e+00" , "%.2e", 2.55555555);
    PRC("7.44e+00" , "%.2e", 7.44444444);
    PRC("5.000e+00" , "%.3e", 4.99999999);
    PRC("2.556e+00" , "%.3e", 2.55555555);
    PRC("7.444e+00" , "%.3e", 7.44444444);
    PRC("5.0000000e+00" , "%.7e", 4.99999999);
    PRC("2.5555555e+00" , "%.7e", 2.55555555);
    PRC("7.4444444e+00" , "%.7e", 7.44444444);
    PRC("4.99999999e+00" , "%.8e", 4.99999999);
    PRC("2.55555555e+00" , "%.8e", 2.55555555);
    PRC("7.44444444e+00" , "%.8e", 7.44444444);
    PRC("4.999999990e+00" , "%.9e", 4.99999999);
    PRC("2.555555550e+00" , "%.9e", 2.55555555);
    PRC("7.444444440e+00" , "%.9e", 7.44444444);
    PRC("4.999999990000000e+00" , "%.15e", 4.99999999);
    PRC("2.555555550000000e+00" , "%.15e", 2.55555555);
    PRC("7.444444440000000e+00" , "%.15e", 7.44444444);
    PRC("4.9999999900000001e+00" , "%.16e", 4.99999999);
    PRC("2.5555555499999998e+00" , "%.16e", 2.55555555);
    PRC("7.4444444399999998e+00" , "%.16e", 7.44444444);

    PRC("1e-01" , "%.0e", 0.123456789);
    PRC("1.2e-01" , "%.1e", 0.123456789);
    PRC("1.23e-01" , "%.2e", 0.123456789);
    PRC("1.235e-01" , "%.3e", 0.123456789);
    PRC("1.2346e-01" , "%.4e", 0.123456789);
    PRC("1.23457e-01" , "%.5e", 0.123456789);
    PRC("1.234568e-01" , "%.6e", 0.123456789);
    PRC("1.2345679e-01" , "%.7e", 0.123456789);
    PRC("1.23456789e-01" , "%.8e", 0.123456789);
    PRC("1.234567890e-01" , "%.9e", 0.123456789);
  }

  { // numbers requiring the max number of digits for round-trip
    PRC("1.000000000000000e-32", "%.15e", 0x1.9f623d5a8a733p-107);
    PRC("1.000000000000000e-32", "%.15e", 0x1.9f623d5a8a734p-107);
    PRC("1.000000000000000e-32", "%.15e", 0x1.9f623d5a8a735p-107);
    PRC("1.000000000000000e-32", "%.15e", 0x1.9f623d5a8a736p-107);

    PRC("4.734663399999998e-24", "%.15e", 0x1.6e53ab9a828c5p-78);
    PRC("4.734663399999998e-24", "%.15e", 0x1.6e53ab9a828c6p-78);

    PRC("1.783677474777479e-01", "%.15e", 0x1.6d4c11d09ffa0p-3);
    PRC("1.783677474777479e-01", "%.15e", 0x1.6d4c11d09ffa1p-3);
    PRC("1.783677474777479e-01", "%.15e", 0x1.6d4c11d09ffa2p-3);
    PRC("1.783677474777479e-01", "%.15e", 0x1.6d4c11d09ffa3p-3);

   PRC("1.000000000000000e+00", "%.15e", 0x1.0000000000000p+0);
    PRC("1.000000000000000e+00", "%.15e", 0x1.0000000000001p+0);
    PRC("1.000000000000000e+00", "%.15e", 0x1.0000000000002p+0);

    PRC("3.089849261685456e+00", "%.15e", 0x1.8b802e3c411fap+1);
    PRC("3.089849261685456e+00", "%.15e", 0x1.8b802e3c411fbp+1);
    PRC("3.089849261685456e+00", "%.15e", 0x1.8b802e3c411fcp+1);

    PRC("1.000000000000000e+01", "%.15e", 0x1.4000000000000p+3);
    PRC("1.000000000000000e+01", "%.15e", 0x1.4000000000001p+3);
    PRC("1.000000000000000e+01", "%.15e", 0x1.4000000000002p+3);

    PRC("5.038814306823722e+07", "%.15e", 0x1.806e5788bbff4p+25);
    PRC("5.038814306823722e+07", "%.15e", 0x1.806e5788bbff5p+25);

    PRC("1.797693134862316e+308", "%.15e", 0x1.ffffffffffffep+1023); // this fails: we get "1.7976931348623154700" (rounded to "1.797693134862315") instead of the correct "17976931348623155085..." (rounded to "17976931348623154700"); it seems we lose more bits than expected in the scaling algorithm
    PRC("1.797693134862316e+308", "%.15e", 0x1.fffffffffffffp+1023);

    ////

    PRC("1.0000000000000001e-32", "%.16e", 0x1.9f623d5a8a733p-107);
    PRC("1.0000000000000002e-32", "%.16e", 0x1.9f623d5a8a734p-107);
    PRC("1.0000000000000003e-32", "%.16e", 0x1.9f623d5a8a735p-107);
    PRC("1.0000000000000005e-32", "%.16e", 0x1.9f623d5a8a736p-107);

    PRC("4.7346633999999977e-24", "%.16e", 0x1.6e53ab9a828c5p-78);
    PRC("4.7346633999999984e-24", "%.16e", 0x1.6e53ab9a828c6p-78);

    PRC("1.7836774747774786e-01", "%.16e", 0x1.6d4c11d09ffa0p-3);
    PRC("1.7836774747774789e-01", "%.16e", 0x1.6d4c11d09ffa1p-3);
    PRC("1.7836774747774792e-01", "%.16e", 0x1.6d4c11d09ffa2p-3);
    PRC("1.7836774747774795e-01", "%.16e", 0x1.6d4c11d09ffa3p-3);

    PRC("1.0000000000000000e+00", "%.16e", 0x1.0000000000000p+0);
    PRC("1.0000000000000002e+00", "%.16e", 0x1.0000000000001p+0);
    PRC("1.0000000000000004e+00", "%.16e", 0x1.0000000000002p+0);

    PRC("3.0898492616854556e+00", "%.16e", 0x1.8b802e3c411fap+1);
    PRC("3.0898492616854560e+00", "%.16e", 0x1.8b802e3c411fbp+1);
    PRC("3.0898492616854565e+00", "%.16e", 0x1.8b802e3c411fcp+1);

    PRC("1.0000000000000000e+01", "%.16e", 0x1.4000000000000p+3);
    PRC("1.0000000000000002e+01", "%.16e", 0x1.4000000000001p+3);
    PRC("1.0000000000000004e+01", "%.16e", 0x1.4000000000002p+3);

    PRC("5.0388143068237215e+07", "%.16e", 0x1.806e5788bbff4p+25);
    PRC("5.0388143068237223e+07", "%.16e", 0x1.806e5788bbff5p+25);

    PRC("1.7976931348623155e+308", "%.16e", 0x1.ffffffffffffep+1023);
    PRC("1.7976931348623157e+308", "%.16e", 0x1.fffffffffffffp+1023);

    ////
    // For f32
    PRC("1.0000008e-36", "%.7e", 0x1.54485ap-120);
    PRC("1.0000008e-36", "%.7e", 0x1.54485cp-120);

    PRC("1.4892376e-30", "%.7e", 0x1.e34904p-100);
    PRC("1.4892376e-30", "%.7e", 0x1.e34906p-100);

    PRC("1.0000002e-01", "%.7e", 0x1.99999ep-4);
    PRC("1.0000002e-01", "%.7e", 0x1.9999a0p-4);

    PRC("1.0000010e+01", "%.7e", 0x1.400014p+3);
    PRC("1.0000010e+01", "%.7e", 0x1.400016p+3);

    PRC("1.0000091e+37", "%.7e", 0x1.e17ca2p+122);
    PRC("1.0000091e+37", "%.7e", 0x1.e17ca4p+122);

    ////
    PRC("1.00000075e-36", "%.8e", 0x1.54485ap-120);
    PRC("1.00000084e-36", "%.8e", 0x1.54485cp-120);

    PRC("1.48923755e-30", "%.8e", 0x1.e34904p-100);
    PRC("1.48923765e-30", "%.8e", 0x1.e34906p-100);

    PRC("1.00000016e-01", "%.8e", 0x1.99999ep-4);
    PRC("1.00000024e-01", "%.8e", 0x1.9999a0p-4);

    PRC("1.00000095e+01", "%.8e", 0x1.400014p+3);
    PRC("1.00000105e+01", "%.8e", 0x1.400016p+3);

    PRC("1.00000906e+37", "%.8e", 0x1.e17ca2p+122);
    PRC("1.00000912e+37", "%.8e", 0x1.e17ca4p+122);
  }

  { // subnormals
    PRC("9.431041e-317" , "%e", 0x1.2345p-1050);
    PRC("9.4310e-317" , "%.4e", 0x1.2345p-1050);
    PRC("9.43104125e-317" , "%.8e", 0x1.2345p-1050);
    PRC("9.4310412498311e-317" , "%.13e", 0x1.2345p-1050);
    PRC("4.940656e-324" , "%e",  0x1p-1074);
    PRC("5e-324" , "%.0e", 0x1p-1074);
    PRC("4.940656458412e-324" , "%.12e", 0x1p-1074);
    PRC("4.9406564584125e-324" , "%.13e", 0x1p-1074);

    PRC("2.612210e-41" , "%e", 0x1.23456789p-135);
    PRC("2.61e-41" , "%.2e", 0x1.2345p-135);
    PRC("2.6122e-41" , "%.4e", 0x1.2345p-135);
    PRC("2.61219550e-41" , "%.8e", 0x1.2345p-135);
    PRC("1.401298e-45" , "%e", 0x1p-149);
    PRC("1e-45" , "%.0e", 0x1p-149);
    PRC("1.4012985e-45" , "%.7e", 0x1p-149);
    PRC("1.40129846e-45" , "%.8e", 0x1p-149);

    PRC("1.112537e-308" , "%e", 0x1p-1023);
    PRC("1e-308" , "%.0e", 0x0.8p-1022);
    PRC("6e-309" , "%.0e", 0x0.4p-1022);
#if NANOPRINTF_USE_ALT_FORM_FLAG == 1
    PRC("1.e-308" , "%#.0e", 0x0.8p-1022);
    PRC("6.e-309" , "%#.0e", 0x0.4p-1022);
#endif
    PRC("1.1e-308" , "%.1e", 0x0.8p-1022);
    PRC("5.6e-309" , "%.1e", 0x0.4p-1022);
    PRC("7.0e-310" , "%.1e", 0x0.08p-1022);
    PRC("3.5e-310" , "%.1e", 0x0.04p-1022);
  }

  {
    // Trailing '0's and dp
    PRC("0.000000e+00", "%e", 0.0);
    PR ("0.000000e+00", "%#e", 0.0);
    PRC("0.0e+00", "%.1e", 0.0);
    PR ("0.0e+00", "%#.1e", 0.0);
    PRC("1.000000e+00", "%e", 1.0);
    PRC("1.000000e+00", "%#e", 1.0);
    PRC("1.0e+00", "%.1e", 1.0);
    PRC("1.0e+00", "%#.1e", 1.0);
    PRC("1.000000e+00", "%#e", 1.0);
    PRC("1.234000e-08", "%e", 1.234e-8);
    PRC("1.234000e-08", "%#e", 1.234e-8);
    PRC("1.234000e+08", "%e", 1.234e+8);
    PRC("1.234000e+08", "%#e", 1.234e+8);

    PRC("1.000004e+01", "%.6e", 10.00004);
    PRC("1.00000e+01", "%.5e", 10.00004);
    PRC("1.0e+01", "%.1e", 10.00004);
    PRC("1e+01", "%.0e", 10.00004);
    PRC("1.00e+05", "%.2e", 100040.0);
    PRC("1.00e+05", "%#.2e", 100040.0);
    PRC("1.00e+05", "%.2e", 100042.3);
    PRC("1.00e+05", "%#.2e", 100042.3);
    PRC("1.00e-02", "%.2e", 0.01000423);
    PRC("1.00e-02", "%#.2e", 0.01000423);
    PRC("1.00e-05", "%.2e", 0.00001000423);
    PRC("1.00e-05", "%#.2e", 0.00001000423);
  }

  { // overly high precision
    if (NANOPRINTF_CONVERSION_BUFFER_SIZE > 1 + 30 + 1 + 4) {
      PRC("1.2345000000000000639488462184090167284011840820312500e+01" , "%.52e", 12.345);
    }
  }

  { // NPF errors
    // we give excessive precision.
    // These are needed for "accessories":
    //   M = 1 (integral) + 1 (decimal point) + 1 (e) + 1 (exp sign) + N (exp digits, min 2)
    // Then, NANOPRINTF_CONVERSION_BUFFER_SIZE - M is the max precision (fractional digits) we can have
    PR("err" , "%.*e", NANOPRINTF_CONVERSION_BUFFER_SIZE - 7 + 1, 1.2345e-100);
    PR("err" , "%.*e", NANOPRINTF_CONVERSION_BUFFER_SIZE - 7 + 100, 1.2345e-100);

    PR("err" , "%.*e", NANOPRINTF_CONVERSION_BUFFER_SIZE - 6 + 1, 1.2345e10);
    PR("err" , "%.*e", NANOPRINTF_CONVERSION_BUFFER_SIZE - 6 + 100, 1.2345e10);

    PR("err" , "%.*e", NANOPRINTF_CONVERSION_BUFFER_SIZE - 6 + 1, 1.2345e0);
    PR("err" , "%.*e", NANOPRINTF_CONVERSION_BUFFER_SIZE - 6 + 100, 1.2345e0);
  }

  // TODO: we should also have some randomized tests (finite-only) against
  // known-good implementations (Grisu-Exact, Ryū, Dragonbox, ...),
  // at least up to 16 significant digits (since we can differ in the 17th)

  // TODO: we should also have some randomized round-trip tests (finite-only),
  // reading the string back with scanf (or, better, a known-good parser)
}

void test_f(void)
{
  { // misc
    PRC("10", "%.0f", 9.87654321e0);
    PRC("9.9", "%.1f", 9.87654321e0);
    PRC("9.88", "%.2f", 9.87654321e0);
    PRC("9.877", "%.3f", 9.87654321e0);
    PRC("9.8765", "%.4f", 9.87654321e0);
    PRC("9.87654", "%.5f", 9.87654321e0);
    PRC("9.876543", "%.6f", 9.87654321e0);
    PRC("9.8765432", "%.7f", 9.87654321e0);
    PRC("9.87654321", "%.8f", 9.87654321e0);
    PRC("9.876543210", "%.9f", 9.87654321e0);
    PRC("988", "%.0f", 9.87654321e2);
    PRC("987.7", "%.1f", 9.87654321e2);
    PRC("987.65", "%.2f", 9.87654321e2);
    PRC("987.654", "%.3f", 9.87654321e2);
    PRC("987.6543", "%.4f", 9.87654321e2);
    PRC("987.65432", "%.5f", 9.87654321e2);
    PRC("987.654321", "%.6f", 9.87654321e2);
    PRC("987.6543210", "%.7f", 9.87654321e2);
    PRC("987.65432100", "%.8f", 9.87654321e2);
    PRC("987.654321000", "%.9f", 9.87654321e2);
    PRC("0", "%.0f", 9.87654321e-4);
    PRC("0.0", "%.1f", 9.87654321e-4);
    PRC("0.00", "%.2f", 9.87654321e-4);
    PRC("0.001", "%.3f", 9.87654321e-4);
    PRC("0.0010", "%.4f", 9.87654321e-4);
    PRC("0.00099", "%.5f", 9.87654321e-4);
    PRC("0.000988", "%.6f", 9.87654321e-4);
    PRC("0.0009877", "%.7f", 9.87654321e-4);
    PRC("0.00098765", "%.8f", 9.87654321e-4);
    PRC("0.000987654", "%.9f", 9.87654321e-4);
    PRC("0.0009876543", "%.10f", 9.87654321e-4);
    PRC("0.00098765432", "%.11f", 9.87654321e-4);
    PRC("0.000987654321", "%.12f", 9.87654321e-4);
    PRC("0.0009876543210", "%.13f", 9.87654321e-4);
  }
  {
    // Trailing '0's and dp
    PRC("0.000000", "%f", 0.0);
    PRC("0.0", "%.1f", 0.0);
    PRC("1.000000", "%f", 1.0);
    PRC("1.0", "%.1f", 1.0);
    PRC("0.000000", "%f", 1.234e-8);
    PRC("123400000.000000", "%f", 1.234e+8);
#if NANOPRINTF_USE_ALT_FORM_FLAG == 1
    PR ("0.000000", "%#f", 0.0);
    PR ("0.0", "%#.1f", 0.0);
    PRC("1.000000", "%#f", 1.0);
    PRC("1.0", "%#.1f", 1.0);
    PRC("1.000000", "%#f", 1.0);
    PRC("0.000000", "%#f", 1.234e-8);
    PRC("123400000.000000", "%#f", 1.234e+8);
#endif

    PRC("10.0000", "%.4f", 10.00004);
    PRC("10.0000", "%.4f", 10.00004);
    PRC("10", "%.0f", 10.00004);
    PRC("100040", "%.0f", 100040.0);
    PRC("100042", "%.0f", 100042.3);
    PRC("0.010", "%.3f", 0.01000423);
    PRC("0.000010", "%.6f", 0.00001000423);
#if NANOPRINTF_USE_ALT_FORM_FLAG == 1
    PRC("100040.", "%#.0f", 100040.0);
    PRC("100042.", "%#.0f", 100042.3);
    PRC("0.010", "%#.3f", 0.01000423);
    PRC("0.000010", "%#.6f", 0.00001000423);
#endif
  }
  { // from "conformance.cc"
    PRC("inf", "%f", (double)INFINITY);
    PRC("-inf", "%f", -(double)INFINITY);
#if NANOPRINTF_USE_FIELD_WIDTH_FORMAT_SPECIFIERS == 1
    PRC(" inf", "%4f", (double)INFINITY);
#endif // NANOPRINTF_USE_FIELD_WIDTH_FORMAT_SPECIFIERS
    PRC("inf", "%.100f", (double)INFINITY);
    PRC("inf", "%.10f", (double)INFINITY);
    //PRC("inf", "%.10e", (double)INFINITY);
    //PRC("inf", "%.10g", (double)INFINITY);
    //PRC("inf", "%.10a", (double)INFINITY);
    PRC("INF", "%F", (double)INFINITY);
    PRC("0.000000", "%f", 0.0);
    PRC("-0.000000", "%f", -0.0);
    PRC("0.000000", "%f", 1e-20);
    PRC("-0.000000", "%f", -1e-20);
    PRC("0.00", "%.2f", 0.0);
    PRC("1.0", "%.1f", 1.0);
    PRC("1", "%.0f", 1.0);
#if NANOPRINTF_USE_ALT_FORM_FLAG == 1
    PRC("1.", "%#.0f", 1.0);
#endif
    PRC("1.00000000000", "%.11f", 1.0);
    PRC("1.5", "%.1f", 1.5);
    PRC("+1.5", "%+.1f", 1.5);
    PRC("-1.5", "%.1f", -1.5);
    PRC(" 1.5", "% .1f", 1.5);
#if NANOPRINTF_USE_FIELD_WIDTH_FORMAT_SPECIFIERS == 1
    PRC(" 1.0", "%4.1f", 1.0);
    PRC(" 1.500", "%6.3f", 1.5);
    PRC("0001.500", "%08.3f", 1.5);
    PRC("+001.500", "%+08.3f", 1.5);
    PRC("-001.500", "%+08.3f", -1.5);
#endif // NANOPRINTF_USE_FIELD_WIDTH_FORMAT_SPECIFIERS
    PRC("1.50000000000000000", "%.17f", 1.5);
    PRC("0.003906", "%f", 0.00390625);
    PRC("0.0039", "%.4f", 0.00390625);
    PRC("0.00390625", "%.8f", 0.00390625);
    PR ("0.00390625", "%.8Lf", (long double)0.00390625);
    PRC("-0.00390625", "%.8f", -0.00390625);
    PR ("-0.00390625", "%.8Lf", (long double)-0.00390625);
  }
}

void test_g(void)
{
  {
    // Precision 0 is equivalent to 1
    PRC("1e+32", "%.0g", 123e30);
    PRC("1e+32", "%.1g", 123e30);
    PRC("1e-27", "%.0g", 987e-30);
    PRC("1e-27", "%.1g", 987e-30);
    PRC("3", "%.0g", 3.49);
    PRC("3", "%.1g", 3.49);
  }
  {
    // Removal of trailing '0' and point (and, 0.0 must have exponent 0)
    PRC("0", "%g", 0.0);
    PRC("0", "%.1g", 0.0);
    PRC("1", "%g", 1.0);
    PRC("1", "%.1g", 1.0);
    PRC("1.234e-08", "%g", 1.234e-8);
    PRC("1.234e+08", "%g", 1.234e+8);
#if NANOPRINTF_USE_ALT_FORM_FLAG == 1
    PR ("0.00000", "%#g", 0.0); // note: some sys printfs get this wrong (they have 6 frac digits, as if the default precision was Pg=7 <-> Pe=6, whereas it is Pg=6 <-> Pe=6)
    PR ("0.", "%#.1g", 0.0); // note: some sys printfs get this wrong as "0.0"
    PRC("1.00000", "%#g", 1.0);
    PRC("1.", "%#.1g", 1.0);
    PRC("1.00000", "%#g", 1.0);
    PRC("1.23400e-08", "%#g", 1.234e-8);
    PRC("1.23400e+08", "%#g", 1.234e+8);
#endif
  }
  {
    // Selection of %f or %e.
    // We choose %f if (P > X >= -4), which is equivalent to this: the number, when
    // rounded to P significant digits (including the leading one) is of the form
    //   xxy.yyyyxxx0000
    // that is there are significant digits in at least one place between the units
    // and the ten-thousands.
    PRC("4567000000000", "%.14g", 4.567e12);
    PRC("4567000000000", "%.13g", 4.567e12);
    PRC("4.567e+12", "%.12g", 4.567e12);
    PRC("4.567e+12", "%.11g", 4.567e12);
    PRC("4.567e+12", "%.11g", 4.567e12);
    PRC("4.567", "%.4g", 4.567);
    PRC("4.57", "%.3g", 4.567);
    PRC("4.6", "%.2g", 4.567);
    PRC("5", "%.1g", 4.567);
    PRC("6.32196e+07", "%g", 6.321957e7);
    PRC("6.32196e+06", "%g", 6.321957e6);
    PRC("632196", "%g", 6.321957e5);
    PRC("63219.6", "%g", 6.321957e4);
    PRC("6321.96", "%g", 6.321957e3);
    PRC("632.196", "%g", 6.321957e2);
    PRC("63.2196", "%g", 6.321957e1);
    PRC("6.32196", "%g", 6.321957e0);
    PRC("0.632196", "%g", 6.321957e-1);
    PRC("0.0632196", "%g", 6.321957e-2);
    PRC("0.00632196", "%g", 6.321957e-3);
    PRC("0.000632196", "%g", 6.321957e-4);
    PRC("6.32196e-05", "%g", 6.321957e-5);
    PRC("4.567e-25", "%.14g", 4.567e-25);
    PRC("4.567e-25", "%.5g", 4.567e-25);
    PRC("5e-25", "%.1g", 4.567e-25);
    PRC("10.00004", "%.7g", 10.00004);
    PRC("10", "%.6g", 10.00004);
    PRC("10", "%.2g", 10.00004);
    PRC("1e+01", "%.1g", 10.00004);
    PRC("1e+05", "%.3g", 100040.0);
    PRC("1e+05", "%.3g", 100042.3);
    PRC("0.01", "%.3g", 0.01000423);
    PRC("1e-05", "%.3g", 0.00001000423);
#if NANOPRINTF_USE_ALT_FORM_FLAG == 1
    PRC("1.00e+05", "%#.3g", 100040.0);
    PRC("1.00e+05", "%#.3g", 100042.3);
    PRC("0.0100", "%#.3g", 0.01000423);
    PRC("1.00e-05", "%#.3g", 0.00001000423);
#endif
  }
  {
    // Cases where rounding can cause us to revisit our initial %f~%e choice
    PRC("9999", "%.4g", 9999.0); // no rounding
#if NANOPRINTF_USE_ALT_FORM_FLAG == 1
    PRC("9999.", "%#.4g", 9999.0); // no rounding
#endif
    PRC("9.999e-05", "%.4g", 0.9999e-4); // no rounding
    PRC("0.0001", "%.3g", 0.9999e-4);
    PRC("0.001", "%.3g", 0.9999e-3);
    PRC("0.01", "%.3g", 0.9999e-2);
    PRC("0.1", "%.3g", 0.9999e-1);
    PRC("1", "%.3g", 0.9999e0);
    PRC("10", "%.3g", 0.9999e1);
    PRC("100", "%.3g", 0.9999e2);
    PRC("1e+03", "%.3g", 0.9999e3);
    PRC("1e+04", "%.3g", 0.9999e4);
    PRC("1e+05", "%.3g", 0.9999e5);
    PRC("0.0001", "%.5g", 1e-4);
    PRC("0.0001", "%.4g", 1e-4); // actually 0.0000999999999999889865875957184471189975738525390625
    PRC("0.0001", "%.3g", 1e-4);
  }
}
