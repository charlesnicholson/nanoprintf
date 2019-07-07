#include "nanoprintf_in_unit_tests.h"

#include "CppUTest/TestHarness.h"

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
    CheckConformance("%", "%-%");
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

    // right justify field width
    CheckConformance("A", "%1c", 'A');
    CheckConformance(" A", "%2c", 'A');
    CheckConformance("  A", "%3c", 'A');

    // left justify field width
    CheckConformance("A", "%-1c", 'A');
    CheckConformance("A ", "%-2c", 'A');
    CheckConformance("A  ", "%-3c", 'A');

    CheckConformance("A", "%+c", 'A');
    CheckConformance("", "%+c", 0);
    CheckConformance("     A", "% 6c", 'A');
    CheckConformance("   A", "%+4c", 'A');
}

TEST(conformance, Strings) {
    CheckConformance("one", "%s", "one");
    CheckConformance("       two", "%10s", "two");
    CheckConformance("thr", "%.3s", "three");
    CheckConformance("four", "%.100s", "four");
    // CheckConformance("abc", "%010s", "abc");  // undefined
    CheckConformance("", "%.0s", "five");
    CheckConformance("       six", "%10.3s", "sixAAAAAAAA");
}

TEST(conformance, UnsignedInt) {
    CheckConformance("0", "%u", 0);
    CheckConformance("4294967295", "%u", UINT_MAX);
    CheckConformance("0", "%+u", 0);
    CheckConformance("", "%.0u", 0);
    CheckConformance("1", "%+u", 1);
    CheckConformance("   1", "%+4u", 1);  // undefined but usually skips +
    CheckConformance("     0", "% 6u", 0);
    CheckConformance("01", "%.2u", 1);
    CheckConformance("    0123", "%8.4u", 123);
    CheckConformance("13", "%hu", (1 << 21u) + 13u);  // "short" mod clips
    CheckConformance("4294967296", "%lu",
                     (unsigned long)UINT_MAX + 1);  // assume ul > u
    CheckConformance("0", "%hhu", 256u);
    CheckConformance("18446744073709551615", "%llu", ULLONG_MAX);
    CheckConformance("18446744073709551615", "%ju", UINTMAX_MAX);
    CheckConformance("18446744073709551615", "%zu", SIZE_MAX);
    CheckConformance("18446744073709551615", "%tu", SIZE_MAX);
}

TEST(conformance, SignedInt) {
    CheckConformance("-2147483648", "%i", INT_MIN);
    CheckConformance("0", "%i", 0);
    CheckConformance("", "%.0i", 0);
    CheckConformance("+", "%+.0i", 0);
    CheckConformance("2147483647", "%i", INT_MAX);
    CheckConformance("-1", "%+i", -1);
    CheckConformance("+0", "%+i", 0);
    CheckConformance("+1", "%+i", 1);
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
    CheckConformance("+01", "%+.2i", 1);
    CheckConformance(" +01", "%+4.2i", 1);
    // CheckConformance("%.-123i", 400); xcode libc doesn't ignore negative
    CheckConformance("2147483648", "%lu",
                     (long)INT_MAX + 1);  // assume l > i
    CheckConformance("-128", "%hhi", 128);
    CheckConformance("9223372036854775807", "%lli", LLONG_MAX);
    CheckConformance("9223372036854775807", "%ji", INTMAX_MAX);
    CheckConformance("9223372036854775807", "%zi", INTMAX_MAX);
    CheckConformance("9223372036854775807", "%ti", PTRDIFF_MAX);
}

TEST(conformance, Octal) {
    CheckConformance("0", "%o", 0);
    CheckConformance("0", "%#o", 0);
    CheckConformance("", "%.0o", 0);
    CheckConformance("0", "%#.0o", 0);
    CheckConformance("37777777777", "%o", UINT_MAX);
    CheckConformance("      2322", "%10o", 1234);
    CheckConformance("     02322", "%#10o", 1234);
    CheckConformance("0001", "%04o", 1);
    CheckConformance("0000", "%04o", 0);
    CheckConformance("0", "%+o", 0);
    CheckConformance("1", "%+o", 1);
    CheckConformance("   1", "%+4o", 1);
    CheckConformance("     1", "% 6o", 1);
    CheckConformance("17", "%ho", (1 << 29u) + 15u);
    CheckConformance("40000000000", "%lo",
                     (unsigned long)UINT_MAX + 1);  // assume ul > u
    CheckConformance("2", "%hho", 258u);
    CheckConformance("1777777777777777777777", "%llo", ULLONG_MAX);
    CheckConformance("1777777777777777777777", "%jo", UINTMAX_MAX);
    CheckConformance("1777777777777777777777", "%zo", SIZE_MAX);
    CheckConformance("1777777777777777777777", "%to", SIZE_MAX);
}

