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
    /*
    // CheckConformance("%012%"); Undefined
    CheckConformance("         %", "%10%");
    // CheckConformance("%", "%.10%"); gcc prints precision, clang doesn't
    CheckConformance("%         ", "%-10%");
    CheckConformance("         %", "%10.10%");
    */
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
    CheckConformance("1", "%+u", 1);
    CheckConformance("   1", "%+4u", 1);  // undefined but usually skips +
    CheckConformance("     0", "% 6u", 0);
    CheckConformance("01", "%.2u", 1);
    CheckConformance("    0123", "%8.4u", 123);
}

TEST(conformance, SignedInt) {
    CheckConformance("-2147483648", "%i", INT_MIN);
    CheckConformance("0", "%i", 0);
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
}

TEST(conformance, Octal) {
    CheckConformance("0", "%o", 0);
    CheckConformance("0", "%#o", 0);
    CheckConformance("37777777777", "%o", UINT_MAX);
    CheckConformance("      2322", "%10o", 1234);
    CheckConformance("     02322", "%#10o", 1234);
    CheckConformance("0001", "%04o", 1);
    CheckConformance("0000", "%04o", 0);
    CheckConformance("0", "%+o", 0);
    CheckConformance("1", "%+o", 1);
    CheckConformance("   1", "%+4o", 1);
    CheckConformance("     1", "% 6o", 1);
}

TEST(conformance, Hex) {
    CheckConformance("0", "%x", 0);
    CheckConformance("12345678", "%x", 0x12345678);
    CheckConformance("ffffffff", "%x", UINT_MAX);
    CheckConformance("0", "%X", 0);
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
}

TEST(conformance, Pointer) {
    // CheckConformance("%p", nullptr); implementation defined
    int x, *p = &x;
    char buf[32];
    snprintf(buf, sizeof(buf), "%p", (void *)p);
    CheckConformance(buf, "%p", p);
    snprintf(buf, sizeof(buf), "%30p", (void *)p);
    CheckConformance(buf, "%30p", p);
    // CheckConformance("%030p", p); 0x comes before zero pad
    // CheckConformance("%.30p", p); 0x comes before precision
}

TEST(conformance, BytesWritten) {
    // CheckConformance("%n"); often unimplemented
}

TEST(conformance, StarArgs) {
    CheckConformance("         Z", "%*c", 10, 'Z');
    CheckConformance("01", "%.*i", 2, 1);
    CheckConformance("        07", "%*.*i", 10, 2, 7);
    CheckConformance("h", "%.*s", 1, "hello world");
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
}
