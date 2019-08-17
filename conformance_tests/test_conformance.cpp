#include "../nanoprintf.h"

#include "CppUTest/TestHarness.h"

#include <limits.h>
#include <cmath>
#include <cstring>

TEST_GROUP(conformance){};

namespace {
char conformance_buf[256];
void CheckConformance(char const *output, char const *fmt, ...) {
    std::string expected;
    {
        va_list v;
        va_start(v, fmt);
        vsnprintf(conformance_buf, sizeof(conformance_buf), fmt, v);
        va_end(v);
        expected = conformance_buf;
    }

    std::string actual;
    {
        va_list v;
        va_start(v, fmt);
        npf_vsnprintf(conformance_buf, sizeof(conformance_buf), fmt, v);
        va_end(v);
        actual = conformance_buf;
    }

    CHECK_EQUAL(expected, std::string(output));
    CHECK_EQUAL(expected, actual);
}
}  // namespace

TEST(conformance, Percent) {
    CheckConformance("%", "%%");
#if NANOPRINTF_USE_FIELD_WIDTH_FORMAT_SPECIFIERS == 1
    CheckConformance("%", "%-%");
#endif
    CheckConformance("%", "% %");
    CheckConformance("%", "%+%");
    CheckConformance("%", "%#%");
    // CheckConformance("         %", "%10%"); clang adds width, gcc doesn't
    // CheckConformance("%         ", "%-10%"); clang adds -width, gcc doesn't
    // CheckConformance("         %", "%10.10%"); clang adds width + precision.
    // CheckConformance("%012%"); Undefined
}

TEST(conformance, Char) {
    // every char
    for (int i = CHAR_MIN; i < CHAR_MAX; ++i) {
        char output[2] = {(char)i, 0};
        CheckConformance(output, "%c", i);
    }

    CheckConformance("A", "%+c", 'A');
    CheckConformance("", "%+c", 0);

#if NANOPRINTF_USE_FIELD_WIDTH_FORMAT_SPECIFIERS == 1
    // right justify field width
    CheckConformance("A", "%1c", 'A');
    CheckConformance(" A", "%2c", 'A');
    CheckConformance("  A", "%3c", 'A');

    // left justify field width
    CheckConformance("A", "%-1c", 'A');
    CheckConformance("A ", "%-2c", 'A');
    CheckConformance("A  ", "%-3c", 'A');

    CheckConformance("     A", "% 6c", 'A');
    CheckConformance("   A", "%+4c", 'A');
#endif
}

TEST(conformance, Strings) {
    CheckConformance("one", "%s", "one");
    CheckConformance("onetwothree", "%s%s%s", "one", "two", "three");

#if NANOPRINTF_USE_FIELD_WIDTH_FORMAT_SPECIFIERS == 1
    CheckConformance("       two", "%10s", "two");
#endif

#if NANOPRINTF_USE_PRECISION_FORMAT_SPECIFIERS == 1
    CheckConformance("thr", "%.3s", "three");
    CheckConformance("four", "%.100s", "four");
    // CheckConformance("abc", "%010s", "abc");  // undefined
    CheckConformance("", "%.0s", "five");
#endif

#if (NANOPRINTF_USE_FIELD_WIDTH_FORMAT_SPECIFIERS == 1) && \
    (NANOPRINTF_USE_PRECISION_FORMAT_SPECIFIERS == 1)
    CheckConformance("       six", "%10.3s", "sixAAAAAAAA");
#endif
}

