/*
  Conformance tests for nanoprintf.
  Merged from conformance.cc and mpaland-conformance/paland.cc.

  This file is compiled once per flag combination, both as C and as C++.
  The test function name is injected via -DNPF_TEST_FUNC=npf_test_combo_NNN.
  NANOPRINTF_VISIBILITY_STATIC keeps all symbols file-local so all .o files link.
*/

#define NANOPRINTF_VISIBILITY_STATIC
#define NANOPRINTF_IMPLEMENTATION
#include "../nanoprintf.h"

#include "test_harness.h"

#include <limits.h>
#include <stdint.h>
#include <math.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

int NPF_TEST_PASS_COUNT;

int NPF_TEST_FUNC(void) {
    npf_test_pass_count = 0;
    npf_test_fail_count = 0;

    /* ===== percent ===== */
    NPF_TEST("%", "%%");
    NPF_TEST("%%", "%%%%");
#if NANOPRINTF_USE_FIELD_WIDTH_FORMAT_SPECIFIERS == 1
    NPF_TEST("%", "%-%");
#endif
    NPF_TEST("%", "% %");
    NPF_TEST("%", "%+%");
#if NANOPRINTF_USE_ALT_FORM_FLAG == 1
    NPF_TEST("%", "%#%");
#endif

    /* ===== snprintf return value ===== */
    NPF_TEST_RET(0, "");
    NPF_TEST_RET(5, "hello");
    NPF_TEST_RET(1, "%%");
    NPF_TEST_RET(2, "%%%%");
    NPF_TEST_RET(5, "%s", "hello");
    NPF_TEST_RET(0, "%s", "");
    NPF_TEST_RET(3, "%d", 100);
    NPF_TEST_RET(4, "%d", -100);
    NPF_TEST_RET(1, "%c", 'A');
    /* truncation: return value is would-be length, not bytes written */
    { char buf[4];
      NPF_TEST_WB(5, npf_snprintf(buf, sizeof(buf), "hello")); }
    { char buf[4];
      NPF_TEST_WB(5, npf_snprintf(buf, sizeof(buf), "%s", "hello")); }
    { char buf[1];
      NPF_TEST_WB(5, npf_snprintf(buf, sizeof(buf), "hello")); }
    /* size 0: returns would-be length, writes nothing */
    { char buf[1]; buf[0] = 'X';
      NPF_TEST_WB(5, npf_snprintf(buf, 0, "hello"));
      NPF_TEST_WB('X', buf[0]); }
    /* NULL buf with size 0: length-only calculation */
    NPF_TEST_WB(5, npf_snprintf((char *)0, 0, "hello"));
    NPF_TEST_WB(0, npf_snprintf((char *)0, 0, ""));
    NPF_TEST_WB(3, npf_snprintf((char *)0, 0, "%d", 100));

    /* ===== char ===== */
    /* every char except the NUL char */
    { int i;
      for (i = 1; i < CHAR_MAX; ++i) {
          char output[2];
          output[0] = (char)i;
          output[1] = 0;
          NPF_TEST_DYN(output, "%c", i);
      }
    }

    /* NUL char: %c with '\0' must return 1, not 0 */
    NPF_TEST_RET(1, "%c", 0);

    NPF_TEST("A", "%+c", 'A');

#if NANOPRINTF_USE_FIELD_WIDTH_FORMAT_SPECIFIERS == 1
    /* right justify field width */
    NPF_TEST("A", "%1c", 'A');
    NPF_TEST(" A", "%2c", 'A');
    NPF_TEST("  A", "%3c", 'A');

    /* left justify field width */
    NPF_TEST("A", "%-1c", 'A');
    NPF_TEST("A ", "%-2c", 'A');
    NPF_TEST("A  ", "%-3c", 'A');

    NPF_TEST("     A", "% 6c", 'A');
    NPF_TEST("   A", "%+4c", 'A');
#endif

    /* ===== string ===== */
    NPF_TEST("Hello testing", "Hello testing");
    NPF_TEST("", "%s", "");
    NPF_TEST("one", "%s", "one");
    NPF_TEST("onetwothree", "%s%s%s", "one", "two", "three");
    NPF_TEST("Hello testing", "%s", "Hello testing");
    NPF_TEST("A Test", "%s", "A Test");

#if NANOPRINTF_USE_FIELD_WIDTH_FORMAT_SPECIFIERS == 1
    NPF_TEST("       two", "%10s", "two");
    NPF_TEST("B---       E", "B%-10sE", "---");
    NPF_TEST("B       ---E", "B%10sE", "---");
    NPF_TEST("Hello testing", "%1s", "Hello testing");
    NPF_TEST("               Hello", "%20s", "Hello");
    NPF_TEST("Hello               ", "%-20s", "Hello");
    NPF_TEST("Hello               ", "%0-20s", "Hello");
#endif

#if NANOPRINTF_USE_PRECISION_FORMAT_SPECIFIERS == 1
    NPF_TEST("thr", "%.3s", "three");
    NPF_TEST("four", "%.100s", "four");
    NPF_TEST("", "%.0s", "five");
    NPF_TEST("This", "%.4s", "This is a test");
    NPF_TEST("test", "%.4s", "test");
    NPF_TEST("123", "%.7s", "123");
    NPF_TEST("", "%.7s", "");
    NPF_TEST("1234ab", "%.4s%.2s", "123456", "abcdef");
    NPF_TEST("123", "%.*s", 3, "123456");
    NPF_TEST("foo", "%.3s", "foobar");
#endif

#if (NANOPRINTF_USE_FIELD_WIDTH_FORMAT_SPECIFIERS == 1) && \
    (NANOPRINTF_USE_PRECISION_FORMAT_SPECIFIERS == 1)
    NPF_TEST("       six", "%10.3s", "sixAAAAAAAA");
    NPF_TEST("", "%.0s", "Hello testing");
    NPF_TEST("                    ", "%20.0s", "Hello testing");
    NPF_TEST("", "%.s", "Hello testing");
    NPF_TEST("                    ", "%20.s", "Hello testing");
#endif

    /* non-standard: malformed precision */
    NPF_TEST("%.4.2s", "%.4.2s", "123456");

    /* ===== signed int / %d / %i ===== */
    NPF_TEST("-2147483648", "%i", INT_MIN);
    NPF_TEST("0", "%i", 0);
    NPF_TEST("2147483647", "%i", INT_MAX);
    NPF_TEST("-1", "%+i", -1);
    NPF_TEST("+0", "%+i", 0);
    NPF_TEST("+1", "%+i", 1);
    NPF_TEST("1024", "%d", 1024);
    NPF_TEST("-1024", "%d", -1024);
    NPF_TEST("1024", "%i", 1024);
    NPF_TEST("-1024", "%i", -1024);
    NPF_TEST("0", "%i", 0);
    NPF_TEST("1234", "%i", 1234);
    NPF_TEST("32767", "%i", 32767);
    NPF_TEST("-32767", "%i", -32767);
    NPF_TEST("30", "%li", 30L);
    NPF_TEST("-2147483647", "%li", -2147483647L);
    NPF_TEST("2147483647", "%li", 2147483647L);

    /* space flag on signed */
    NPF_TEST(" 42", "% d", 42);
    NPF_TEST("-42", "% d", -42);
    NPF_TEST(" 1024", "% d", 1024);
    NPF_TEST("-1024", "% d", -1024);
    NPF_TEST(" 1024", "% i", 1024);
    NPF_TEST("-1024", "% i", -1024);
    NPF_TEST(" 0", "% d", 0);
    NPF_TEST(" 0", "% i", 0);

    /* + flag on signed */
    NPF_TEST("+42", "%+d", 42);
    NPF_TEST("-42", "%+d", -42);
    NPF_TEST("+1024", "%+d", 1024);
    NPF_TEST("-1024", "%+d", -1024);
    NPF_TEST("+1024", "%+i", 1024);
    NPF_TEST("-1024", "%+i", -1024);

    /* + overrides space (C standard) */
    NPF_TEST("+42", "%+ d", 42);
    NPF_TEST("+42", "% +d", 42);
    NPF_TEST("-42", "%+ d", -42);
    NPF_TEST("+0", "%+ d", 0);

#if NANOPRINTF_USE_SMALL_FORMAT_SPECIFIERS == 1
    NPF_TEST("-128", "%hhi", 128);
    NPF_TEST("0", "%hhd", 256);
    NPF_TEST("-1", "%hhd", 255);
    NPF_TEST("127", "%hhd", 127);
    NPF_TEST_SYS("%hhd", CHAR_MAX);
    NPF_TEST_SYS("%hhd", CHAR_MIN);
    NPF_TEST_SYS("%hd", SHRT_MAX);
    NPF_TEST_SYS("%hd", SHRT_MIN);
#endif

#if NANOPRINTF_USE_FIELD_WIDTH_FORMAT_SPECIFIERS == 1
    NPF_TEST("  -1", "% 4i", -1);
    NPF_TEST("   0", "% 4i", 0);
    NPF_TEST("   1", "% 4i", 1);
    NPF_TEST("  +1", "%+4i", 1);
    NPF_TEST("  +0", "%+4i", 0);
    NPF_TEST("  -1", "%+4i", -1);
    NPF_TEST("0001", "%04i", 1);
    NPF_TEST("0000", "%04i", 0);
    NPF_TEST("-001", "%04i", -1);
    NPF_TEST("+001", "%+04i", 1);

    /* space flag with field width */
    NPF_TEST("   42", "% 5d", 42);
    NPF_TEST("  -42", "% 5d", -42);
    NPF_TEST("             42", "% 15d", 42);
    NPF_TEST("            -42", "% 15d", -42);

    /* + flag with field width */
    NPF_TEST("  +42", "%+5d", 42);
    NPF_TEST("  -42", "%+5d", -42);
    NPF_TEST("            +42", "%+15d", 42);
    NPF_TEST("            -42", "%+15d", -42);

    /* 0 flag */
    NPF_TEST("42", "%0d", 42);
    NPF_TEST("42", "%0ld", 42L);
    NPF_TEST("-42", "%0d", -42);
    NPF_TEST("00042", "%05d", 42);
    NPF_TEST("-0042", "%05d", -42);
    NPF_TEST("000000000000042", "%015d", 42);
    NPF_TEST("-00000000000042", "%015d", -42);

    /* - flag */
    NPF_TEST("42", "%-d", 42);
    NPF_TEST("-42", "%-d", -42);
    NPF_TEST("42   ", "%-5d", 42);
    NPF_TEST("-42  ", "%-5d", -42);
    NPF_TEST("42             ", "%-15d", 42);
    NPF_TEST("-42            ", "%-15d", -42);

    /* - flag and non-standard 0 modifier */
    NPF_TEST("42", "%-0d", 42);
    NPF_TEST("-42", "%-0d", -42);
    NPF_TEST("42   ", "%-05d", 42);
    NPF_TEST("-42  ", "%-05d", -42);
    NPF_TEST("42             ", "%-015d", 42);
    NPF_TEST("-42            ", "%-015d", -42);
    NPF_TEST("42", "%0-d", 42);
    NPF_TEST("-42", "%0-d", -42);
    NPF_TEST("42   ", "%0-5d", 42);
    NPF_TEST("-42  ", "%0-5d", -42);
    NPF_TEST("42             ", "%0-15d", 42);
    NPF_TEST("-42            ", "%0-15d", -42);

    /* width */
    NPF_TEST("1024", "%1d", 1024);
    NPF_TEST("-1024", "%1d", -1024);
    NPF_TEST("1024", "%1i", 1024);
    NPF_TEST("-1024", "%1i", -1024);

    /* width 20 */
    NPF_TEST("                1024", "%20d", 1024);
    NPF_TEST("               -1024", "%20d", -1024);
    NPF_TEST("                1024", "%20i", 1024);
    NPF_TEST("               -1024", "%20i", -1024);
    NPF_TEST("                   0", "%20i", 0);

    /* width *20 */
    NPF_TEST("                1024", "%*d", 20, 1024);
    NPF_TEST("               -1024", "%*d", 20, -1024);
    NPF_TEST("                1024", "%*i", 20, 1024);
    NPF_TEST("               -1024", "%*i", 20, -1024);

    /* width -20 */
    NPF_TEST("1024                ", "%-20d", 1024);
    NPF_TEST("-1024               ", "%-20d", -1024);
    NPF_TEST("1024                ", "%-20i", 1024);
    NPF_TEST("-1024               ", "%-20i", -1024);

    /* width 0-20 */
    NPF_TEST("1024                ", "%0-20d", 1024);
    NPF_TEST("-1024               ", "%0-20d", -1024);
    NPF_TEST("1024                ", "%0-20i", 1024);
    NPF_TEST("-1024               ", "%0-20i", -1024);

    /* padding 20 */
    NPF_TEST("00000000000000001024", "%020d", 1024);
    NPF_TEST("-0000000000000001024", "%020d", -1024);
    NPF_TEST("00000000000000001024", "%020i", 1024);
    NPF_TEST("-0000000000000001024", "%020i", -1024);

    /* misc width/alignment */
    NPF_TEST("|    9| |9 | |    9|", "|%5d| |%-2d| |%5d|", 9, 9, 9);
    NPF_TEST("|   10| |10| |   10|", "|%5d| |%-2d| |%5d|", 10, 10, 10);
    NPF_TEST("|    9| |9           | |    9|", "|%5d| |%-12d| |%5d|", 9, 9, 9);
    NPF_TEST("|   10| |10          | |   10|", "|%5d| |%-12d| |%5d|", 10, 10, 10);

    /* padding neg numbers */
    NPF_TEST("-5", "% 1d", -5);
    NPF_TEST("-5", "% 2d", -5);
    NPF_TEST(" -5", "% 3d", -5);
    NPF_TEST("  -5", "% 4d", -5);
    NPF_TEST("-5", "%01d", -5);
    NPF_TEST("-5", "%02d", -5);
    NPF_TEST("-05", "%03d", -5);
    NPF_TEST("-005", "%04d", -5);

#if NANOPRINTF_USE_ALT_FORM_FLAG == 1
    /* # flag non-standard on d/i/u */
    NPF_TEST("00000000000000001024", "%#020d", 1024);
    NPF_TEST("-0000000000000001024", "%#020d", -1024);
    NPF_TEST("00000000000000001024", "%#020i", 1024);
    NPF_TEST("-0000000000000001024", "%#020i", -1024);
    NPF_TEST("                1024", "%#20d", 1024);
    NPF_TEST("               -1024", "%#20d", -1024);
    NPF_TEST("                1024", "%#20i", 1024);
    NPF_TEST("               -1024", "%#20i", -1024);
#endif
#endif /* NANOPRINTF_USE_FIELD_WIDTH_FORMAT_SPECIFIERS */

#if NANOPRINTF_USE_PRECISION_FORMAT_SPECIFIERS == 1
    NPF_TEST("", "%.0i", 0);
    NPF_TEST("+", "%+.0i", 0);
    NPF_TEST("+01", "%+.2i", 1);
    NPF_TEST("+", "%+.0d", 0);
    NPF_TEST("", "%.d", 0);
    NPF_TEST("", "%.i", 0);
    NPF_TEST("1", "%.d", 1);
    NPF_TEST("00042", "%.5d", 42);
    NPF_TEST("-00042", "%.5d", -42);
    NPF_TEST("00042", "%.5i", 42);
    NPF_TEST("-00042", "%.5i", -42);
#endif

#if (NANOPRINTF_USE_FIELD_WIDTH_FORMAT_SPECIFIERS == 1) && \
    (NANOPRINTF_USE_PRECISION_FORMAT_SPECIFIERS == 1)
    NPF_TEST(" +01", "%+4.2i", 1);
    NPF_TEST(" 0", "%02.1d", 0);

    /* precision overrides 0 flag for integers */
    NPF_TEST("  042", "%05.3d", 42);
    NPF_TEST(" -042", "%05.3d", -42);
    NPF_TEST("  042", "%05.3i", 42);
    NPF_TEST(" -042", "%05.3i", -42);

    /* padding 20.5 */
    NPF_TEST("               01024", "%20.5d", 1024);
    NPF_TEST("              -01024", "%20.5d", -1024);
    NPF_TEST("               01024", "%20.5i", 1024);
    NPF_TEST("              -01024", "%20.5i", -1024);

    /* padding .20 */
    NPF_TEST("00000000000000001024", "%.20d", 1024);
    NPF_TEST("-00000000000000001024", "%.20d", -1024);
    NPF_TEST("00000000000000001024", "%.20i", 1024);
    NPF_TEST("-00000000000000001024", "%.20i", -1024);

    /* length tests */
    NPF_TEST("                1024", "%20.0d", 1024);
    NPF_TEST("               -1024", "%20.0d", -1024);
    NPF_TEST("                    ", "%20.d", 0);
    NPF_TEST("                1024", "%20.0i", 1024);
    NPF_TEST("               -1024", "%20.i", -1024);
    NPF_TEST("                    ", "%20.i", 0);
    NPF_TEST("     00004", "%10.5d", 4);
    NPF_TEST("00123               ", "%-20.5i", 123);

    /* non-standard */
    NPF_TEST("  ", "%02.0d", 0);
#endif

#if NANOPRINTF_USE_PRECISION_FORMAT_SPECIFIERS == 1
    NPF_TEST(" ", "% .0d", 0);
    NPF_TEST("1", "%.*d", -1, 1);
#endif

    /* extremal signed integer values */
    NPF_TEST_SYS("%d", INT_MIN);
    NPF_TEST_SYS("%d", INT_MAX);
    NPF_TEST_SYS("%ld", LONG_MIN);
    NPF_TEST_SYS("%ld", LONG_MAX);

#if (NANOPRINTF_USE_LARGE_FORMAT_SPECIFIERS == 1)
#if LLONG_MAX == 9223372036854775807ll
    NPF_TEST("9223372036854775807", "%lli", LLONG_MAX);
#else
    NPF_TEST("2147483647", "%lli", LLONG_MAX);
#endif

#if INTMAX_MAX == 9223372036854775807ll
    NPF_TEST("9223372036854775807", "%ji", INTMAX_MAX);
#else
    NPF_TEST("2147483647", "%ji", INTMAX_MAX);
#endif

/* SSIZE_MAX is POSIX, not C standard. Provide a fallback. */
#ifndef SSIZE_MAX
  #ifdef _MSC_VER
    #define SSIZE_MAX LONG_MAX
  #elif SIZE_MAX == 0xffffffffu
    #define SSIZE_MAX 2147483647
  #else
    #define SSIZE_MAX 9223372036854775807ll
  #endif
#endif

#if SSIZE_MAX == 2147483647
    NPF_TEST("2147483647", "%zi", SSIZE_MAX);
#else
    NPF_TEST("9223372036854775807", "%zi", SSIZE_MAX);
#endif

#if PTRDIFF_MAX == 9223372036854775807ll
    NPF_TEST("9223372036854775807", "%ti", PTRDIFF_MAX);
#else
    NPF_TEST("2147483647", "%ti", PTRDIFF_MAX);
#endif

    NPF_TEST("30", "%lli", 30LL);
    NPF_TEST("-9223372036854775807", "%lli", -9223372036854775807LL);
    NPF_TEST("9223372036854775807", "%lli", 9223372036854775807LL);
    NPF_TEST("-2147483647", "%ji", (intmax_t)-2147483647L);
    NPF_TEST_SYS("%lld", LLONG_MIN);
    NPF_TEST_SYS("%lld", LLONG_MAX);
#endif /* NANOPRINTF_USE_LARGE_FORMAT_SPECIFIERS */

    /* ===== unsigned int ===== */
    NPF_TEST("0", "%u", 0);
    NPF_TEST("4294967295", "%u", UINT_MAX);
    NPF_TEST("0", "%+u", 0);
    NPF_TEST("1", "%+u", 1);
    NPF_TEST("1024", "%u", 1024);
    NPF_TEST("4294966272", "%u", 4294966272U);
    NPF_TEST("0", "%lu", 0UL);
    NPF_TEST("100000", "%lu", 100000L);
    NPF_TEST("4294967295", "%lu", 0xFFFFFFFFL);

    /* space/plus flag non-standard on unsigned */
    NPF_TEST("1024", "% u", 1024);
    NPF_TEST("4294966272", "% u", 4294966272U);
    NPF_TEST("1024", "%+u", 1024);
    NPF_TEST("4294966272", "%+u", 4294966272U);

#if NANOPRINTF_USE_SMALL_FORMAT_SPECIFIERS == 1
    NPF_TEST("0", "%hhu", 256u);
    NPF_TEST("13", "%hu", (1 << 21u) + 13u);
    NPF_TEST("255", "%hhu", (unsigned char)0xFFU);
    NPF_TEST("4660", "%hu", (unsigned short)0x1234u);
    NPF_TEST_SYS("%hhu", (unsigned char)UCHAR_MAX);
    NPF_TEST_SYS("%hu", (unsigned short)USHRT_MAX);
#endif

#if ULONG_MAX > UINT_MAX
    NPF_TEST("4294967296", "%lu", (unsigned long)UINT_MAX + 1u);
#endif

#if NANOPRINTF_USE_FIELD_WIDTH_FORMAT_SPECIFIERS == 1
    NPF_TEST("   1", "%+4u", 1);
    NPF_TEST("     0", "% 6u", 0);

    /* width */
    NPF_TEST("1024", "%1u", 1024);
    NPF_TEST("4294966272", "%1u", 4294966272U);

    /* width 20 */
    NPF_TEST("                1024", "%20u", 1024);
    NPF_TEST("          4294966272", "%20u", 4294966272U);

    /* width *20 */
    NPF_TEST("                1024", "%*u", 20, 1024);
    NPF_TEST("          4294966272", "%*u", 20, 4294966272U);

    /* width -20 */
    NPF_TEST("1024                ", "%-20u", 1024);
    NPF_TEST("4294966272          ", "%-20u", 4294966272U);

    /* width 0-20 */
    NPF_TEST("1024                ", "%0-20u", 1024);
    NPF_TEST("4294966272          ", "%0-20u", 4294966272U);

    /* padding 20 */
    NPF_TEST("00000000000000001024", "%020u", 1024);
    NPF_TEST("00000000004294966272", "%020u", 4294966272U);

#if NANOPRINTF_USE_ALT_FORM_FLAG == 1
    NPF_TEST("00000000000000001024", "%#020u", 1024);
    NPF_TEST("00000000004294966272", "%#020u", 4294966272U);
    NPF_TEST("                1024", "%#20u", 1024);
    NPF_TEST("          4294966272", "%#20u", 4294966272U);
#endif
#endif /* NANOPRINTF_USE_FIELD_WIDTH_FORMAT_SPECIFIERS */

#if NANOPRINTF_USE_PRECISION_FORMAT_SPECIFIERS == 1
    NPF_TEST("", "%.0u", 0);
    NPF_TEST("", "%.u", 0);
    NPF_TEST("01", "%.2u", 1);
    NPF_TEST("00042", "%.5u", 42);

    /* padding .20 */
    NPF_TEST("00000000000000001024", "%.20u", 1024);
    NPF_TEST("00000000004294966272", "%.20u", 4294966272U);
#endif

#if (NANOPRINTF_USE_FIELD_WIDTH_FORMAT_SPECIFIERS == 1) && \
    (NANOPRINTF_USE_PRECISION_FORMAT_SPECIFIERS == 1)
    NPF_TEST("    0123", "%8.4u", 123);
    /* precision overrides 0 flag for unsigned */
    NPF_TEST("  00042", "%07.5u", 42);

    /* padding 20.5 */
    NPF_TEST("               01024", "%20.5u", 1024);
    NPF_TEST("          4294966272", "%20.5u", 4294966272U);

    /* length tests */
    NPF_TEST("                1024", "%20.u", 1024);
    NPF_TEST("          4294966272", "%20.0u", 4294966272U);
    NPF_TEST("                    ", "%20.u", 0U);

    /* non-standard */
    NPF_TEST("  ", "%02.0u", 0U);
#endif

    NPF_TEST_SYS("%u", UINT_MAX);
    NPF_TEST_SYS("%lu", ULONG_MAX);

#if (NANOPRINTF_USE_LARGE_FORMAT_SPECIFIERS == 1)
#if ULLONG_MAX == 18446744073709551615llu
    NPF_TEST("18446744073709551615", "%llu", ULLONG_MAX);
#else
    NPF_TEST("4294967295", "%llu", ULLONG_MAX);
#endif
#if UINTMAX_MAX == 18446744073709551615llu
    NPF_TEST("18446744073709551615", "%ju", UINTMAX_MAX);
#else
    NPF_TEST("4294967295", "%ju", UINTMAX_MAX);
#endif
#if SIZE_MAX == 18446744073709551615llu
    NPF_TEST("18446744073709551615", "%zu", SIZE_MAX);
    NPF_TEST("18446744073709551615", "%tu", SIZE_MAX);
#else
    NPF_TEST("4294967295", "%zu", SIZE_MAX);
    NPF_TEST("4294967295", "%tu", SIZE_MAX);
#endif

    NPF_TEST("281474976710656", "%llu", 281474976710656LLU);
    NPF_TEST("18446744073709551615", "%llu", 18446744073709551615LLU);
    NPF_TEST("2147483647", "%zu", (size_t)2147483647UL);
    NPF_TEST("2147483647", "%zd", (size_t)2147483647UL);
    NPF_TEST("-2147483647", "%zi", (npf_ssize_t)-2147483647L);
    {
        char ptrdiff_buf[11];
        NPF_TEST("a", "%tx", &ptrdiff_buf[10] - &ptrdiff_buf[0]);
    }

    NPF_TEST_SYS("%llu", ULLONG_MAX);
#endif /* NANOPRINTF_USE_LARGE_FORMAT_SPECIFIERS */

    /* ===== octal ===== */
    NPF_TEST("0", "%o", 0);
    NPF_TEST("37777777777", "%o", UINT_MAX);
    NPF_TEST("777", "%o", 511);
    NPF_TEST("37777777001", "%o", 4294966785U);
    NPF_TEST("165140", "%o", 60000);
    NPF_TEST("57060516", "%lo", 12345678L);

    /* space/plus flag non-standard on octal */
    NPF_TEST("777", "% o", 511);
    NPF_TEST("37777777001", "% o", 4294966785U);
    NPF_TEST("777", "%+o", 511);
    NPF_TEST("37777777001", "%+o", 4294966785U);

#if NANOPRINTF_USE_ALT_FORM_FLAG == 1
    NPF_TEST("0", "%#o", 0);
    NPF_TEST("01", "%#o", 1);
    NPF_TEST("010", "%#o", 8);
    NPF_TEST("0377", "%#o", 255);
    NPF_TEST("0777", "%#o", 511);
#endif

#if NANOPRINTF_USE_SMALL_FORMAT_SPECIFIERS == 1
    NPF_TEST("17", "%ho", (1 << 29u) + 15u);
    NPF_TEST("2", "%hho", 258u);
#endif

#if ULONG_MAX > UINT_MAX
    NPF_TEST("40000000000", "%lo", (unsigned long)UINT_MAX + 1u);
#endif

#if NANOPRINTF_USE_FIELD_WIDTH_FORMAT_SPECIFIERS == 1
    NPF_TEST("      2322", "%10o", 1234);
#if NANOPRINTF_USE_ALT_FORM_FLAG == 1
    NPF_TEST("     02322", "%#10o", 1234);
#endif
    NPF_TEST("0001", "%04o", 1);
    NPF_TEST("0000", "%04o", 0);
    NPF_TEST("0", "%+o", 0);
    NPF_TEST("1", "%+o", 1);
    NPF_TEST("   1", "%+4o", 1);
    NPF_TEST("     1", "% 6o", 1);

    /* width */
    NPF_TEST("777", "%1o", 511);
    NPF_TEST("37777777001", "%1o", 4294966785U);

    /* width 20 */
    NPF_TEST("                 777", "%20o", 511);
    NPF_TEST("         37777777001", "%20o", 4294966785U);

    /* width *20 */
    NPF_TEST("                 777", "%*o", 20, 511);
    NPF_TEST("         37777777001", "%*o", 20, 4294966785U);

    /* width -20 */
    NPF_TEST("777                 ", "%-20o", 511);
    NPF_TEST("37777777001         ", "%-20o", 4294966785U);

    /* width 0-20 */
    NPF_TEST("777                 ", "%0-20o", 511);
    NPF_TEST("37777777001         ", "%0-20o", 4294966785U);

    /* padding 20 */
    NPF_TEST("00000000000000000777", "%020o", 511);
    NPF_TEST("00000000037777777001", "%020o", 4294966785U);

#if NANOPRINTF_USE_ALT_FORM_FLAG == 1
    /* # flag with field width */
    NPF_TEST("0", "%#0o", 0);
    NPF_TEST("   0", "%#4o", 0);
    NPF_TEST("01", "%#0o", 1);
    NPF_TEST("  01", "%#4o", 1);
    NPF_TEST("01001", "%#04o", 01001);

    /* padding #020 */
    NPF_TEST("00000000000000000777", "%#020o", 511);
    NPF_TEST("00000000037777777001", "%#020o", 4294966785U);

    /* padding #20 */
    NPF_TEST("                0777", "%#20o", 511);
    NPF_TEST("        037777777001", "%#20o", 4294966785U);
#endif
#endif /* NANOPRINTF_USE_FIELD_WIDTH_FORMAT_SPECIFIERS */

#if NANOPRINTF_USE_PRECISION_FORMAT_SPECIFIERS == 1
    NPF_TEST("", "%.0o", 0);
    NPF_TEST("", "%.o", 0);
    NPF_TEST("00777", "%.5o", 511);
    NPF_TEST("00001", "%.5o", 1);
#if NANOPRINTF_USE_ALT_FORM_FLAG == 1
    NPF_TEST("0", "%#.0o", 0);
    NPF_TEST("0", "%#.1o", 0);
    NPF_TEST("0000", "%#.4o", 0);
    NPF_TEST("01", "%#.0o", 1);
    NPF_TEST("01", "%#.1o", 1);
    NPF_TEST("0001", "%#.4o", 1);
#endif

    /* padding .20 */
    NPF_TEST("00000000000000000777", "%.20o", 511);
    NPF_TEST("00000000037777777001", "%.20o", 4294966785U);
#endif

#if (NANOPRINTF_USE_FIELD_WIDTH_FORMAT_SPECIFIERS == 1) && \
    (NANOPRINTF_USE_PRECISION_FORMAT_SPECIFIERS == 1)
    /* padding 20.5 */
    NPF_TEST("               00777", "%20.5o", 511);
    NPF_TEST("         37777777001", "%20.5o", 4294966785U);

    /* length tests */
    NPF_TEST("                 777", "%20.o", 511);
    NPF_TEST("         37777777001", "%20.0o", 4294966785U);
    NPF_TEST("                    ", "%20.o", 0U);
#endif

#if (NANOPRINTF_USE_LARGE_FORMAT_SPECIFIERS == 1)
#if ULLONG_MAX == 01777777777777777777777llu
    NPF_TEST("1777777777777777777777", "%llo", ULLONG_MAX);
#else
    NPF_TEST("37777777777", "%llo", ULLONG_MAX);
#endif

#if UINTMAX_MAX == 01777777777777777777777llu
    NPF_TEST("1777777777777777777777", "%jo", UINTMAX_MAX);
#else
    NPF_TEST("37777777777", "%jo", UINTMAX_MAX);
#endif

#if SIZE_MAX == 01777777777777777777777llu
    NPF_TEST("1777777777777777777777", "%zo", SIZE_MAX);
    NPF_TEST("1777777777777777777777", "%to", SIZE_MAX);
#else
    NPF_TEST("37777777777", "%zo", SIZE_MAX);
    NPF_TEST("37777777777", "%to", SIZE_MAX);
#endif

#if NANOPRINTF_USE_ALT_FORM_FLAG == 1
    NPF_TEST("0", "%#llo", (long long)0);
    NPF_TEST("01", "%#llo", (long long)1);
#if NANOPRINTF_USE_FIELD_WIDTH_FORMAT_SPECIFIERS == 1
    NPF_TEST("0", "%#0llo", (long long)0);
    NPF_TEST("   0", "%#4llo", (long long)0);
    NPF_TEST("01", "%#0llo", (long long)1);
    NPF_TEST("  01", "%#4llo", (long long)1);
    NPF_TEST("01001", "%#04llo", (long long)01001);
#endif
#if NANOPRINTF_USE_PRECISION_FORMAT_SPECIFIERS == 1
    NPF_TEST("0", "%#.0llo", (long long)0);
    NPF_TEST("0", "%#.1llo", (long long)0);
    NPF_TEST("0000", "%#.4llo", (long long)0);
    NPF_TEST("01", "%#.0llo", (long long)1);
    NPF_TEST("01", "%#.1llo", (long long)1);
    NPF_TEST("0001", "%#.4llo", (long long)1);
#endif
#endif /* NANOPRINTF_USE_ALT_FORM_FLAG */
#endif /* NANOPRINTF_USE_LARGE_FORMAT_SPECIFIERS */

    /* ===== hex ===== */
    NPF_TEST("0", "%x", 0);
    NPF_TEST("12345678", "%x", 0x12345678);
    NPF_TEST("ffffffff", "%x", UINT_MAX);
    NPF_TEST("0", "%X", 0);
    NPF_TEST("90ABCDEF", "%X", 0x90ABCDEF);
    NPF_TEST("FFFFFFFF", "%X", UINT_MAX);
    NPF_TEST("0", "%+x", 0);
    NPF_TEST("1", "%+x", 1);
    NPF_TEST("1234abcd", "%x", 305441741);
    NPF_TEST("edcb5433", "%x", 3989525555U);
    NPF_TEST("1234ABCD", "%X", 305441741);
    NPF_TEST("EDCB5433", "%X", 3989525555U);
    NPF_TEST("12345678", "%lx", 0x12345678L);
    NPF_TEST("abcdefab", "%lx", 0xabcdefabL);
    NPF_TEST("ABCDEFAB", "%lX", 0xabcdefabL);

    /* space/plus flag non-standard on hex */
    NPF_TEST("1234abcd", "% x", 305441741);
    NPF_TEST("edcb5433", "% x", 3989525555U);
    NPF_TEST("1234ABCD", "% X", 305441741);
    NPF_TEST("EDCB5433", "% X", 3989525555U);
    NPF_TEST("1234abcd", "%+x", 305441741);
    NPF_TEST("edcb5433", "%+x", 3989525555U);
    NPF_TEST("1234ABCD", "%+X", 305441741);
    NPF_TEST("EDCB5433", "%+X", 3989525555U);

#if NANOPRINTF_USE_ALT_FORM_FLAG == 1
    NPF_TEST("0", "%#x", 0);
    NPF_TEST("0", "%#X", 0);
    NPF_TEST("0x1", "%#x", 1);
    NPF_TEST("0xff", "%#x", 255);
    NPF_TEST("0X1", "%#X", 1);
    NPF_TEST("0XFF", "%#X", 255);
    NPF_TEST("0xffffffff", "%#x", UINT_MAX);
    NPF_TEST("0XFFFFFFFF", "%#X", UINT_MAX);
#endif

#if NANOPRINTF_USE_SMALL_FORMAT_SPECIFIERS == 1
    NPF_TEST("7b", "%hx", (1 << 26u) + 123u);
    NPF_TEST("b", "%hhx", 256u + 0xb);
#endif

#if ULONG_MAX > UINT_MAX
    NPF_TEST("100000000", "%lx", (unsigned long)UINT_MAX + 1u);
#endif

#if NANOPRINTF_USE_FIELD_WIDTH_FORMAT_SPECIFIERS == 1
    NPF_TEST("      1234", "%10x", 0x1234);
#if NANOPRINTF_USE_ALT_FORM_FLAG == 1
    NPF_TEST("    0x1234", "%#10x", 0x1234);
    NPF_TEST("0x1001", "%#04x", 0x1001);
#endif
    NPF_TEST("0001", "%04u", 1);
    NPF_TEST("0000", "%04u", 0);
    NPF_TEST("     0", "% 6x", 0);
    NPF_TEST("     1", "% 6x", 1);

    /* width */
    NPF_TEST("1234abcd", "%1x", 305441741);
    NPF_TEST("edcb5433", "%1x", 3989525555U);
    NPF_TEST("1234ABCD", "%1X", 305441741);
    NPF_TEST("EDCB5433", "%1X", 3989525555U);

    /* width 20 */
    NPF_TEST("            1234abcd", "%20x", 305441741);
    NPF_TEST("            edcb5433", "%20x", 3989525555U);
    NPF_TEST("            1234ABCD", "%20X", 305441741);
    NPF_TEST("            EDCB5433", "%20X", 3989525555U);
    NPF_TEST("                   0", "%20X", 0);
    NPF_TEST("                   0", "%20X", 0U);

    /* width *20 */
    NPF_TEST("            1234abcd", "%*x", 20, 305441741);
    NPF_TEST("            edcb5433", "%*x", 20, 3989525555U);
    NPF_TEST("            1234ABCD", "%*X", 20, 305441741);
    NPF_TEST("            EDCB5433", "%*X", 20, 3989525555U);

    /* width -20 */
    NPF_TEST("1234abcd            ", "%-20x", 305441741);
    NPF_TEST("edcb5433            ", "%-20x", 3989525555U);
    NPF_TEST("1234ABCD            ", "%-20X", 305441741);
    NPF_TEST("EDCB5433            ", "%-20X", 3989525555U);

    /* width 0-20 */
    NPF_TEST("1234abcd            ", "%0-20x", 305441741);
    NPF_TEST("edcb5433            ", "%0-20x", 3989525555U);
    NPF_TEST("1234ABCD            ", "%0-20X", 305441741);
    NPF_TEST("EDCB5433            ", "%0-20X", 3989525555U);

    /* padding 20 */
    NPF_TEST("0000000000001234abcd", "%020x", 305441741);
    NPF_TEST("000000000000edcb5433", "%020x", 3989525555U);
    NPF_TEST("0000000000001234ABCD", "%020X", 305441741);
    NPF_TEST("000000000000EDCB5433", "%020X", 3989525555U);

#if NANOPRINTF_USE_ALT_FORM_FLAG == 1
    /* padding #020 */
    NPF_TEST("0x00000000001234abcd", "%#020x", 305441741);
    NPF_TEST("0x0000000000edcb5433", "%#020x", 3989525555U);
    NPF_TEST("0X00000000001234ABCD", "%#020X", 305441741);
    NPF_TEST("0X0000000000EDCB5433", "%#020X", 3989525555U);

    /* padding #20 */
    NPF_TEST("          0x1234abcd", "%#20x", 305441741);
    NPF_TEST("          0xedcb5433", "%#20x", 3989525555U);
    NPF_TEST("          0X1234ABCD", "%#20X", 305441741);
    NPF_TEST("          0XEDCB5433", "%#20X", 3989525555U);
#endif

#if NANOPRINTF_USE_LARGE_FORMAT_SPECIFIERS == 1
    NPF_TEST("                   0", "%20llX", 0ULL);
#endif
#endif /* NANOPRINTF_USE_FIELD_WIDTH_FORMAT_SPECIFIERS */

#if NANOPRINTF_USE_PRECISION_FORMAT_SPECIFIERS == 1
    NPF_TEST("", "%.0x", 0);
    NPF_TEST("", "%.0X", 0);
    NPF_TEST("", "%.x", 0);
    NPF_TEST("", "%.X", 0);
    NPF_TEST("000ff", "%.5x", 255);
    NPF_TEST("000FF", "%.5X", 255);
    NPF_TEST("00001", "%.5x", 1);
    NPF_TEST("00001", "%.5X", 1);
#if NANOPRINTF_USE_ALT_FORM_FLAG == 1
    NPF_TEST("", "%#.0X", 0);
    NPF_TEST("", "%#.0x", 0);
    NPF_TEST("0x0000614e", "%#.8x", 0x614e);
#if NANOPRINTF_USE_LARGE_FORMAT_SPECIFIERS == 1
    NPF_TEST("", "%#.0llx", (long long)0);
    NPF_TEST("0x0000614e", "%#.8llx", (long long)0x614e);
#endif
#endif

    /* padding .20 */
    NPF_TEST("0000000000001234abcd", "%.20x", 305441741);
    NPF_TEST("000000000000edcb5433", "%.20x", 3989525555U);
    NPF_TEST("0000000000001234ABCD", "%.20X", 305441741);
    NPF_TEST("000000000000EDCB5433", "%.20X", 3989525555U);
#endif

#if (NANOPRINTF_USE_FIELD_WIDTH_FORMAT_SPECIFIERS == 1) && \
    (NANOPRINTF_USE_PRECISION_FORMAT_SPECIFIERS == 1)
    /* padding 20.5 */
    NPF_TEST("            1234abcd", "%20.5x", 305441741);
    NPF_TEST("          00edcb5433", "%20.10x", 3989525555U);
    NPF_TEST("            1234ABCD", "%20.5X", 305441741);
    NPF_TEST("          00EDCB5433", "%20.10X", 3989525555U);

    /* length tests */
    NPF_TEST("            1234abcd", "%20.x", 305441741);
    NPF_TEST("                                          1234abcd", "%50.x", 305441741);
    NPF_TEST("                                          1234abcd     12345", "%50.x%10.u", 305441741, 12345);
    NPF_TEST("            edcb5433", "%20.0x", 3989525555U);
    NPF_TEST("                    ", "%20.x", 0U);
    NPF_TEST("            1234ABCD", "%20.X", 305441741);
    NPF_TEST("            EDCB5433", "%20.0X", 3989525555U);
    NPF_TEST("                    ", "%20.X", 0U);
#endif

#if (NANOPRINTF_USE_LARGE_FORMAT_SPECIFIERS == 1)
#if ULLONG_MAX == 0xffffffffffffffffllu
    NPF_TEST("ffffffffffffffff", "%llx", ULLONG_MAX);
#else
    NPF_TEST("ffffffff", "%llx", ULLONG_MAX);
#endif

#if UINTMAX_MAX == 0xffffffffffffffffllu
    NPF_TEST("ffffffffffffffff", "%jx", UINTMAX_MAX);
#else
    NPF_TEST("ffffffff", "%jx", UINTMAX_MAX);
#endif

#if SIZE_MAX == 0xffffffffffffffffllu
    NPF_TEST("ffffffffffffffff", "%zx", SIZE_MAX);
    NPF_TEST("ffffffffffffffff", "%tx", SIZE_MAX);
#else
    NPF_TEST("ffffffff", "%zx", SIZE_MAX);
    NPF_TEST("ffffffff", "%tx", SIZE_MAX);
#endif

    NPF_TEST("1234567891234567", "%llx", 0x1234567891234567LLU);

#if NANOPRINTF_USE_ALT_FORM_FLAG == 1
#if NANOPRINTF_USE_FIELD_WIDTH_FORMAT_SPECIFIERS == 1
    NPF_TEST("0x1001", "%#04llx", (long long)0x1001);
#endif
#endif
#endif /* NANOPRINTF_USE_LARGE_FORMAT_SPECIFIERS */

    /* ===== binary ===== */
#if NANOPRINTF_USE_BINARY_FORMAT_SPECIFIERS == 1
    NPF_TEST("0", "%b", 0);
    NPF_TEST("1", "%b", 1);
    NPF_TEST("101", "%b", 5);
    NPF_TEST("11111111", "%b", 255);
    NPF_TEST("1110101001100000", "%b", 60000);
    NPF_TEST("101111000110000101001110", "%lb", 12345678L);
    NPF_TEST("11111111111111111111111111111111", "%b", UINT_MAX);

#if NANOPRINTF_USE_ALT_FORM_FLAG == 1
    NPF_TEST("0", "%#b", 0);
    NPF_TEST("0b1", "%#b", 1);
    NPF_TEST("0b101", "%#b", 5);
    NPF_TEST("0b110", "%#b", 6);
    NPF_TEST("0b11111111", "%#b", 255);
#if NANOPRINTF_USE_LARGE_FORMAT_SPECIFIERS == 1
    NPF_TEST("0b110", "%#llb", (long long)6);
#endif
#endif

#if NANOPRINTF_USE_SMALL_FORMAT_SPECIFIERS == 1
    NPF_TEST("11111111", "%hhb", 0xFFu);
    NPF_TEST("0", "%hhb", 256u);
    NPF_TEST("1111111111111111", "%hb", 0xFFFFu);
#endif

#if NANOPRINTF_USE_FIELD_WIDTH_FORMAT_SPECIFIERS == 1
    NPF_TEST("       101", "%10b", 5);
    NPF_TEST("101       ", "%-10b", 5);
    NPF_TEST("0000000101", "%010b", 5);
#if NANOPRINTF_USE_ALT_FORM_FLAG == 1
    NPF_TEST("     0b101", "%#10b", 5);
    NPF_TEST("0b101     ", "%#-10b", 5);
    NPF_TEST("0b00000101", "%#010b", 5);
#endif
#endif

#if NANOPRINTF_USE_PRECISION_FORMAT_SPECIFIERS == 1
    NPF_TEST("", "%.0b", 0);
    NPF_TEST("1", "%.0b", 1);
    NPF_TEST("00000101", "%.8b", 5);
    NPF_TEST("00000001", "%.8b", 1);
#if NANOPRINTF_USE_ALT_FORM_FLAG == 1
    NPF_TEST("", "%#.0b", 0);
    NPF_TEST("0b00000101", "%#.8b", 5);
#endif
#endif

#if (NANOPRINTF_USE_FIELD_WIDTH_FORMAT_SPECIFIERS == 1) && \
    (NANOPRINTF_USE_PRECISION_FORMAT_SPECIFIERS == 1)
    NPF_TEST("  00000101", "%10.8b", 5);
    NPF_TEST("00000101  ", "%-10.8b", 5);
#endif

#if NANOPRINTF_USE_LARGE_FORMAT_SPECIFIERS == 1
    NPF_TEST("101", "%llb", 5ULL);
#endif
#endif

    /* ===== pointer ===== */
#if NANOPRINTF_USE_PRECISION_FORMAT_SPECIFIERS == 1
  #if UINTPTR_MAX > 0xffffffffu
    NPF_TEST("0000000000001234", "%p", (void *)0x1234u);
    NPF_TEST("0000000012345678", "%p", (void *)0x12345678u);
    NPF_TEST("0000000012345678-000000007edcba98", "%p-%p",
        (void *)0x12345678u, (void *)0x7edcba98u);
    NPF_TEST("00000000ffffffff", "%p", (void *)(uintptr_t)0xffffffffu);
    NPF_TEST("0000000000000000", "%p", (void *)0);
  #else
    NPF_TEST("00001234", "%p", (void *)0x1234u);
    NPF_TEST("12345678", "%p", (void *)0x12345678u);
    NPF_TEST("12345678-7edcba98", "%p-%p",
        (void *)0x12345678u, (void *)0x7edcba98u);
    NPF_TEST("ffffffff", "%p", (void *)(uintptr_t)0xffffffffu);
    NPF_TEST("00000000", "%p", (void *)0);
  #endif
#else
    NPF_TEST("1234", "%p", (void *)0x1234u);
    NPF_TEST("12345678", "%p", (void *)0x12345678u);
    NPF_TEST("12345678-7edcba98", "%p-%p",
        (void *)0x12345678u, (void *)0x7edcba98u);
    NPF_TEST("ffffffff", "%p", (void *)(uintptr_t)0xffffffffu);
    NPF_TEST("0", "%p", (void *)0);
#endif

    /* ===== writeback ===== */
#if NANOPRINTF_USE_WRITEBACK_FORMAT_SPECIFIERS == 1
    { int wb = -1;
      npf_pprintf(npf_test_null_putc, 0, "%n", &wb);
      NPF_TEST_WB(0, wb); }
    { int wb = -1;
      npf_pprintf(npf_test_null_putc, 0, " %n", &wb);
      NPF_TEST_WB(1, wb); }
    { int wb = -1;
      npf_pprintf(npf_test_null_putc, 0, "  %n", &wb);
      NPF_TEST_WB(2, wb); }
    { int wb = -1;
      npf_pprintf(npf_test_null_putc, 0, "%s%n", "abcd", &wb);
      NPF_TEST_WB(4, wb); }
    { int wb = -1;
      npf_pprintf(npf_test_null_putc, 0, "%u%s%n", 0, "abcd", &wb);
      NPF_TEST_WB(5, wb); }

    /* snprintf writeback */
    { char wbbuf[100]; int wb = 1234;
      npf_snprintf(wbbuf, sizeof(wbbuf), "%n", &wb);
      NPF_TEST_WB(0, wb); }
    { char wbbuf[100]; int wb = 1234;
      npf_snprintf(wbbuf, sizeof(wbbuf), "foo%nbar", &wb);
      NPF_TEST_WB(3, wb);
      NPF_TEST("foobar", "foo%nbar", &wb); }

    /* writeback long */
    { long wb = -1;
      npf_pprintf(npf_test_null_putc, 0, "1234567%ln", &wb);
      NPF_TEST_WB(7, wb); }

#if NANOPRINTF_USE_SMALL_FORMAT_SPECIFIERS == 1
    /* writeback short */
    { short wb = -1;
      npf_pprintf(npf_test_null_putc, 0, "1234%hn", &wb);
      NPF_TEST_WB(4, wb); }
    /* writeback char */
    { signed char wb = -1;
      npf_pprintf(npf_test_null_putc, 0, "1234567%hhn", &wb);
      NPF_TEST_WB(7, wb); }
#endif

#if NANOPRINTF_USE_LARGE_FORMAT_SPECIFIERS == 1
    /* writeback long long */
    { long long wb = -1;
      npf_pprintf(npf_test_null_putc, 0, "12345678%lln", &wb);
      NPF_TEST_WB(8, wb); }
    /* writeback intmax_t */
    { intmax_t wb = -1;
      npf_pprintf(npf_test_null_putc, 0, "12345678%jn", &wb);
      NPF_TEST_WB(8, wb); }
    /* writeback size_t (via intmax_t) */
    { intmax_t wb = 100000;
      npf_pprintf(npf_test_null_putc, 0, "12345678%zn", &wb);
      NPF_TEST_WB(8, wb); }
    /* writeback ptrdiff_t */
    { ptrdiff_t wb = -1;
      npf_pprintf(npf_test_null_putc, 0, "12345678%tn", &wb);
      NPF_TEST_WB(8, wb); }
#endif
#endif /* NANOPRINTF_USE_WRITEBACK_FORMAT_SPECIFIERS */

    /* ===== star args ===== */
#if NANOPRINTF_USE_FIELD_WIDTH_FORMAT_SPECIFIERS == 1
    NPF_TEST("         Z", "%*c", 10, 'Z');
    NPF_TEST("5     ", "%*u", -6, 5);
    NPF_TEST("hi x", "%*sx", -3, "hi");
    NPF_TEST("               Hello", "%*s", 20, "Hello");
    NPF_TEST("                   x", "%*c", 20, 'x');
    /* negative star width = left-justify with abs(width) */
    NPF_TEST("42        ", "%*d", -10, 42);
    NPF_TEST("-42       ", "%*d", -10, -42);
    NPF_TEST("x         ", "%*c", -10, 'x');
    NPF_TEST("hello     ", "%*s", -10, "hello");
#endif

#if NANOPRINTF_USE_PRECISION_FORMAT_SPECIFIERS == 1
    NPF_TEST("01", "%.*i", 2, 1);
    NPF_TEST("h", "%.*s", 1, "hello world");
    NPF_TEST("1", "%.*u", -123, 1);
    /* negative star precision = precision omitted (use default) */
    NPF_TEST("42", "%.*d", -1, 42);
    NPF_TEST("0", "%.*d", -1, 0);
    NPF_TEST("hello", "%.*s", -1, "hello");
    NPF_TEST("hello world", "%.*s", -1, "hello world");
#endif

#if (NANOPRINTF_USE_FIELD_WIDTH_FORMAT_SPECIFIERS == 1) && \
    (NANOPRINTF_USE_PRECISION_FORMAT_SPECIFIERS == 1)
    NPF_TEST("        07", "%*.*i", 10, 2, 7);
#endif

    /* ===== float ===== */
#if NANOPRINTF_USE_FLOAT_FORMAT_SPECIFIERS == 1
    /* nan */
    {
        char nan_buf[32];
        npf_snprintf(nan_buf, sizeof(nan_buf), "%f", (double)NAN);
        /* nan sign is implementation-defined */
        if (strcmp(nan_buf, "nan") != 0 && strcmp(nan_buf, "-nan") != 0) {
            fprintf(stderr, "FAIL [%s:%d]: %%f NAN expected nan or -nan, got \"%s\"\n",
                    __FILE__, __LINE__, nan_buf);
            ++npf_test_fail_count;
        } else { ++npf_test_pass_count; }
    }
    {
        char nan_buf[32];
        npf_snprintf(nan_buf, sizeof(nan_buf), "%F", (double)NAN);
        if (strcmp(nan_buf, "NAN") != 0 && strcmp(nan_buf, "-NAN") != 0) {
            fprintf(stderr, "FAIL [%s:%d]: %%F NAN expected NAN or -NAN, got \"%s\"\n",
                    __FILE__, __LINE__, nan_buf);
            ++npf_test_fail_count;
        } else { ++npf_test_pass_count; }
    }

    /* inf */
    NPF_TEST("inf", "%f", (double)INFINITY);
    NPF_TEST("-inf", "%f", -(double)INFINITY);
    NPF_TEST("inf", "%.100f", (double)INFINITY);
    NPF_TEST("inf", "%.10f", (double)INFINITY);
    NPF_TEST("inf", "%.10e", (double)INFINITY);
    NPF_TEST("inf", "%.10g", (double)INFINITY);
    NPF_TEST("inf", "%.10a", (double)INFINITY);
    NPF_TEST("INF", "%F", (double)INFINITY);
    NPF_TEST("+inf", "%+f", (double)INFINITY);
    NPF_TEST("-inf", "%+f", -(double)INFINITY);
    NPF_TEST(" inf", "% f", (double)INFINITY);
    NPF_TEST("-inf", "% f", -(double)INFINITY);
    NPF_TEST(" 0.000000", "% f", 0.0);
    NPF_TEST("-0.000000", "% f", -0.0);
    NPF_TEST(" 1.500000", "% f", 1.5);
    NPF_TEST("-1.500000", "% f", -1.5);

#if NANOPRINTF_USE_FIELD_WIDTH_FORMAT_SPECIFIERS == 1
    NPF_TEST(" inf", "%4f", (double)INFINITY);
    NPF_TEST("     nan", "%8f", (double)NAN);
    NPF_TEST("     inf", "%8f", (double)INFINITY);
    NPF_TEST("-inf    ", "%-8f", (double)-INFINITY);
#endif

    /* basic float */
    NPF_TEST("0.000000", "%f", 0.0);
    NPF_TEST("-0.000000", "%f", -0.0);
    NPF_TEST("0.000000", "%f", 1e-20);
    NPF_TEST("-0.000000", "%f", -1e-20);
    NPF_TEST("0.00", "%.2f", 0.0);
    NPF_TEST("1.0", "%.1f", 1.0);
    NPF_TEST("1", "%.0f", 1.0);
    NPF_TEST("1.00000000000", "%.11f", 1.0);
    NPF_TEST("1.5", "%.1f", 1.5);
    NPF_TEST("+1.5", "%+.1f", 1.5);
    NPF_TEST("-1.5", "%.1f", -1.5);
    NPF_TEST(" 1.5", "% .1f", 1.5);
    NPF_TEST("1.50000000000000000", "%.17f", 1.5);
    NPF_TEST("0.003906", "%f", 0.00390625);
    NPF_TEST("0.0039", "%.4f", 0.00390625);
    NPF_TEST("0.00390625", "%.8f", 0.00390625);
    NPF_TEST("0.00390625", "%.8Lf", (long double)0.00390625);
    NPF_TEST("-0.00390625", "%.8f", -0.00390625);
    NPF_TEST("-0.00390625", "%.8Lf", (long double)-0.00390625);
    NPF_TEST("42.500000", "%f", 42.5);
    NPF_TEST("42.5", "%.1f", 42.5);
    NPF_TEST("42167.000000", "%f", 42167.0);

#if NANOPRINTF_USE_ALT_FORM_FLAG == 1
    NPF_TEST("0.", "%#.0f", 0.0);
    NPF_TEST("1.", "%#.0f", 1.0);
    NPF_TEST("42.", "%#.0f", 42.0);
    NPF_TEST("-1.", "%#.0f", -1.0);
#endif

    /* paland float tests */
    NPF_TEST("3.1415", "%.4f", 3.1415354);
    NPF_TEST("30343.142", "%.3f", 30343.1415354);
    NPF_TEST("34", "%.0f", 34.1415354);
    NPF_TEST("1", "%.0f", 1.3);
    NPF_TEST("2", "%.0f", 1.55);
    NPF_TEST("1.6", "%.1f", 1.64);
    NPF_TEST("42.90", "%.2f", 42.8952);
    NPF_TEST("42.895199999", "%.9f", 42.8952);
    NPF_TEST("42.8952229992", "%.10f", 42.895223);
    NPF_TEST("42.895223123208", "%.12f", 42.89522312345678);
    NPF_TEST("42.895223876461", "%.12f", 42.89522387654321);
    NPF_TEST("-12345.987654321", "%.9f", -12345.987654321);
    NPF_TEST("4.0", "%.1f", 3.999);
    NPF_TEST("4", "%.0f", 3.5);
    NPF_TEST("5", "%.0f", 4.5);
    NPF_TEST("3", "%.0f", 3.49);
    NPF_TEST("3.5", "%.1f", 3.49);

#if NANOPRINTF_USE_FIELD_WIDTH_FORMAT_SPECIFIERS == 1
    NPF_TEST(" 1.0", "%4.1f", 1.0);
    NPF_TEST(" 1.500", "%6.3f", 1.5);
    NPF_TEST("0001.500", "%08.3f", 1.5);
    NPF_TEST("+001.500", "%+08.3f", 1.5);
    NPF_TEST("-001.500", "%+08.3f", -1.5);

    /* paland field width float */
    NPF_TEST("42477.371093750000000", "%020.15f", 42477.37109375);
    NPF_TEST(" 42.90", "%6.2f", 42.8952);
    NPF_TEST("+42.90", "%+6.2f", 42.8952);
    NPF_TEST("+42.9", "%+5.1f", 42.9252);
    NPF_TEST("a0.5  ", "a%-5.1f", 0.5);
    NPF_TEST("a0.5  end", "a%-5.1fend", 0.5);
    NPF_TEST("1024.1234           ", "%-20.4f", 1024.1234);

    /* space flag float */
    NPF_TEST("        -42.987", "% 15.3f", -42.987);
    NPF_TEST("         42.987", "% 15.3f", 42.987);

    /* 0 flag float */
    NPF_TEST("000000000042.12", "%015.2f", 42.1234);
    NPF_TEST("00000000042.988", "%015.3f", 42.9876);
    NPF_TEST("-00000042.98760", "%015.5f", -42.9876);

    /* float padding neg numbers */
    NPF_TEST("-5.0", "% 3.1f", -5.);
    NPF_TEST("-5.0", "% 4.1f", -5.);
    NPF_TEST(" -5.0", "% 5.1f", -5.);
    NPF_TEST("-5.0", "%03.1f", -5.);
    NPF_TEST("-5.0", "%04.1f", -5.);
    NPF_TEST("-05.0", "%05.1f", -5.);
    NPF_TEST("-5", "%01.0f", -5.);
    NPF_TEST("-5", "%02.0f", -5.);
    NPF_TEST("-05", "%03.0f", -5.);
#endif /* NANOPRINTF_USE_FIELD_WIDTH_FORMAT_SPECIFIERS */

    /* misc float */
    NPF_TEST("-67224.54687500000000000", "%.17f", -67224.546875);
    NPF_TEST("0.33", "%.*f", 2, 0.33333333);

    /* ===== hex float (%a / %A) ===== */
#if NANOPRINTF_USE_FLOAT_HEX_FORMAT_SPECIFIER == 1
    /* basic zero */
    NPF_TEST("0x0p+0", "%.0a", 0.0);
    NPF_TEST("-0x0p+0", "%.0a", -0.0);
    NPF_TEST("0x0.0p+0", "%.1a", 0.0);
    NPF_TEST("0x0.00p+0", "%.2a", 0.0);

    /* case: %A vs %a */
    NPF_TEST("0X0P+0", "%.0A", 0.0);
    NPF_TEST("0X1P+0", "%.0A", 1.0);
    NPF_TEST("0x1p+0", "%.0a", 1.0);

    /* special values */
    NPF_TEST("inf", "%a", (double)INFINITY);
    NPF_TEST("-inf", "%a", -(double)INFINITY);
    NPF_TEST("INF", "%A", (double)INFINITY);
    NPF_TEST("+inf", "%+a", (double)INFINITY);
    NPF_TEST(" inf", "% a", (double)INFINITY);

    /* basic normals */
    NPF_TEST("0x1p+0", "%.0a", 1.0);
    NPF_TEST("0x1.8p+0", "%.1a", 1.5);
    NPF_TEST("0x1.0p+1", "%.1a", 2.0);
    NPF_TEST("0x1.4p+1", "%.1a", 2.5);
    NPF_TEST("0x1.8p+1", "%.1a", 3.0);
    NPF_TEST("0x1.0p+2", "%.1a", 4.0);
    NPF_TEST("-0x1.8p+0", "%.1a", -1.5);

    /* default precision: 13 hex digits for float64 */
    NPF_TEST("0x0.0000000000000p+0", "%a", 0.0);
    NPF_TEST("0x1.0000000000000p+0", "%a", 1.0);
    NPF_TEST("0x1.8000000000000p+0", "%a", 1.5);

    /* explicit precision */
    NPF_TEST("0x1.921fb54442d18p+2", "%a", 6.283185307179586);
    NPF_TEST("0x1.9p+2", "%.1a", 6.283185307179586);

    /* rounding */
    NPF_TEST("0x1p+0", "%.0a", 1.0);
    NPF_TEST("0x1.cp+0", "%.1a", 1.75);
    NPF_TEST("0x1.0000000000000p+0", "%.13a", 1.0);

    /* sign flags */
    NPF_TEST("+0x1p+0", "%+.0a", 1.0);
    NPF_TEST("-0x1p+0", "%+.0a", -1.0);
    NPF_TEST(" 0x1p+0", "% .0a", 1.0);
    NPF_TEST("-0x1p+0", "% .0a", -1.0);

#if NANOPRINTF_USE_ALT_FORM_FLAG == 1
    /* # flag: always show decimal point */
    NPF_TEST("0x1.p+0", "%#.0a", 1.0);
    NPF_TEST("0x0.p+0", "%#.0a", 0.0);
#endif

#if NANOPRINTF_USE_FIELD_WIDTH_FORMAT_SPECIFIERS == 1
    /* field width, right-justified */
    NPF_TEST("    0x1p+0", "%10.0a", 1.0);
    NPF_TEST("   -0x1p+0", "%10.0a", -1.0);

    /* field width, left-justified */
    NPF_TEST("0x1p+0    ", "%-10.0a", 1.0);
    NPF_TEST("-0x1p+0   ", "%-10.0a", -1.0);

    /* zero-padded: sign -> 0x -> zeros -> digits */
    NPF_TEST("0x001p+0", "%08.0a", 1.0);
    NPF_TEST("-0x01p+0", "%08.0a", -1.0);
    NPF_TEST("+0x01p+0", "%+08.0a", 1.0);

    /* space flag with width */
    NPF_TEST(" 0x1p+0   ", "% -10.0a", 1.0);
    NPF_TEST("-0x1p+0   ", "% -10.0a", -1.0);
#endif

    /* long double */
    NPF_TEST("0x1.8000000000000p+0", "%La", (long double)1.5);

    /* precision 0 with no alt: no decimal point */
    NPF_TEST("0x1p+0", "%.0a", 1.0);

    /* large exponent */
    NPF_TEST("0x1.fffffffffffffp+1023", "%a", 1.7976931348623157e+308);

    /* subnormal: smallest subnormal */
    NPF_TEST("0x0.0000000000001p-1022", "%a", 5e-324);
    NPF_TEST("0x0p-1022", "%.0a", 5e-324);
#endif /* NANOPRINTF_USE_FLOAT_HEX_FORMAT_SPECIFIER */

#endif /* NANOPRINTF_USE_FLOAT_FORMAT_SPECIFIERS */

    /* ===== non-standard specifiers ===== */

    /* space/plus flag non-standard on string/char */
    NPF_TEST("Hello testing", "% s", "Hello testing");
    NPF_TEST("Hello testing", "%+s", "Hello testing");
    NPF_TEST("x", "% c", 'x');
    NPF_TEST("x", "%+c", 'x');

    /* unknown flag (non-standard) */
    NPF_TEST("%kmarco", "%kmarco");

    /* ===== field width never truncates ===== */
#if NANOPRINTF_USE_FIELD_WIDTH_FORMAT_SPECIFIERS == 1
    NPF_TEST("12345", "%3d", 12345);
    NPF_TEST("-12345", "%3d", -12345);
    NPF_TEST("12345", "%3u", 12345);
    NPF_TEST("hello world", "%5s", "hello world");
    NPF_TEST("ffffffff", "%4x", UINT_MAX);
    NPF_TEST("FFFFFFFF", "%4X", UINT_MAX);
    NPF_TEST("37777777777", "%4o", UINT_MAX);
#if NANOPRINTF_USE_BINARY_FORMAT_SPECIFIERS == 1
    NPF_TEST("11111111", "%4b", 255);
#endif
#endif

    /* ===== misc ===== */
    NPF_TEST("", "");
    NPF_TEST("53000atest-20 bit", "%u%u%ctest%d %s", 5, 3000, 'a', -20, "bit");
    NPF_TEST("v", "%c", 'v');
    NPF_TEST("wv", "%cv", 'w');
    NPF_TEST("100%", "%d%%", 100);
    NPF_TEST("%42%", "%%%d%%", 42);
    NPF_TEST("abc", "a%sc", "b");
    NPF_TEST("hello 42 world", "hello %d world", 42);

#if NANOPRINTF_USE_SMALL_FORMAT_SPECIFIERS == 1
    NPF_TEST("Test100 65535", "%s%hhi %hu", "Test", (char)100, (unsigned short)0xFFFF);
#endif

    /* ===== types ===== */
#if NANOPRINTF_USE_LARGE_FORMAT_SPECIFIERS == 1
    NPF_TEST("30", "%lli", 30LL);
    NPF_TEST("-9223372036854775807", "%lli", -9223372036854775807LL);
    NPF_TEST("9223372036854775807", "%lli", 9223372036854775807LL);
    NPF_TEST("281474976710656", "%llu", 281474976710656LLU);
    NPF_TEST("18446744073709551615", "%llu", 18446744073709551615LLU);
    NPF_TEST("-2147483647", "%ji", (intmax_t)-2147483647L);
    NPF_TEST("1234567891234567", "%llx", 0x1234567891234567LLU);
#endif

    NPF_TEST_PASS_COUNT = npf_test_pass_count;
    return npf_test_fail_count;
}

#ifdef __cplusplus
}
#endif
