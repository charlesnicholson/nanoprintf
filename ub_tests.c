#define NANOPRINTF_ENABLE_SAFETY_CHECKS
#define NANOPRINTF_USE_FIELD_WIDTH_FORMAT_SPECIFIERS    1
#define NANOPRINTF_USE_PRECISION_FORMAT_SPECIFIERS      1
#define NANOPRINTF_USE_FLOAT_FORMAT_SPECIFIERS          1
#define NANOPRINTF_USE_LARGE_FORMAT_SPECIFIERS          1
#define NANOPRINTF_USE_BINARY_FORMAT_SPECIFIERS         1
#define NANOPRINTF_USE_WRITEBACK_FORMAT_SPECIFIERS      1
#define NANOPRINTF_IMPLEMENTATION
#include "nanoprintf.h"

#include <stdio.h>
#include <string.h>

#define PR(...)    pr(__LINE__, __VA_ARGS__)

void pr(int line, const char *format, ...)
{
  va_list list;
  va_start(list, format);

#if 0
  printf("%6d:", line);
  vprintf(format, list);
  puts("");
#else
  va_list list2;
  va_start(list2, format);
  static char buffer_std[4096];
  static char buffer_npf[4096];
  vsnprintf(buffer_std, sizeof(buffer_std), format, list);
  npf_vsnprintf(buffer_npf, sizeof(buffer_npf), format, list2);
  if(strcmp(buffer_std, buffer_npf) != 0) {
    printf("!%d: |%s|%s|\n", line, buffer_npf, buffer_std);
  } else {
    //printf("*%d: |%s|%s|\n", line, buffer_npf, buffer_std);
  }
  va_end(list2);
#endif

  va_end(list);
}

#define CHK_ERR(...)    chk_err(__LINE__, __VA_ARGS__)

void chk_err(int line, const char *format, ...)
{
  // in this chk_err call, an error MUST occur, otherwise the printf function is misbehaving.

  va_list list;
  va_start(list, format);

  static char buffer[4096];
  memset(buffer, 0xFF, sizeof(buffer));
  int n = npf_vsnprintf(buffer, sizeof(buffer), format, list);
  if(n >= 0 || buffer[0] != '\0') {
    printf("!%d: %d |%.*s|\n", line, n, (int)sizeof(buffer), buffer);
  } else {
    //printf("*%d: %d\n", line, n);
  }

  va_end(list);
}

double npf_u64_to_dbl(uint64_t v)
{
    double d;
    memcpy(&d, &v, 8);
    return d;
}