TEST(conformance, UnsignedInt) {
    CheckConformance("0", "%u", 0);
    CheckConformance("4294967295", "%u", UINT_MAX);
    CheckConformance("0", "%+u", 0);
    CheckConformance("1", "%+u", 1);
    CheckConformance("13", "%hu", (1 << 21u) + 13u);  // "short" mod clips
#if ULONG_MAX > UINT_MAX
    CheckConformance("4294967296", "%lu", (unsigned long)UINT_MAX + 1u);
#endif
    CheckConformance("0", "%hhu", 256u);

#if NANOPRINTF_USE_FIELD_WIDTH_FORMAT_SPECIFIERS == 1
    CheckConformance("   1", "%+4u", 1);  // undefined but usually skips +
    CheckConformance("     0", "% 6u", 0);
#endif

#if NANOPRINTF_USE_PRECISION_FORMAT_SPECIFIERS == 1
    CheckConformance("", "%.0u", 0);
    CheckConformance("01", "%.2u", 1);
#endif

#if (NANOPRINTF_USE_FIELD_WIDTH_FORMAT_SPECIFIERS == 1) && \
    (NANOPRINTF_USE_PRECISION_FORMAT_SPECIFIERS == 1)
    CheckConformance("    0123", "%8.4u", 123);
#endif

#if (NANOPRINTF_USE_LARGE_FORMAT_SPECIFIERS == 1)
#if ULLONG_MAX == 18446744073709551615llu
    CheckConformance("18446744073709551615", "%llu", ULLONG_MAX);
#else
    CheckConformance("4294967295", "%llu", ULLONG_MAX);
#endif
#if UINTMAX_MAX == 18446744073709551615llu
    CheckConformance("18446744073709551615", "%ju", UINTMAX_MAX);
#else
    CheckConformance("4294967295", "%ju", UINTMAX_MAX);
#endif
#if SIZE_MAX == 18446744073709551615llu
    CheckConformance("18446744073709551615", "%zu", SIZE_MAX);
    CheckConformance("18446744073709551615", "%tu", SIZE_MAX);
#else
    CheckConformance("4294967295", "%zu", SIZE_MAX);
    CheckConformance("4294967295", "%tu", SIZE_MAX);
#endif
#endif
}

TEST(conformance, SignedInt) {
    CheckConformance("-2147483648", "%i", INT_MIN);
    CheckConformance("0", "%i", 0);
    CheckConformance("2147483647", "%i", INT_MAX);
    CheckConformance("-1", "%+i", -1);
    CheckConformance("+0", "%+i", 0);
    CheckConformance("+1", "%+i", 1);
    // CheckConformance("%.-123i", 400); xcode libc doesn't ignore negative
    CheckConformance("-128", "%hhi", 128);

#if NANOPRINTF_USE_FIELD_WIDTH_FORMAT_SPECIFIERS == 1
    CheckConformance("  -1", "% 4i", -1);
    CheckConformance("   0", "% 4i", 0);
    CheckConformance("   1", "% 4i", 1);
    CheckConformance("  +1", "%+4i", 1);
    CheckConformance("  +0", "%+4i", 0);
    CheckConformance("  -1", "%+4i", -1);
    CheckConformance("0001", "%04i", 1);
    CheckConformance("0000", "%04i", 0);
    CheckConformance("-001", "%04i", -1);
    CheckConformance("+001", "%+04i", 1);
#endif

#if NANOPRINTF_USE_PRECISION_FORMAT_SPECIFIERS == 1
    CheckConformance("", "%.0i", 0);
    CheckConformance("+", "%+.0i", 0);
    CheckConformance("+01", "%+.2i", 1);
#endif

#if (NANOPRINTF_USE_FIELD_WIDTH_FORMAT_SPECIFIERS == 1) && \
    (NANOPRINTF_USE_PRECISION_FORMAT_SPECIFIERS == 1)
    CheckConformance(" +01", "%+4.2i", 1);
#endif

#if (NANOPRINTF_USE_LARGE_FORMAT_SPECIFIERS == 1)
#if LLONG_MAX == 9223372036854775807ll
    CheckConformance("9223372036854775807", "%lli", LLONG_MAX);
#else
    CheckConformance("2147483647", "%lli", LLONG_MAX);
#endif

#if INTMAX_MAX == 9223372036854775807ll
    CheckConformance("9223372036854775807", "%ji", INTMAX_MAX);
#else
    CheckConformance("2147483647", "%ji", INTMAX_MAX);
#endif

#ifdef _MSC_VER
#define SSIZE_MAX LONG_MAX
#endif

#if SSIZE_MAX == 2147483647
    CheckConformance("2147483647", "%zi", SSIZE_MAX);
#else
    CheckConformance("9223372036854775807", "%zi", SSIZE_MAX);
#endif

#if PTRDIFF_MAX == 9223372036854775807ll
    CheckConformance("9223372036854775807", "%ti", PTRDIFF_MAX);
#else
    CheckConformance("2147483647", "%ti", PTRDIFF_MAX);
#endif
#endif
}

