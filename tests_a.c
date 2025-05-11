#define NANOPRINTF_VISIBILITY_STATIC
#define NANOPRINTF_USE_FIELD_WIDTH_FORMAT_SPECIFIERS    1
#define NANOPRINTF_USE_PRECISION_FORMAT_SPECIFIERS      1
#define NANOPRINTF_USE_FLOAT_FORMAT_SPECIFIERS          1
#define NANOPRINTF_USE_LARGE_FORMAT_SPECIFIERS          1
#define NANOPRINTF_USE_BINARY_FORMAT_SPECIFIERS         1
#define NANOPRINTF_USE_WRITEBACK_FORMAT_SPECIFIERS      1
#define NANOPRINTF_USE_FLOAT_HEX_FORMAT_SPECIFIER       1
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

int main(void)
{
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

  ////////////////

  // TODO: the tests for f32 are yet to be validated

  // %a that print 0.0 (with an explicit precision) are the only ones with a unique mandated output string

  { // basic tests
    PRC("0x0p+0"    , "%.0a", 0.0);
    PRC("0X0P+0"    , "%.0A", 0.0);
    PRC("-0x0p+0"   , "%.0a", -0.0);
    PRC("-0X0P+0"   , "%.0A", -0.0);

    PRC("0x0.0p+0"  , "%.1a", 0.0);
    PRC("0x0.00p+0" , "%.2a", 0.0);
  }
  { // flags/fields combinations
    PRC("0x0.p+0"              , "%#.0a", 0.0);
    PRC("0x0.0p+0"             , "%#.1a", 0.0);
    PRC("0x0.00p+0"            , "%#.2a", 0.0);

    PRC("0x0.0p+0"             , "%0.1a", 0.0);
    PRC("0x0.000p+0          " , "%-20.3a", 0.0);

    PRC("0x00000000000.000p+0" , "%#020.3a", 0.0);
    PRC("0x0.000p+0          " , "%#-20.3a", 0.0);
    PRC("          0x0.000p+0" , "%#*.3a", 20, 0.0);
    PRC("0x0.000p+0          " , "%#*.3a", -20, 0.0);
    PRC("+0x0.000p+0"          , "%#+.3a", 0.0);
    PRC("-0x0.000p+0"          , "%#+.3a", -0.0);
    PRC(" 0x0.000p+0"          , "%# .3a", 0.0);

    PRC("0x00000000000.000p+0" , "%020.3a", 0.0);
    PRC("0x00000000000.000p+0" , "%0*.3a", 20, 0.0);
    PRC("0x0.000p+0          " , "%0*.3a", -20, 0.0);
    PRC("+0x0000000000.000p+0" , "%+020.3a", 0.0);
    PRC("-0x0000000000.000p+0" , "%+020.3a", -0.0);
    PRC(" 0x0000000000.000p+0" , "% 020.3a", 0.0);

    PRC("0x0.000p+0          " , "%-20.3a", 0.0);
    PRC("+0x0.000p+0         " , "%-+20.3a", 0.0);
    PRC("+0x0.000p+0         " , "%-+*.3a", 20, 0.0);
    PRC("+0x0.000p+0"          , "%-+.3a", 0.0);
    PRC("-0x0.000p+0"          , "%-+.3a", -0.0);
    PRC("0x0.000p+0"           , "%-.3a", 0.0);

    PRC("+0x0000000000.000p+0" , "%+020.3a", 0.0);
    PRC("         +0x0.000p+0" , "%+20.3a", 0.0);
    PRC("         +0x0.000p+0" , "%+*.3a", 20, 0.0);
    PRC("+0x0.000p+0         " , "%+*.3a", -20, 0.0);
    PRC("+0x0.000p+0"          , "%+.3a", 0.0);
    PRC("-0x0.000p+0"          , "%+.3a", -0.0);
    PRC("+0x0.000p+0"          , "%+ .3a", 0.0);
  }

  // From now on, all tests are about UB/IB
  // nan/inf are mandated, but "nan" and "inf" themselves are IB
  // All other cases are IB

  { // nan/inf
    PRC("inf"        , "%a", +INFINITY);
    PRC("-inf"       , "%a", -INFINITY);
    PRC("nan"        , "%a", NAN); // should use npf_nan()

    PRC("inf"        , "%#a", +INFINITY);
    PRC("-inf"       , "%#a", -INFINITY);
    PRC("nan"        , "%#a", NAN); // should use npf_nan()

    PRC("INF"        , "%#A", +INFINITY);

    PR ("       inf" , "%#010a", +INFINITY); // NOTE: some implementations are buggy and print "0000000inf" in this and similar cases

    PRC("inf"        , "%.8a", +INFINITY);
    PRC("       inf" , "%10.8a", +INFINITY);
    PR ("       inf" , "%010.8a", +INFINITY);
    PRC("inf"        , "%.0a", +INFINITY);
    PR ("inf"        , "%#.0a", +INFINITY); // NOTE: some implementations are buggy and print "i.nf" in this and similar cases
    PRC("inf       " , "%-10.0a", +INFINITY);
    PRC("+inf"       , "%+.0a", +INFINITY);
    PRC(" inf"       , "% .0a", +INFINITY);
    PRC("       inf" , "%10.0a", +INFINITY);
    PR ("       inf" , "%#10.0a", +INFINITY);
  }

  { // auto precision
    PRC(FF("0x0.0000000000000p+0"  , "0x0.00000000p+0" ) , "%a", 0.0);
    PRC(FF("0X0.0000000000000P+0"  , "0X0.00000000P+0" ) , "%A", 0.0);
    PRC(FF("-0x0.0000000000000p+0" , "-0x0.00000000p+0") , "%a", -0.0);
    PRC(FF("-0X0.0000000000000P+0" , "-0X0.00000000P+0") , "%A", -0.0);

    PRC(FF("0x1.2345000000000p+0"  , "0x1.23450000p+0" ) , "%a", 0x1.2345p0);
    PRC(FF("0x1.23456788ab000p+0"  , "0x1.23456788p+0" ) , "%a", 0x1.23456788ABp0);

    PRC(FF("0x1.2345000000000p+100", "0x1.23450000p+100") , "%a", 0x1.2345p100);
    PRC(FF("0x1.23456788ab000p+100", "0x1.23456788p+100") , "%a", 0x1.23456788ABp100);

    PRC(FF("0x1.2345000000000p-100", "0x1.23450000p-100") , "%a", 0x1.2345p-100);
    PRC(FF("0x1.23456788ab000p-100", "0x1.23456788p-100") , "%a", 0x1.23456788ABp-100);
  }

  { // long double
    PR (FF("0x1.23456788abcdep+0", "0x1.23456788p+0") , "%La", 0x1.23456788ABCDE7p0L);
    PR (FF("0x1.23456788abcdep+0", "0x1.23456788p+0") , "%La", 0x1.23456788ABCDE8p0L); // The (long double) -> (double) conversion might depend on the environments' settings for rounding.
    PR (FF("0x1.fffffffefffffp+0", "0x1.fffffffep+0") , "%La", 0x1.FFFFFFFEFFFFF7p0L);
    PR (FF("0x1.0000000000000p+1", "0x1.00000000p+1") , "%La", 0x1.FFFFFFFFFFFFF8p0L); // The (long double) -> (double) conversion might depend on the environments' settings for rounding.
    PR (FF("0x1.ffffffff00000p+0", "0x1.fffffffep+0") , "%La", 0x1.FFFFFFFEFFFFF8p0L); // The (long double) -> (double) conversion might depend on the environments' settings for rounding.
    PR (   "0x2p+0"                                   , "%.0La", 0x1.FFFFFFFFFFFFF7p0L);
    PR (FF("0X1.23456788ABCDEP+0", "0X1.23456788P+0") , "%LA", 0x1.23456788ABCDE7p0L);
  }

  { // misc values
    PRC("0x1.0091177587f83p-1022" , "%a", 2.23e-308),
    PRC(FF("0x1.23456788abcdep+0"   , "0x1.23456788p+0"   ) , "%a", 0x1.23456788ABCDEp0);
    PRC(FF("0x1.0000000000000p-1020", "0x0.00000000p+0"   ) , "%a", 0x1p-1020);
    PRC(FF("0x1.0000000000000p-1021", "0x0.00000000p+0"   ) , "%a", 0x1p-1021);
    PRC(FF("0x1.0000000000000p-1022", "0x0.00000000p+0"   ) , "%a", 0x1p-1022);
    PRC(FF("0x0.8000000000000p-1022", "0x0.00000000p+0"   ) , "%a", 0x1p-1023);
    PRC(FF("0x0.4000000000000p-1022", "0x0.00000000p+0"   ) , "%a", 0x1p-1024);
    PRC(FF("0x0.2000000000000p-1022", "0x0.00000000p+0"   ) , "%a", 0x1p-1025);
    PRC(FF("0x0.1000000000000p-1022", "0x0.00000000p+0"   ) , "%a", 0x1p-1026);
    PRC(FF("0x0.0800000000000p-1022", "0x0.00000000p+0"   ) , "%a", 0x1p-1027);
    PRC(FF("0x0.0400000000000p-1022", "0x0.00000000p+0"   ) , "%a", 0x1p-1028);
    PRC(FF("0x0.0200000000000p-1022", "0x0.00000000p+0"   ) , "%a", 0x1p-1029);
    PRC(FF("0x0.0000000000040p-1022", "0x0.00000000p+0"   ) , "%a", 0x1p-1068);
    PRC(FF("0x0.0000000000020p-1022", "0x0.00000000p+0"   ) , "%a", 0x1p-1069);
    PRC(FF("0x0.0000000000010p-1022", "0x0.00000000p+0"   ) , "%a", 0x1p-1070);
    PRC(FF("0x0.0000000000008p-1022", "0x0.00000000p+0"   ) , "%a", 0x1p-1071);
    PRC(FF("0x0.0000000000004p-1022", "0x0.00000000p+0"   ) , "%a", 0x1p-1072);
    PRC(FF("0x0.0000000000002p-1022", "0x0.00000000p+0"   ) , "%a", 0x1p-1073);
    PRC(FF("0x0.0000000000001p-1022", "0x0.00000000p+0"   ) , "%a", 0x1p-1074);
    PRC(FF("0x1.0000000000000p+1020", "0x0.00000000p+0"   ) , "%a", 0x1p+1020);
    PRC(FF("0x1.0000000000000p+1021", "0x0.00000000p+0"   ) , "%a", 0x1p+1021);
    PRC(FF("0x1.0000000000000p+1022", "0x0.00000000p+0"   ) , "%a", 0x1p+1022);
    PRC(FF("0x1.0000000000000p+1023", "0x0.00000000p+0"   ) , "%a", 0x1p+1023);
    PRC(FF("0x1.0000000000000p-126" , "0x1.00000000p-126" ) , "%a", 0x1p-126);
    PRC(FF("0x1.0000000000000p-127" , "0x1.00000000p-127" ) , "%a", 0x1p-127);
    PRC(FF("0x1.0000000000000p-128" , "0x1.00000000p-128" ) , "%a", 0x1p-128);
    PRC(FF("0x1.0000000000000p-129" , "0x1.00000000p-129" ) , "%a", 0x1p-129);
    PRC(FF("0x1.0000000000000p-130" , "0x1.00000000p-130" ) , "%a", 0x1p-130);
    PRC(FF("0x1.0000000000000p-149" , "0x1.00000000p-149" ) , "%a", 0x1p-149);
    PRC(FF("0x1.0000000000000p+124" , "0x1.00000000p+124" ) , "%a", 0x1p+124);
    PRC(FF("0x1.0000000000000p+125" , "0x1.00000000p+125" ) , "%a", 0x1p+125);
    PRC(FF("0x1.0000000000000p+126" , "0x1.00000000p+126" ) , "%a", 0x1p+126);
    PRC(FF("0x1.0000000000000p+127" , "0x1.00000000p+127" ) , "%a", 0x1p+127);
    PRC(FF("0x1.0000000000000p+1"   , "0x1.00000000p+1"   ) , "%a", 0x1p+1);
    PRC(FF("0x1.0000000000000p+2"   , "0x1.00000000p+2"   ) , "%a", 0x1p+2);
    PRC(FF("0x1.fffffffffffffp+1020", "inf"               ) , "%a", 0x1.fffffffffffffp+1020);
    PRC(FF("0x1.fffffffe00000p+126" , "0x1.fffffffep+126" ) , "%a", 0x1.fffffffep+126);
    PRC(FF("0x1.0000000000000p+0"   , "0x1.00000000p+0"   ) , "%a", 0x1p0);
    PRC(FF("0x1.8000000000000p+0"   , "0x1.80000000p+0"   ) , "%a", 0x1.8p0);
    PRC(FF("0x1.c000000000000p+0"   , "0x1.c0000000p+0"   ) , "%a", 0x1.cp0);
    PRC(FF("0x1.e000000000000p+0"   , "0x1.e0000000p+0"   ) , "%a", 0x1.ep0);
    PRC(FF("0x1.f000000000000p+0"   , "0x1.f0000000p+0"   ) , "%a", 0x1.fp0);
    PRC(FF("0x1.f800000000000p+0"   , "0x1.f8000000p+0"   ) , "%a", 0x1.f8p0);
    PRC(FF("0x1.ff00000000000p+0"   , "0x1.ff000000p+0"   ) , "%a", 0x1.ffp0);
    PRC(FF("0x1.fff0000000000p+0"   , "0x1.fff00000p+0"   ) , "%a", 0x1.fffp0);
    PRC(FF("0x1.ffff000000000p+0"   , "0x1.ffff0000p+0"   ) , "%a", 0x1.ffffp0);
    PRC(FF("0x1.fffff00000000p+0"   , "0x1.fffff000p+0"   ) , "%a", 0x1.fffffp0);
    PRC(FF("0x1.ffffff0000000p+0"   , "0x1.ffffff00p+0"   ) , "%a", 0x1.ffffffp0);
    PRC(FF("0x1.fffffff000000p+0"   , "0x1.fffffff0p+0"   ) , "%a", 0x1.fffffffp0);
    PRC(FF("0x1.fffffffe00000p+0"   , "0x1.fffffffep+0"   ) , "%a", 0x1.fffffffep0);
    PRC(FF("0x1.fffffffef0000p+0"   , "0x1.fffffffep+0"   ) , "%a", 0x1.fffffffefp0);
    PRC(FF("0x1.fffffffeff000p+0"   , "0x1.fffffffep+0"   ) , "%a", 0x1.fffffffeffp0);
    PRC(FF("0x1.fffffffefff00p+0"   , "0x1.fffffffep+0"   ) , "%a", 0x1.fffffffefffp0);
    PRC(FF("0x1.fffffffeffff0p+0"   , "0x1.fffffffep+0"   ) , "%a", 0x1.fffffffeffffp0);
    PRC(FF("0x1.fffffffefffffp+0"   , "0x1.fffffffep+0"   ) , "%a", 0x1.fffffffefffffp0);
    PRC(FF("0x1.fdb9752e0a865p+126" , "0x1.fdb9752ep+126" ) , "%a", 0xF.EDCBA97054328p+123);
    PRC(FF("0x1.04682f746ca3dp-45"  , "0x1.04682f74p-45"  ) , "%a", 0x1.04682F746CA3Dp-45);
    PRC(FF("-0x1.23456788abcdep+0"  , "-0x1.23456788p+0"  ) , "%a", -0x1.23456788ABCDEp0);
    PRC(FF("-0x1.ffff000000000p+104", "-0x1.ffff0000p+104") , "%a", -0x1.ffffp104);
    PRC(FF("-0x1.ffff000000000p+729", "-inf"              ) , "%a", -0x1.ffffp729);
    PRC(FF("-0x1.ade8300000000p-999", "0x0.00000000p+0"   ) , "%a", -0x1.ADE83p-999);
    PRC(FF("-0x1.ade8300000000p-118", "-0x1.ade83000p-118") , "%a", -0x1.ADE83p-118);
  }

  { // case
    PRC("0X1.0091177587F83P-1022" , "%A", 2.23e-308),
    PRC(FF("0X1.23456788ABCDEP+0", "0X1.23456788P+0") , "%A", 0x1.23456788ABCDEp0);
    PRC("0X1.0000000000000P+1"    , "%A", 0x1p+1);
    PRC("0X1.0000000000000P+2"    , "%A", 0x1p+2);
    PRC("0X0.0000000000000P+0"    , "%A", 0x0p0);
    PRC("0X1.0000000000000P+0"    , "%A", 0x1p0);
    PRC("0X1.8000000000000P+0"    , "%A", 0x1.8p0);
    PRC("0X1.C000000000000P+0"    , "%A", 0x1.cp0);
    PRC("0X1.E000000000000P+0"    , "%A", 0x1.ep0);
    PRC("0X1.F000000000000P+0"    , "%A", 0x1.fp0);
    PRC("0X1.F800000000000P+0"    , "%A", 0x1.f8p0);
    PRC("0X1.FF00000000000P+0"    , "%A", 0x1.ffp0);
    PRC("0X1.FFF0000000000P+0"    , "%A", 0x1.fffp0);
    PRC("0X1.FDB9740000000P+48"   , "%A", 0xF.EDCBAp+45);
    PRC("0X1.04682F0000000P-45"   , "%A", 0x1.04682Fp-45);
    PRC("-0X1.AC23000000000P+0"   , "%A", -0x1.AC23p0);
    PRC("-0X1.FFFF000000000P-34"  , "%A", -0x1.ffffp-34);
  }

  { // hash
    PRC("0x0.0000000000000p+0"  , "%#a", 0x0p0);
    PRC("0x1.0000000000000p+0"  , "%#a", 0x1p0);
    PRC("0x1.4500000000000p+8"  , "%#020a", 0x1.45p8);
    PRC("0x1.4500000000000p+8"  , "%#-20a", 0x1.45p8);
    PRC(" 0x1.4500000000000p+8" , "%# 20a", 0x1.45p8);
    PRC("+0x1.4500000000000p+8" , "%#+20a", 0x1.45p8);
    PRC("            +0x1.p+8"  , "%#+20.0a", 0x1.45p8);
    PRC("           +0x1.4p+8"  , "%#+20.1a", 0x1.45p8);
    PRC("          +0x1.45p+8"  , "%#+20.2a", 0x1.45p8);
    PRC("0x00000000001.450p+8"  , "%#020.3a", 0x1.45p8);
    PRC("   0x1.45000p+8"       , "%#*.*a", 15, 5, 0x1.45p8);
    PRC("0x1.4500000p+8      "  , "%#*.*a", -20, 7, 0x1.45p8);
    PRC("0x1.4500000000000p+8"  , "%#*.*a", -20, -7, 0x1.45p8);

    PRC("-0x0.0000000000000p+0" , "%#a", -0x0p0);
    PRC("-0x1.0000000000000p+0" , "%#a", -0x1p0);
    PRC("-0x1.4500000000000p+8" , "%#020a", -0x1.45p8);
    PRC("-0x1.4500000000000p+8" , "%#-20a", -0x1.45p8);
    PRC("-0x1.4500000000000p+8" , "%# 20a", -0x1.45p8);
    PRC("-0x1.4500000000000p+8" , "%#+20a", -0x1.45p8);
    PRC("-0x1.4500000000000p+8" , "%#020a", -0x1.45p8);
    PRC("-0x1.4500000000000p+8" , "%#-20a", -0x1.45p8);
    PRC("-0x1.4500000000000p+8" , "%# 20a", -0x1.45p8);
    PRC("-0x1.4500000000000p+8" , "%#+20a", -0x1.45p8);
    PRC("            -0x1.p+8"  , "%#+20.0a", -0x1.45p8);
    PRC("           -0x1.4p+8"  , "%#+20.1a", -0x1.45p8);
    PRC("          -0x1.45p+8"  , "%#+20.2a", -0x1.45p8);
    PRC("-0x0000000001.450p+8"  , "%#020.3a", -0x1.45p8);
    PRC("  -0x1.45000p+8"       , "%#*.*a", 15, 5, -0x1.45p8);
    PRC("-0x1.4500000p+8     "  , "%#*.*a", -20, 7, -0x1.45p8);
    PRC("-0x1.4500000000000p+8" , "%#*.*a", -20, -7, -0x1.45p8);
  }

  { // space
    PRC(" 0x0.0000000000000p+0" , "% 20a", 0x0p0);
    PRC(" 0x1.0000000000000p+0" , "% 20a", 0x1p0);
    PRC(" 0x1.fa90000000000p-5" , "% 020a", 0x1.FA9p-5);
    PRC(" 0x1.fa90000000000p-5" , "% -20a", 0x1.FA9p-5);
    PRC(" 0x1.fa90000000000p-5" , "%# 20a", 0x1.FA9p-5);
    PRC("+0x1.fa90000000000p-5" , "% +20a", 0x1.FA9p-5);
    PRC("             +0x2p-5"  , "% +20.0a", 0x1.FA9p-5);
    PRC("           +0x2.0p-5"  , "% +20.1a", 0x1.FA9p-5);
    PRC("          +0x1.fbp-5"  , "% +20.2a", 0x1.FA9p-5);
    PRC(" 0x0000000001.fa9p-5"  , "% 020.3a", 0x1.FA9p-5);
    PRC("   0x1.fa900p-5"       , "% *.*a", 15, 5, 0x1.FA9p-5);
    PRC(" 0x1.fa90000p-5     "  , "% *.*a", -20, 7, 0x1.FA9p-5);
    PRC(" 0x1.fa90000000000p-5" , "% *.*a", -20, -7, 0x1.FA9p-5);

    PRC("-0x0.0000000000000p+0" , "% 20a", -0x0p0);
    PRC("-0x1.0000000000000p+0" , "% 20a", -0x1p0);
    PRC("-0x1.fa90000000000p-5" , "% 020a", -0x1.FA9p-5);
    PRC("-0x1.fa90000000000p-5" , "% -20a", -0x1.FA9p-5);
    PRC("-0x1.fa90000000000p-5" , "%# 20a", -0x1.FA9p-5);
    PRC("-0x1.fa90000000000p-5" , "% +20a", -0x1.FA9p-5);
    PRC("             -0x2p-5"  , "% +20.0a", -0x1.FA9p-5);
    PRC("           -0x2.0p-5"  , "% +20.1a", -0x1.FA9p-5);
    PRC("          -0x1.fbp-5"  , "% +20.2a", -0x1.FA9p-5);
    PRC("-0x0000000001.fa9p-5"  , "% 020.3a", -0x1.FA9p-5);
    PRC("  -0x1.fa900p-5"       , "% *.*a", 15, 5, -0x1.FA9p-5);
    PRC("-0x1.fa90000p-5     "  , "% *.*a", -20, 7, -0x1.FA9p-5);
    PRC("-0x1.fa90000000000p-5" , "% *.*a", -20, -7, -0x1.FA9p-5);
  }

  { // plus
    PRC("+0x0.0000000000000p+0"  , "%+a", 0x0p0);
    PRC("+0x1.0000000000000p+0"  , "%+a", 0x1p0);
    PRC("+0x1.9360000000000p+59" , "%+020a", 0x1.936p59);
    PRC("+0x1.9360000000000p+59" , "%+-20a", 0x1.936p59);
    PRC("+0x1.9360000000000p+59" , "%+ 20a", 0x1.936p59);
    PRC("+0x1.9360000000000p+59" , "%#+20a", 0x1.936p59);
    PRC("           +0x2.p+59"   , "%#+20.0a", 0x1.936p59);
    PRC("          +0x1.9p+59"   , "%#+20.1a", 0x1.936p59);
    PRC("         +0x1.93p+59"   , "%#+20.2a", 0x1.936p59);
    PRC("+0x000000001.936p+59"   , "%+020.3a", 0x1.936p59);
    PRC(" +0x1.93600p+59"        , "%+*.*a", 15, 5, 0x1.936p59);
    PRC("+0x1.9360000p+59    "   , "%+*.*a", -20, 7, 0x1.936p59);
    PRC("+0x1.9360000000000p+59" , "%+*.*a", -20, -7, 0x1.936p59);

    PRC("-0x0.0000000000000p+0"  , "%+a", -0x0p0);
    PRC("-0x1.0000000000000p+0"  , "%+a", -0x1p0);
    PRC("-0x1.9360000000000p+59" , "%+020a", -0x1.936p59);
    PRC("-0x1.9360000000000p+59" , "%+-20a", -0x1.936p59);
    PRC("-0x1.9360000000000p+59" , "%+ 20a", -0x1.936p59);
    PRC("-0x1.9360000000000p+59" , "%#+20a", -0x1.936p59);
    PRC("           -0x2.p+59"   , "%#+20.0a", -0x1.936p59);
    PRC("          -0x1.9p+59"   , "%#+20.1a", -0x1.936p59);
    PRC("         -0x1.93p+59"   , "%#+20.2a", -0x1.936p59);
    PRC("-0x000000001.936p+59"   , "%+020.3a", -0x1.936p59);
    PRC(" -0x1.93600p+59"        , "%+*.*a", 15, 5, -0x1.936p59);
    PRC("-0x1.9360000p+59    "   , "%+*.*a", -20, 7, -0x1.936p59);
    PRC("-0x1.9360000000000p+59" , "%+*.*a", -20, -7, -0x1.936p59);
  }

  { // minus
    PRC("0x0.0000000000000p+0          " , "%-30a", 0x0p0);
    PRC("0x1.0000000000000p+0          " , "%-30a", 0x1p0);
    PRC("0x1.b050000000000p-99"          , "%-020a", 0x1.B05p-99);
    PRC("+0x1.b050000000000p-99"         , "%+-20a", 0x1.B05p-99);
    PRC(" 0x1.b050000000000p-99"         , "%- 20a", 0x1.B05p-99);
    PRC("0x1.b050000000000p-99"          , "%#-20a", 0x1.B05p-99);
    PRC("0x2.p-99            "           , "%#-20.0a", 0x1.B05p-99);
    PRC("0x1.bp-99           "           , "%#-20.1a", 0x1.B05p-99);
    PRC("0x1.b0p-99          "           , "%#-20.2a", 0x1.B05p-99);
    PRC("0x1.b05p-99         "           , "%-020.3a", 0x1.B05p-99);
    PRC("0x1.b0500p-99  "                , "%-*.*a", 15, 5, 0x1.B05p-99);
    PRC("0x1.b050000p-99     "           , "%-*.*a", -20, 7, 0x1.B05p-99);
    PRC("0x1.b050000000000p-99"          , "%-*.*a", -20, -7, 0x1.B05p-99);

    PRC("-0x0.0000000000000p+0         " , "%-30a", -0x0p0);
    PRC("-0x1.0000000000000p+0         " , "%-30a", -0x1p0);
    PRC("-0x1.b050000000000p-99"         , "%-020a", -0x1.B05p-99);
    PRC("-0x1.b050000000000p-99"         , "%+-20a", -0x1.B05p-99);
    PRC("-0x1.b050000000000p-99"         , "%- 20a", -0x1.B05p-99);
    PRC("-0x1.b050000000000p-99"         , "%#-20a", -0x1.B05p-99);
    PRC("-0x2.p-99           "           , "%#-20.0a", -0x1.B05p-99);
    PRC("-0x1.bp-99          "           , "%#-20.1a", -0x1.B05p-99);
    PRC("-0x1.b0p-99         "           , "%#-20.2a", -0x1.B05p-99);
    PRC("-0x1.b05p-99        "           , "%-020.3a", -0x1.B05p-99);
    PRC("-0x1.b0500p-99 "                , "%-*.*a", 15, 5, -0x1.B05p-99);
    PRC("-0x1.b050000p-99    "           , "%-*.*a", -20, 7, -0x1.B05p-99);
    PRC("-0x1.b050000000000p-99"         , "%-*.*a", -20, -7, -0x1.B05p-99);
  }

  { // zero
    PRC("0x000.0000000000000p+0"         , "%022a", 0x0p0);
    PRC("0x001.0000000000000p+0"         , "%022a", 0x1p0);
    PRC("0x001.3810000000000p-4"         , "%022a", 0x1.381p-4);
    PRC("0x1.3810000000000p-4  "         , "%0-22a", 0x1.381p-4);
    PRC(" 0x01.3810000000000p-4"         , "%0 22a", 0x1.381p-4);
    PRC("0x001.3810000000000p-4"         , "%#022a", 0x1.381p-4);
    PRC("0x0000000000000001.p-4"         , "%#022.0a", 0x1.381p-4);
    PR ("0x000000000000001.4p-4"         , "%#022.1a", 0x1.381p-4); // rounding is IB
    PRC("0x00000000000001.38p-4"         , "%#022.2a", 0x1.381p-4);
    PRC("0x0000000000001.381p-4"         , "%022.3a", 0x1.381p-4);
    PRC("0x00000000000001.38100p-4"      , "%0*.*a", 25, 5, 0x1.381p-4);
    PRC("0x1.3810000p-4                " , "%0*.*a", -30, 7, 0x1.381p-4);
    PRC("0x1.3810000000000p-4          " , "%0*.*a", -30, -7, 0x1.381p-4);

    PRC("-0x00.0000000000000p+0"         , "%022a", -0x0p0);
    PRC("-0x01.0000000000000p+0"         , "%022a", -0x1p0);
    PRC("-0x01.3810000000000p-4"         , "%022a", -0x1.381p-4);
    PRC("-0x1.3810000000000p-4 "         , "%0-22a", -0x1.381p-4);
    PRC("-0x01.3810000000000p-4"         , "%0 22a", -0x1.381p-4);
    PRC("-0x01.3810000000000p-4"         , "%#022a", -0x1.381p-4);
    PRC("-0x000000000000001.p-4"         , "%#022.0a", -0x1.381p-4);
    PR ("-0x00000000000001.4p-4"         , "%#022.1a", -0x1.381p-4); // rounding is IB
    PRC("-0x0000000000001.38p-4"         , "%#022.2a", -0x1.381p-4);
    PRC("-0x000000000001.381p-4"         , "%022.3a", -0x1.381p-4);
    PRC("-0x0000000000001.38100p-4"      , "%0*.*a", 25, 5, -0x1.381p-4);
    PRC("-0x1.3810000p-4               " , "%0*.*a", -30, 7, -0x1.381p-4);
    PRC("-0x1.3810000000000p-4         " , "%0*.*a", -30, -7, -0x1.381p-4);
  }

  { // width
    PRC("0x0.00000p+0"    , "%8.5a", 0x0p0);
    PRC("   0x0.00000p+0" , "%15.5a", 0x0p0);
    PRC("0x1.00000p+0"    , "%8.5a", 0x1p0);
    PRC("   0x1.00000p+0" , "%15.5a", 0x1p0);
    PRC("0x1.23000p+7"    , "%-8.5a", 0x1.23p7);
    PRC("0x1.23000p+7   " , "%-15.5a", 0x1.23p7);
    PRC("0x0.00000p+0"    , "%*.5a", 8, 0x0p0);
    PRC("   0x0.00000p+0" , "%*.5a", 15, 0x0p0);
    PRC("0x1.00000p+0"    , "%*.5a", 8, 0x1p0);
    PRC("   0x1.00000p+0" , "%*.5a", 15, 0x1p0);
    PRC("0x1.23000p+7"    , "%*.5a", -8, 0x1.23p7);
    PRC("0x1.23000p+7   " , "%*.5a", -15, 0x1.23p7);

    PRC("-0x0.00000p+0"   , "%8.5a", -0x0p0);
    PRC("  -0x0.00000p+0" , "%15.5a", -0x0p0);
    PRC("-0x1.00000p+0"   , "%8.5a", -0x1p0);
    PRC("  -0x1.00000p+0" , "%15.5a", -0x1p0);
    PRC("-0x1.23000p+7"   , "%-8.5a", -0x1.23p7);
    PRC("-0x1.23000p+7  " , "%-15.5a", -0x1.23p7);
    PRC("-0x0.00000p+0"   , "%*.5a", 8, -0x0p0);
    PRC("  -0x0.00000p+0" , "%*.5a", 15, -0x0p0);
    PRC("-0x1.00000p+0"   , "%*.5a", 8, -0x1p0);
    PRC("  -0x1.00000p+0" , "%*.5a", 15, -0x1p0);
    PRC("-0x1.23000p+7"   , "%*.5a", -8, -0x1.23p7);
    PRC("-0x1.23000p+7  " , "%*.5a", -15, -0x1.23p7);
  }

  { // precision
    PRC("0x0.00000p+0"           , "%.5a", 0x0p0);
    PRC("0x0.00000000p+0"        , "%.8a", 0x0p0);
    PRC("0x1.00000p+0"           , "%.5a", 0x1p0);
    PRC("0x1.00000000p+0"        , "%.8a", 0x1p0);
    PRC("0x1.23000p+7"           , "%.5a", 0x1.23p7);
    PRC("0x1.23000000p+7"        , "%.8a", 0x1.23p7);
    PRC("0x0.00000000p+0"        , "%.*a", 8, 0x0p0);
    PRC("0x0.000000000000000p+0" , "%.*a", 15, 0x0p0);
    PRC("0x1.00000000p+0"        , "%.*a", 8, 0x1p0);
    PRC("0x1.000000000000000p+0" , "%.*a", 15, 0x1p0);
    PRC("0x1.2300000000000p+7"   , "%.*a", -8, 0x1.23p7);
    PRC("0x1.2300000000000p+7"   , "%.*a", -15, 0x1.23p7);

    PRC("-0x0.00000p+0"           , "%.5a", -0x0p0);
    PRC("-0x0.00000000p+0"        , "%.8a", -0x0p0);
    PRC("-0x1.00000p+0"           , "%.5a", -0x1p0);
    PRC("-0x1.00000000p+0"        , "%.8a", -0x1p0);
    PRC("-0x1.23000p+7"           , "%.5a", -0x1.23p7);
    PRC("-0x1.23000000p+7"        , "%.8a", -0x1.23p7);
    PRC("-0x0.00000000p+0"        , "%.*a", 8, -0x0p0);
    PRC("-0x0.000000000000000p+0" , "%.*a", 15, -0x0p0);
    PRC("-0x1.00000000p+0"        , "%.*a", 8, -0x1p0);
    PRC("-0x1.000000000000000p+0" , "%.*a", 15, -0x1p0);
    PRC("-0x1.2300000000000p+7"   , "%.*a", -8, -0x1.23p7);
    PRC("-0x1.2300000000000p+7"   , "%.*a", -15, -0x1.23p7);
  }

  { // rounding
    // NOTE: rounding is IB. It seems that common implementations either round-to-nearest
    // or round-to-0. We do round-to-nearest.
    PRC("0x1p+0"              , "%.0a", 0x1.7p0);
    PR ("0x2p+0"              , "%.0a", 0x1.8p0);
    PRC("0x1.8p+0"            , "%.1a", 0x1.87p0);
    PR ("0x1.9p+0"            , "%.1a", 0x1.88p0);
    PRC("0x1.fp+0"            , "%.1a", 0x1.f7p0);
    PR ("0x2.0p+0"            , "%.1a", 0x1.f8p0);
    PRC("0x1.88p+0"           , "%.2a", 0x1.887p0);
    PR ("0x1.89p+0"           , "%.2a", 0x1.888p0);
    PRC("0x1.ffp+0"           , "%.2a", 0x1.ff7p0);
    PR ("0x2.00p+0"           , "%.2a", 0x1.ff8p0);
    PRC("0x1.888p+0"          , "%.3a", 0x1.8887p0);
    PR ("0x1.889p+0"          , "%.3a", 0x1.8888p0);
    PRC("0x1.fffp+0"          , "%.3a", 0x1.fff7p0);
    PR ("0x2.000p+0"          , "%.3a", 0x1.fff8p0);
    PRC("0x1.8888p+0"         , "%.4a", 0x1.88887p0);
    PR ("0x1.8889p+0"         , "%.4a", 0x1.88888p0);
    PRC("0x1.ffffp+0"         , "%.4a", 0x1.ffff7p0);
    PR ("0x2.0000p+0"         , "%.4a", 0x1.ffff8p0);
    PRC(FF("0x1.888888p+0"      , "0x1.888888p+0"  ) , "%.6a", 0x1.8888887ffp0);
    PR (FF("0x1.88888ap+0"      , "0x1.88888ap+0"  ) , "%.6a", 0x1.8888898ffp0);
    PR (FF("0x2.000000p+0"      , "0x2.000000p+0"  ) , "%.6a", 0x1.fffffffep0);
    PRC(FF("0x1.888888888888p+0", "0x1.888888880000p+0") , "%.12a", 0x1.8888888888887p0);
    PR (FF("0x1.888888888889p+0", "0x1.888888880000p+0") , "%.12a", 0x1.8888888888888p0);
    PRC(FF("0x1.fffffffeffffp+0", "0x1.fffffffe0000p+0") , "%.12a", 0x1.fffffffeffff7p0);
    PR (FF("0x2.000000000000p+0", "0x2.000000000000p+0") , "%.12a", 0x1.ffffffffffff8p0);

    PRC("0x1.77p+0"           , "%.2a", 0x1.7777777777777p0);
    PR ("0x2.00p+0"           , "%.2a", 0x1.fffffffffffffp0);
    PRC("0x1.777p+0"          , "%.3a", 0x1.7777777777777p0);
    PR ("0x2.000p+0"          , "%.3a", 0x1.fffffffffffffp0);
    PRC("0x1.7777p+0"         , "%.4a", 0x1.7777777777777p0);
    PR ("0x2.0000p+0"         , "%.4a", 0x1.fffffffffffffp0);
    PRC("0x1.77777p+0"        , "%.5a", 0x1.7777777777777p0);
    PR ("0x2.00000p+0"        , "%.5a", 0x1.fffffffffffffp0);
    PRC("0x1.777777p+0"       , "%.6a", 0x1.7777777777777p0);
    PR ("0x2.000000p+0"       , "%.6a", 0x1.fffffffffffffp0);
    PRC("0x1.7777777p+0"      , "%.7a", 0x1.7777777777777p0);
    PR ("0x2.0000000p+0"      , "%.7a", 0x1.fffffffffffffp0);
    PRC("0x1.77777777p+0"     , "%.8a", 0x1.7777777777777p0);
    PR ("0x2.00000000p+0"     , "%.8a", 0x1.fffffffffffffp0);
    PRC(FF("0x1.777777777p+0"   , "0x1.777777780p+0"   ) , "%.9a", 0x1.7777777777777p0);
    PR (FF("0x2.000000000p+0"   , "0x2.000000000p+0"   ) , "%.9a", 0x1.fffffffffffffp0);
    PRC(FF("0x1.7777777777p+0"  , "0x1.7777777800p+0"  ) , "%.10a", 0x1.7777777777777p0);
    PR (FF("0x2.0000000000p+0"  , "0x2.0000000000p+0"  ) , "%.10a", 0x1.fffffffffffffp0);
    PRC(FF("0x1.77777777777p+0" , "0x1.77777778000p+0" ) , "%.11a", 0x1.7777777777777p0);
    PR (FF("0x2.00000000000p+0" , "0x2.00000000000p+0" ) , "%.11a", 0x1.fffffffffffffp0);
    PRC(FF("0x1.777777777777p+0", "0x1.777777780000p+0") , "%.12a", 0x1.7777777777777p0);
    PR (FF("0x2.000000000000p+0", "0x2.000000000000p+0") , "%.12a", 0x1.fffffffffffffp0);
  }

  { // subnormals
    PRC(FF("0x0.0000001234500p-1022", "0x0.00000000p0") , "%a", 0x1.2345p-1050);
    PRC(FF("0x0.0000p-1022"         , "0x0.00000000p0") , "%.4a", 0x1.2345p-1050);
    PRC(FF("0x0.00000012p-1022"     , "0x0.00000000p0") , "%.8a", 0x1.2345p-1050);
    PRC(FF("0x0.0000001234500p-1022", "0x0.00000000p0") , "%.13a", 0x1.2345p-1050);
    PRC(FF("0x0.0000000000001p-1022", "0x0.00000000p0") , "%a",  0x1p-1074);
    PRC(FF("0x0p-1022"              , "0x0.00000000p0") , "%.0a", 0x1p-1074);
    PRC(FF("0x0.000000000000p-1022" , "0x0.00000000p0") , "%.12a", 0x1p-1074);
    PRC(FF("0x0.0000000000001p-1022", "0x0.00000000p0") , "%.13a", 0x1p-1074);

    PRC("0x1.2345678900000p-135"  , "%a", 0x1.23456789p-135);
    PRC("0x1.23p-135"             , "%.2a", 0x1.2345p-135);
    PRC("0x1.2345p-135"           , "%.4a", 0x1.2345p-135);
    PRC("0x1.23450000p-135"       , "%.8a", 0x1.2345p-135);
    PRC("0x1.0000000000000p-149"  , "%a", 0x1p-149);
    PRC("0x1p-149"                , "%.0a", 0x1p-149);
    PRC("0x1.0000000p-149"        , "%.7a", 0x1p-149);
    PRC("0x1.00000000p-149"       , "%.8a", 0x1p-149);

    PRC("0x0.8000000000000p-1022", "%a", 0x1p-1023);
    PR ("0x1p-1022", "%.0a", 0x0.8p-1022);
    PR ("0x0p-1022", "%.0a", 0x0.4p-1022);
    PR ("0x1.p-1022", "%#.0a", 0x0.8p-1022);
    PR ("0x0.p-1022", "%#.0a", 0x0.4p-1022);
    PR ("0x0.8p-1022", "%.1a", 0x0.8p-1022);
    PR ("0x0.4p-1022", "%.1a", 0x0.4p-1022);
    PR ("0x0.1p-1022", "%.1a", 0x0.08p-1022);
    PR ("0x0.0p-1022", "%.1a", 0x0.04p-1022);
  }

  { // NPF errors
    // we give excessive precision.
    // These are needed for "accessories":
    //   M = 1 (integral) + 1 (decimal point) + 1 (p) + 1 (exp sign) + N (exp digits)
    // Then, NANOPRINTF_CONVERSION_BUFFER_SIZE - M is the max precision (fractional digits) we can have
    PR("err" , "%.*a", NANOPRINTF_CONVERSION_BUFFER_SIZE - 7 + 1, 0x1.2345p-100);
    PR("err" , "%.*a", NANOPRINTF_CONVERSION_BUFFER_SIZE - 7 + 100, 0x1.2345p-100);

    PR("err" , "%.*a", NANOPRINTF_CONVERSION_BUFFER_SIZE - 5 + 1, 0x1.2345p0);
    PR("err" , "%.*a", NANOPRINTF_CONVERSION_BUFFER_SIZE - 5 + 100, 0x1.2345p0);
  }

  printf("Done.");
  return 0;
}