int main(void)
{
  /*
  UB sources (and things in which NPF deviates from standard):
  0) NULL format string
  1) Unterminated '%'
  2) Unrecognized specifier, which might also appear after the '*' or '.*' field.
  This could lead to wrong accesses to the variadic arguments; however, NPF
  already defers the fetching of any argument, including width and precision,
  until after it has determined that the conversion specifier is correct; so,
  we have not performed any illegal access yet when we detect this problem.
  Note that we also trigger a UB response (if enabled) on modifiers/specifiers
  that are legal but we do not recognize with the current compilation options,
  since we cannot properly handle them.
  This case also includes the case of unrecognized size modifiers -- since the
  modifier is optional, any unknown one is parsed as "no modifier, followed by
  an unknown specifier".
  3) Duplicate flags. The standard describes which flags are ignored, in case
  conflicting flags are specified (or in case they conflict with the width, or
  the precision, or the actual value of the argument), but it is silent about
  duplicate flags.
  This case also includes a negative-value "*" field width -- the standard
  says that such a negative value is equivalent to a width equal to its
  absolute value, plus the '-' flag
  4) Duplicate field width, also in the form of both an asterisk and a
  non-negative number. A negative number is not an error in itself (the minus
  is not part of the width, it's the '-' flag), but this case can happen too:
  "%*-1i", in which the minus is not a flag since the flags all come before the
  width, which instead was 'started' by the asterisk.
  5) Overflowing field width. The standard is silent on this, and talks about
  "a nonnegative integer". It also says the following, not strictly related:
  "As noted above, a field width, or precision, or both, may be indicated by
  an asterisk. In this case, an int argument supplies the field width or precision"
  So, we assume that the literal width value must fit in an (int) too.
  6) Negative literal precision.
  7) Overflowing precision.
  8) Duplicate precision. See above for duplicate field width.
  9) Precision specified for any conversion specifier other than: b B d i o u x X a A e E f F g G s
  10) Duplicate length modifier.
  11) Invalid size modifier.
  hh h ll j z t are only valid for b B d i o u x X n
  l is valid for b B d i o u x X n s c  (but NPF does not support it for 's' or 'c')
  l is also valid (but ignored) for a A e E f F g G
  L is only valid for a A e E f F g G (but NPF does not support it if long double
    is larger than double -- it cast to double, causing rounding errors)
  w<N> wf<N> H D DD are not supported by NPF
  12) NPF does not support standard e E g G a A
  13) '\0' as the argument for %c
  14) NULL as the argument for %s
  15) NULL as the argument for %n
  16) %n can't have any flag, width, or precision
  17) the '0' flag is only valid for b B d i o u x X a A e E f F g G
  18) flags '+' or ' ' for c s p
  19) flag '+' is only specified for float or signed conversions. It's unclear
  if it is UB (or to be ignored) for unsigned conversions

  Conditions that we do not check:
  - Unterminated format string (overrun)
  - Non-ASCII characters, which might or might not be legal. The standard says:
  "The format shall be a multibyte character sequence, beginning and ending in its initial shift state"
  - '%f' and '%F': they are always non-conforming due to wrong rounding. This is
  a deliberate choice to have a smaller footprint for NPF, so it is considered
  "correct" for us.
  - invalid pointer passed as argument for %n (we only check for NULL)
  - arguments inconsistent with the format string
  - wilder things, like aliased format string or arguments (possibly changing
  their content while the NPF call is executing)
  */
  int p = -1;
  int32_t i32 = 0;
  int_fast32_t if32 = 0;
  // 0
  CHK_ERR(NULL);
  // 1
  CHK_ERR("%");
  CHK_ERR("%0");
  CHK_ERR("%-");
  CHK_ERR("%+");
  CHK_ERR("% ");
  CHK_ERR("%#");
  CHK_ERR("%1");
  CHK_ERR("%*");
  CHK_ERR("%.");
  CHK_ERR("%.1");
  CHK_ERR("%.*");
  CHK_ERR("%h");
  CHK_ERR("%hh");
  CHK_ERR("%l");
  CHK_ERR("%ll");
  CHK_ERR("%L");
  CHK_ERR("%j");
  CHK_ERR("%z");
  CHK_ERR("%t");
  // 2
  CHK_ERR("%k");
  CHK_ERR("%m");
  CHK_ERR("%Q");
  CHK_ERR("% 05.3LZ");
  CHK_ERR("%+-#*.*hhZ");
  // 3
  CHK_ERR("%--i", 1);
  CHK_ERR("%++i", 1);
  CHK_ERR("%  i", 1);
  CHK_ERR("%00i", 1);
  CHK_ERR("%##i", 1);
  CHK_ERR("%-*i", -1, 1);
  CHK_ERR("%--u", 1u);
  CHK_ERR("%++u", 1u);
  CHK_ERR("%  u", 1u);
  CHK_ERR("%00u", 1u);
  CHK_ERR("%##u", 1u);
  CHK_ERR("%-*u", -1, 1u);
  CHK_ERR("%--f", 1.0);
  CHK_ERR("%++f", 1.0);
  CHK_ERR("%  f", 1.0);
  CHK_ERR("%00f", 1.0);
  CHK_ERR("%##f", 1.0);
  CHK_ERR("%-*f", -1, 1.0);
  CHK_ERR("%--s", "abc");
  CHK_ERR("%++s", "abc");
  CHK_ERR("%  s", "abc");
  CHK_ERR("%00s", "abc");
  CHK_ERR("%##s", "abc");
  CHK_ERR("%-*s", -1, "abc");
  // 4
  CHK_ERR("%**i", 1);
  CHK_ERR("%*4i", 1);
  CHK_ERR("%*-4i", 1);
  CHK_ERR("%4*i", 1);
  CHK_ERR("%4-4i", 1);
  // 5
  CHK_ERR("%9223372036854775808i", 1); // this overflows even on theoretical systems with 64-bit ints
  // 6
  CHK_ERR("%.-4i", 1);
  CHK_ERR("%.-4u", 1u);
  CHK_ERR("%.-4f", 1.0);
  CHK_ERR("%.-4s", "abc");
  // 7
  CHK_ERR("%.9223372036854775808i", 1); // this overflows even on theoretical systems with 64-bit ints
  // 8
  CHK_ERR("%.*.*i", 1);
  CHK_ERR("%.*.4i", 1);
  CHK_ERR("%.4.*i", 1);
  CHK_ERR("%.4.4i", 1);
  // 9
  CHK_ERR("%.2%");
  CHK_ERR("%.2c", 'a');
  CHK_ERR("%.*c", 2, 'a');
  CHK_ERR("%.2n", &p);
  CHK_ERR("%.*n", 2, &p);
  CHK_ERR("%.2p", &p);
  CHK_ERR("%.*p", 2, &p);
  // 10
  CHK_ERR("%hhhi", 1);
  CHK_ERR("%llli", 1);
  CHK_ERR("%jji", 1);
  CHK_ERR("%zzi", 1);
  CHK_ERR("%tti", 1);
  CHK_ERR("%LLf", 1.0);
  CHK_ERR("%Lhf", 1.0);
  CHK_ERR("%jzi", 1);
  CHK_ERR("%lhi", 1);
  CHK_ERR("%tji", 1);
  // 11
  CHK_ERR(  "%hh%");
  CHK_ERR(   "%h%");
  CHK_ERR(   "%l%");
  CHK_ERR(  "%ll%");
  CHK_ERR(   "%j%");
  CHK_ERR(   "%z%");
  CHK_ERR(   "%t%");
  CHK_ERR(   "%L%");
  CHK_ERR(   "%H%");
  CHK_ERR(   "%D%");
  CHK_ERR(  "%DD%");
  CHK_ERR( "%w32%");
  CHK_ERR("%wf32%");

  CHK_ERR(  "%hhc", 'a');
  CHK_ERR(   "%hc", 'a');
  CHK_ERR(   "%lc", L'a');
  CHK_ERR(  "%llc", 'a');
  CHK_ERR(   "%jc", 'a');
  CHK_ERR(   "%zc", 'a');
  CHK_ERR(   "%tc", 'a');
  CHK_ERR(   "%Lc", 'a');
  CHK_ERR(   "%Hc", 'a');
  CHK_ERR(   "%Dc", 'a');
  CHK_ERR(  "%DDc", 'a');
  CHK_ERR( "%w32c", 'a');
  CHK_ERR("%wf32c", 'a');

  CHK_ERR(  "%hhs", "abc");
  CHK_ERR(   "%hs", "abc");
  CHK_ERR(   "%ls", L"abc");
  CHK_ERR(  "%lls", "abc");
  CHK_ERR(   "%js", "abc");
  CHK_ERR(   "%zs", "abc");
  CHK_ERR(   "%ts", "abc");
  CHK_ERR(   "%Ls", "abc");
  CHK_ERR(   "%Hs", "abc");
  CHK_ERR(   "%Ds", "abc");
  CHK_ERR(  "%DDs", "abc");
  CHK_ERR( "%w32s", "abc");
  CHK_ERR("%wf32s", "abc");

  CHK_ERR(   "%Ld", 1);
  CHK_ERR(   "%Li", 1);
  CHK_ERR(   "%Lu", 1);
  CHK_ERR(   "%Lb", 1);
  CHK_ERR(   "%LB", 1);
  CHK_ERR(   "%Lo", 1);
  CHK_ERR(   "%Lx", 1);
  CHK_ERR(   "%LX", 1);

  CHK_ERR(  "%hhf", 1.0);
  CHK_ERR(   "%hf", 1.0);
  CHK_ERR(  "%llf", 1.0);
  CHK_ERR(   "%jf", 1.0);
  CHK_ERR(   "%zf", 1.0);
  CHK_ERR(   "%tf", 1.0);
  CHK_ERR(   "%Hf", 1.0); // should actually be a _Decimal32 argument
  CHK_ERR(   "%Df", 1.0); // should actually be a _Decimal64 argument
  CHK_ERR(  "%DDf", 1.0); // should actually be a _Decimal128 argument
  CHK_ERR( "%w32f", 1.0);
  CHK_ERR("%wf32f", 1.0);

  CHK_ERR(  "%hhF", 1.0);
  CHK_ERR(   "%hF", 1.0);
  CHK_ERR(  "%llF", 1.0);
  CHK_ERR(   "%jF", 1.0);
  CHK_ERR(   "%zF", 1.0);
  CHK_ERR(   "%tF", 1.0);
  CHK_ERR(   "%HF", 1.0); // should actually be a _Decimal32 argument
  CHK_ERR(   "%DF", 1.0); // should actually be a _Decimal64 argument
  CHK_ERR(  "%DDF", 1.0); // should actually be a _Decimal128 argument
  CHK_ERR( "%w32F", 1.0);
  CHK_ERR("%wf32F", 1.0);

  CHK_ERR(  "%hhe", 1.0);
  CHK_ERR(   "%he", 1.0);
  CHK_ERR(  "%lle", 1.0);
  CHK_ERR(   "%je", 1.0);
  CHK_ERR(   "%ze", 1.0);
  CHK_ERR(   "%te", 1.0);
  CHK_ERR(   "%He", 1.0); // should actually be a _Decimal32 argument
  CHK_ERR(   "%De", 1.0); // should actually be a _Decimal64 argument
  CHK_ERR(  "%DDe", 1.0); // should actually be a _Decimal128 argument
  CHK_ERR( "%w32e", 1.0);
  CHK_ERR("%wf32e", 1.0);

  CHK_ERR(  "%hhE", 1.0);
  CHK_ERR(   "%hE", 1.0);
  CHK_ERR(  "%llE", 1.0);
  CHK_ERR(   "%jE", 1.0);
  CHK_ERR(   "%zE", 1.0);
  CHK_ERR(   "%tE", 1.0);
  CHK_ERR(   "%HE", 1.0); // should actually be a _Decimal32 argument
  CHK_ERR(   "%DE", 1.0); // should actually be a _Decimal64 argument
  CHK_ERR(  "%DDE", 1.0); // should actually be a _Decimal128 argument
  CHK_ERR( "%w32E", 1.0);
  CHK_ERR("%wf32E", 1.0);

  CHK_ERR(  "%hhg", 1.0);
  CHK_ERR(   "%hg", 1.0);
  CHK_ERR(  "%llg", 1.0);
  CHK_ERR(   "%jg", 1.0);
  CHK_ERR(   "%zg", 1.0);
  CHK_ERR(   "%tg", 1.0);
  CHK_ERR(   "%Hg", 1.0); // should actually be a _Decimal32 argument
  CHK_ERR(   "%Dg", 1.0); // should actually be a _Decimal64 argument
  CHK_ERR(  "%DDg", 1.0); // should actually be a _Decimal128 argument
  CHK_ERR( "%w32g", 1.0);
  CHK_ERR("%wf32g", 1.0);

  CHK_ERR(  "%hhG", 1.0);
  CHK_ERR(   "%hG", 1.0);
  CHK_ERR(  "%llG", 1.0);
  CHK_ERR(   "%jG", 1.0);
  CHK_ERR(   "%zG", 1.0);
  CHK_ERR(   "%tG", 1.0);
  CHK_ERR(   "%HG", 1.0); // should actually be a _Decimal32 argument
  CHK_ERR(   "%DG", 1.0); // should actually be a _Decimal64 argument
  CHK_ERR(  "%DDG", 1.0); // should actually be a _Decimal128 argument
  CHK_ERR( "%w32G", 1.0);
  CHK_ERR("%wf32G", 1.0);

  CHK_ERR(  "%hha", 1.0);
  CHK_ERR(   "%ha", 1.0);
  CHK_ERR(  "%lla", 1.0);
  CHK_ERR(   "%ja", 1.0);
  CHK_ERR(   "%za", 1.0);
  CHK_ERR(   "%ta", 1.0);
  CHK_ERR(   "%Ha", 1.0); // should actually be a _Decimal32 argument
  CHK_ERR(   "%Da", 1.0); // should actually be a _Decimal64 argument
  CHK_ERR(  "%DDa", 1.0); // should actually be a _Decimal128 argument
  CHK_ERR( "%w32a", 1.0);
  CHK_ERR("%wf32a", 1.0);

  CHK_ERR(  "%hhA", 1.0);
  CHK_ERR(   "%hA", 1.0);
  CHK_ERR(  "%llA", 1.0);
  CHK_ERR(   "%jA", 1.0);
  CHK_ERR(   "%zA", 1.0);
  CHK_ERR(   "%tA", 1.0);
  CHK_ERR(   "%HA", 1.0); // should actually be a _Decimal32 argument
  CHK_ERR(   "%DA", 1.0); // should actually be a _Decimal64 argument
  CHK_ERR(  "%DDA", 1.0); // should actually be a _Decimal128 argument
  CHK_ERR( "%w32A", 1.0);
  CHK_ERR("%wf32A", 1.0);

  CHK_ERR(  "%hhp", &p);
  CHK_ERR(   "%hp", &p);
  CHK_ERR(   "%lp", &p);
  CHK_ERR(  "%llp", &p);
  CHK_ERR(   "%jp", &p);
  CHK_ERR(   "%zp", &p);
  CHK_ERR(   "%tp", &p);
  CHK_ERR(   "%Lp", &p);
  CHK_ERR(   "%Hp", &p);
  CHK_ERR(   "%Dp", &p);
  CHK_ERR(  "%DDp", &p);
  CHK_ERR( "%w32p", &p);
  CHK_ERR("%wf32p", &p);

  if(sizeof(long double) > sizeof(double)) {
    CHK_ERR(  "%Lf", 1.0L);
    CHK_ERR(  "%LF", 1.0L);
    CHK_ERR(  "%Le", 1.0L);
    CHK_ERR(  "%LE", 1.0L);
    CHK_ERR(  "%Lg", 1.0L);
    CHK_ERR(  "%LG", 1.0L);
    CHK_ERR(  "%La", 1.0L);
    CHK_ERR(  "%LA", 1.0L);
  }

  CHK_ERR("%w32d", i32);
  CHK_ERR("%w32i", i32);
  CHK_ERR("%w32o", i32);
  CHK_ERR("%w32u", i32);
  CHK_ERR("%w32x", i32);
  CHK_ERR("%w32X", i32);
  CHK_ERR("%w32b", i32);
  CHK_ERR("%w32B", i32);
  CHK_ERR("%w32n", &i32);
  CHK_ERR("%wf32d", if32);
  CHK_ERR("%wf32i", if32);
  CHK_ERR("%wf32o", if32);
  CHK_ERR("%wf32u", if32);
  CHK_ERR("%wf32x", if32);
  CHK_ERR("%wf32X", if32);
  CHK_ERR("%wf32b", if32);
  CHK_ERR("%wf32B", if32);
  CHK_ERR("%wf32n", &if32);

  // 12
  CHK_ERR("%e", 1.0);
  CHK_ERR("%E", 1.0);
  CHK_ERR("%g", 1.0);
  CHK_ERR("%G", 1.0);
  CHK_ERR("%a", 1.0);
  CHK_ERR("%A", 1.0);

  // 13
  CHK_ERR("%c xyz", '\0');

  // 14
  CHK_ERR("%s", NULL);

  // 15
  CHK_ERR("%n", NULL);

  // 16
  CHK_ERR("%+n", p);
  CHK_ERR("%-n", p);
  CHK_ERR("% n", p);
  CHK_ERR("%#n", p);
  CHK_ERR("%0n", p);
  CHK_ERR("%1n", p);
  CHK_ERR("%*n", 1, p);
  CHK_ERR("%.1n", p);
  CHK_ERR("%.*n", 1, p);

  // 17
  CHK_ERR("%0c", 'a');
  CHK_ERR("%0s", "abc");
  CHK_ERR("%0n", &p);

  // 18
  CHK_ERR("%+c", 'a');
  CHK_ERR("% c", 'a');
  CHK_ERR("%+s", "abc");
  CHK_ERR("% s", "abc");
  CHK_ERR("%+p", &p);
  CHK_ERR("% p", &p);

  // 19
  CHK_ERR("%+b", 0);
  CHK_ERR("%+o", 0);
  CHK_ERR("%+u", 0);
  CHK_ERR("%+x", 0);

  //
  // Positive checks
  //

  // C99+: "l" must be accepted (and ignored) for f F e E g G a A
  PR("%lf", 1.234);
  PR("%lF", 1.234);

  return 0;
}