TEST(conformance, Octal) {
    CheckConformance("0", "%o", 0);
    CheckConformance("0", "%#o", 0);
    CheckConformance("37777777777", "%o", UINT_MAX);
    CheckConformance("17", "%ho", (1 << 29u) + 15u);
#if ULONG_MAX > UINT_MAX
    CheckConformance("40000000000", "%lo", (unsigned long)UINT_MAX + 1u);
#endif
    CheckConformance("2", "%hho", 258u);

#if NANOPRINTF_USE_FIELD_WIDTH_FORMAT_SPECIFIERS == 1
    CheckConformance("      2322", "%10o", 1234);
    CheckConformance("     02322", "%#10o", 1234);
    CheckConformance("0001", "%04o", 1);
    CheckConformance("0000", "%04o", 0);
    CheckConformance("0", "%+o", 0);
    CheckConformance("1", "%+o", 1);
    CheckConformance("   1", "%+4o", 1);
    CheckConformance("     1", "% 6o", 1);
#endif

#if NANOPRINTF_USE_PRECISION_FORMAT_SPECIFIERS == 1
    CheckConformance("", "%.0o", 0);
    CheckConformance("0", "%#.0o", 0);
#endif

#if (NANOPRINTF_USE_LARGE_FORMAT_SPECIFIERS == 1)
#if ULLONG_MAX == 01777777777777777777777llu
    CheckConformance("1777777777777777777777", "%llo", ULLONG_MAX);
#else
    CheckConformance("37777777777", "%llo", ULLONG_MAX);
#endif

#if UINTMAX_MAX == 01777777777777777777777llu
    CheckConformance("1777777777777777777777", "%jo", UINTMAX_MAX);
#else
    CheckConformance("37777777777", "%jo", UINTMAX_MAX);
#endif

#if SIZE_MAX == 01777777777777777777777llu
    CheckConformance("1777777777777777777777", "%zo", SIZE_MAX);
    CheckConformance("1777777777777777777777", "%to", SIZE_MAX);
#else
    CheckConformance("37777777777", "%zo", SIZE_MAX);
    CheckConformance("37777777777", "%to", SIZE_MAX);
#endif
#endif
}

TEST(conformance, Hex) {
    CheckConformance("0", "%x", 0);
    CheckConformance("12345678", "%x", 0x12345678);
    CheckConformance("ffffffff", "%x", UINT_MAX);
    CheckConformance("0", "%X", 0);
    CheckConformance("90ABCDEF", "%X", 0x90ABCDEF);
    CheckConformance("FFFFFFFF", "%X", UINT_MAX);
    CheckConformance("0", "%#x", 0);
    CheckConformance("0", "%+x", 0);
    CheckConformance("1", "%+x", 1);
    CheckConformance("7b", "%hx", (1 << 26u) + 123u);
#if ULONG_MAX > UINT_MAX
    CheckConformance("100000000", "%lx", (unsigned long)UINT_MAX + 1u);
#endif
    CheckConformance("b", "%hhx", 256u + 0xb);

#if NANOPRINTF_USE_FIELD_WIDTH_FORMAT_SPECIFIERS == 1
    CheckConformance("      1234", "%10x", 0x1234);
    CheckConformance("    0x1234", "%#10x", 0x1234);
    CheckConformance("0001", "%04u", 1);
    CheckConformance("0000", "%04u", 0);
    CheckConformance("     0", "% 6x", 0);
    CheckConformance("     1", "% 6x", 1);
#endif

#if NANOPRINTF_USE_PRECISION_FORMAT_SPECIFIERS == 1
    CheckConformance("", "%.0x", 0);
    CheckConformance("", "%.0X", 0);
    CheckConformance("", "%#.0X", 0);
#endif

#if (NANOPRINTF_USE_LARGE_FORMAT_SPECIFIERS == 1)
#if ULLONG_MAX == 0xffffffffffffffffllu
    CheckConformance("ffffffffffffffff", "%llx", ULLONG_MAX);
#else
    CheckConformance("ffffffff", "%llx", ULLONG_MAX);
#endif

#if UINTMAX_MAX == 0xffffffffffffffffllu
    CheckConformance("ffffffffffffffff", "%jx", UINTMAX_MAX);
#else
    CheckConformance("ffffffff", "%jx", UINTMAX_MAX);
#endif

#if SIZE_MAX == 0xffffffffffffffffllu
    CheckConformance("ffffffffffffffff", "%zx", SIZE_MAX);
    CheckConformance("ffffffffffffffff", "%tx", SIZE_MAX);
#else
    CheckConformance("ffffffff", "%zx", SIZE_MAX);
    CheckConformance("ffffffff", "%tx", SIZE_MAX);
#endif
#endif
}