TEST(conformance, Hex) {
    CheckConformance("0", "%x", 0);
    CheckConformance("", "%.0x", 0);
    CheckConformance("12345678", "%x", 0x12345678);
    CheckConformance("ffffffff", "%x", UINT_MAX);
    CheckConformance("0", "%X", 0);
    CheckConformance("", "%.0X", 0);
    CheckConformance("", "%#.0X", 0);
    CheckConformance("90ABCDEF", "%X", 0x90ABCDEF);
    CheckConformance("FFFFFFFF", "%X", UINT_MAX);
    CheckConformance("0", "%#x", 0);
    CheckConformance("      1234", "%10x", 0x1234);
    CheckConformance("    0x1234", "%#10x", 0x1234);
    CheckConformance("0001", "%04u", 1);
    CheckConformance("0000", "%04u", 0);
    CheckConformance("     0", "% 6x", 0);
    CheckConformance("     1", "% 6x", 1);
    CheckConformance("0", "%+x", 0);
    CheckConformance("1", "%+x", 1);
    CheckConformance("7b", "%hx", (1 << 26u) + 123u);
    CheckConformance("100000000", "%lx",
                     (unsigned long)UINT_MAX + 1);  // assume ul > u
    CheckConformance("b", "%hhx", 256u + 0xb);
    CheckConformance("ffffffffffffffff", "%llx", ULLONG_MAX);
    CheckConformance("ffffffffffffffff", "%jx", UINTMAX_MAX);
    CheckConformance("ffffffffffffffff", "%zx", SIZE_MAX);
    CheckConformance("ffffffffffffffff", "%tx", SIZE_MAX);
}

TEST(conformance, Pointer) {
    // CheckConformance("%p", nullptr); implementation defined
    int x, *p = &x;
    char buf[32];
    snprintf(buf, sizeof(buf), "%p", (void *)p);
    CheckConformance(buf, "%p", p);
    snprintf(buf, sizeof(buf), "%30p", (void *)p);
    CheckConformance(buf, "%30p", p);
    // CheckConformance("%030p", p); 0 flag + 'p' is undefined
    // CheckConformance("%.30p", p); precision + 'p' is undefined
}

namespace {
int dummy_putc(int, void *) { return 1; }
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

TEST(conformance, StarArgs) {
    CheckConformance("         Z", "%*c", 10, 'Z');
    CheckConformance("01", "%.*i", 2, 1);
    CheckConformance("        07", "%*.*i", 10, 2, 7);
    CheckConformance("h", "%.*s", 1, "hello world");
    CheckConformance("1", "%.*u", -123, 1);    // ignore negative * precision
    CheckConformance("5     ", "%*u", -6, 5);  // * fw < 0 => '-' and abs(fw)
}

TEST(conformance, FloatNan) {
    char buf[32];
    npf_snprintf(buf, sizeof(buf), "%f", 0.0 / 0.0);
    CHECK(!strcmp(buf, "nan") || !strcmp(buf, "-nan"));
    npf_snprintf(buf, sizeof(buf), "%F", 0.0 / 0.0);
    CHECK(!strcmp(buf, "NAN") || !strcmp(buf, "-NAN"));
}

TEST(conformance, Float) {
    CheckConformance("inf", "%f", 1.0 / 0.0);
    CheckConformance("INF", "%F", 1.0 / 0.0);
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
    CheckConformance(" 1.500", "%6.3f", 1.5);
    CheckConformance("0001.500", "%08.3f", 1.5);
    CheckConformance("+001.500", "%+08.3f", 1.5);
    CheckConformance("-001.500", "%+08.3f", -1.5);
    CheckConformance("0.00390625", "%.8f", 0.00390625);
    CheckConformance("0.00390625", "%.8Lf", (long double)0.00390625);
}