#if !defined(_MSC_VER)  // Visual Studio prints "00000ABCDEF" (upper, no 0x)
TEST(conformance, Pointer) {
    // CheckConformance("%p", nullptr); implementation defined
    int x, *p = &x;
    char buf[32];
    snprintf(buf, sizeof(buf), "%p", (void *)p);
    CheckConformance(buf, "%p", p);
    // CheckConformance("%030p", p); 0 flag + 'p' is undefined
    // CheckConformance("%.30p", p); precision + 'p' is undefined

#if NANOPRINTF_USE_FIELD_WIDTH_FORMAT_SPECIFIERS == 1
    snprintf(buf, sizeof(buf), "%30p", (void *)p);
    CheckConformance(buf, "%30p", p);
#endif
}
#endif

#if NANOPRINTF_USE_WRITEBACK_FORMAT_SPECIFIERS == 1
namespace {
void dummy_putc(int, void *) {}
}  // namespace

TEST(conformance, WritebackInt) {
    int writeback = -1;
    npf_pprintf(dummy_putc, nullptr, "%n", &writeback);
    CHECK_EQUAL(0, writeback);
    npf_pprintf(dummy_putc, nullptr, " %n", &writeback);
    CHECK_EQUAL(1, writeback);
    npf_pprintf(dummy_putc, nullptr, "  %n", &writeback);
    CHECK_EQUAL(2, writeback);
    npf_pprintf(dummy_putc, nullptr, "%s%n", "abcd", &writeback);
    CHECK_EQUAL(4, writeback);
    npf_pprintf(dummy_putc, nullptr, "%u%s%n", 0, "abcd", &writeback);
    CHECK_EQUAL(5, writeback);
}

TEST(conformance, WritebackShort) {
    short writeback = -1;
    npf_pprintf(dummy_putc, nullptr, "1234%hn", &writeback);
    CHECK_EQUAL(4, writeback);
}

TEST(conformance, WritebackLong) {
    long writeback = -1;
    npf_pprintf(dummy_putc, nullptr, "1234567%ln", &writeback);
    CHECK_EQUAL(7, writeback);
}

TEST(conformance, WritebackChar) {
    signed char writeback = -1;
    npf_pprintf(dummy_putc, nullptr, "1234567%hhn", &writeback);
    CHECK_EQUAL(7, writeback);
}

#if NANOPRINTF_USE_LARGE_FORMAT_SPECIFIERS == 1
TEST(conformance, WritebackLongLong) {
    long long writeback = -1;
    npf_pprintf(dummy_putc, nullptr, "12345678%lln", &writeback);
    CHECK_EQUAL(8, writeback);
}

TEST(conformance, WritebackIntmax) {
    intmax_t writeback = -1;
    npf_pprintf(dummy_putc, nullptr, "12345678%jn", &writeback);
    CHECK_EQUAL(8, writeback);
}

TEST(conformance, WritebackSizeT) {
    intmax_t writeback = 100000;
    npf_pprintf(dummy_putc, nullptr, "12345678%zn", &writeback);
    CHECK_EQUAL(8, writeback);
}

TEST(conformance, WritebackPtrdiffT) {
    ptrdiff_t writeback = -1;
    npf_pprintf(dummy_putc, nullptr, "12345678%tn", &writeback);
    CHECK_EQUAL(8, writeback);
}
#endif
#endif

TEST(conformance, StarArgs) {
#if NANOPRINTF_USE_FIELD_WIDTH_FORMAT_SPECIFIERS == 1
    CheckConformance("         Z", "%*c", 10, 'Z');
    CheckConformance("5     ", "%*u", -6, 5);  // * fw < 0 => '-' and abs(fw)
#endif

#if NANOPRINTF_USE_PRECISION_FORMAT_SPECIFIERS == 1
    CheckConformance("01", "%.*i", 2, 1);
    CheckConformance("h", "%.*s", 1, "hello world");
    CheckConformance("1", "%.*u", -123, 1);  // ignore negative * precision
#endif

#if (NANOPRINTF_USE_FIELD_WIDTH_FORMAT_SPECIFIERS == 1) && \
    (NANOPRINTF_USE_PRECISION_FORMAT_SPECIFIERS == 1)
    CheckConformance("        07", "%*.*i", 10, 2, 7);
#endif
}

#if NANOPRINTF_USE_FLOAT_FORMAT_SPECIFIERS == 1
TEST(conformance, FloatNan) {
    char buf[32];
    npf_snprintf(buf, sizeof(buf), "%f", (double)NAN);
    CHECK(!strcmp(buf, "nan") || !strcmp(buf, "-nan"));
    npf_snprintf(buf, sizeof(buf), "%F", (double)NAN);
    CHECK(!strcmp(buf, "NAN") || !strcmp(buf, "-NAN"));
}

TEST(conformance, Float) {
    CheckConformance("inf", "%f", (double)INFINITY);
#if NANOPRINTF_USE_FIELD_WIDTH_FORMAT_SPECIFIERS == 1
    CheckConformance(" inf", "%4f", (double)INFINITY);
#endif
    CheckConformance("inf", "%.100f", (double)INFINITY);
    CheckConformance("INF", "%F", (double)INFINITY);
    CheckConformance("0.000000", "%f", 0.0);
    CheckConformance("0.00", "%.2f", 0.0);
    CheckConformance("1.0", "%.1f", 1.0);
    CheckConformance("1", "%.0f", 1.0);
    CheckConformance("1.", "%#.0f", 1.0);
    CheckConformance("1.00000000000", "%.11f", 1.0);
    CheckConformance("1.5", "%.1f", 1.5);
    CheckConformance("+1.5", "%+.1f", 1.5);
    CheckConformance("-1.5", "%.1f", -1.5);
    CheckConformance(" 1.5", "% .1f", 1.5);
#if NANOPRINTF_USE_FIELD_WIDTH_FORMAT_SPECIFIERS == 1
    CheckConformance(" 1.0", "%4.1f", 1.0);
    CheckConformance(" 1.500", "%6.3f", 1.5);
    CheckConformance("0001.500", "%08.3f", 1.5);
    CheckConformance("+001.500", "%+08.3f", 1.5);
    CheckConformance("-001.500", "%+08.3f", -1.5);
#endif
    CheckConformance("1.50000000000000000", "%.17f", 1.5);
    CheckConformance("0.003906", "%f", 0.00390625);
    CheckConformance("0.0039", "%.4f", 0.00390625);
    CheckConformance("0.00390625", "%.8f", 0.00390625);
    CheckConformance("0.00390625", "%.8Lf", (long double)0.00390625);
    CheckConformance("-0.00390625", "%.8f", -0.00390625);
    CheckConformance("-0.00390625", "%.8Lf", (long double)-0.00390625);
}
#endif

