#include "CppUTest/TestHarness.h"
#include "nanoprintf_in_unit_tests.h"

TEST_GROUP(conformance){};

namespace {
char conformance_buf[256];
void CheckConformance(char const *fmt, ...) {
    std::string actual;
    {
        va_list v;
        va_start(v, fmt);
        npf_vsnprintf(conformance_buf, sizeof(conformance_buf), fmt, v);
        va_end(v);
        actual = conformance_buf;
    }
    std::string expected;
    {
        va_list v;
        va_start(v, fmt);
        vsnprintf(conformance_buf, sizeof(conformance_buf), fmt, v);
        va_end(v);
        expected = conformance_buf;
    }
    CHECK_EQUAL(expected, actual);
}
}  // namespace

TEST(conformance, Percent) {
    CheckConformance("%%");
    CheckConformance("%-%");
    CheckConformance("% %");
    // CheckConformance("%012%"); Undefined
    CheckConformance("%+%");
    CheckConformance("%#%");
    CheckConformance("%10%");
    CheckConformance("%.10%");
    CheckConformance("%-10%");
    CheckConformance("%10.10%");
}

TEST(conformance, Char) {
    // every char
    for (int i = CHAR_MIN; i < CHAR_MAX; ++i) {
        CheckConformance("%c", i);
    }

    // right justify field width
    for (int precision = 0; precision < 20; ++precision) {
        char fmt[8];
        sprintf(fmt, "%%%dc", precision);
        CheckConformance(fmt, 'A');
    }

    // left justify field width
    for (int precision = 0; precision < 20; ++precision) {
        char fmt[8];
        sprintf(fmt, "%%-%dc", precision);
        CheckConformance(fmt, 'A');
    }

    CheckConformance("%+c", 'A');
    CheckConformance("%+c", 0);
    CheckConformance("%+c", -1);
    CheckConformance("% 6c", 'A');
    CheckConformance("% 6c", 0);
    CheckConformance("% 6c", -1);
    CheckConformance("%+4c", 1);
    CheckConformance("%+4c", 0);
    CheckConformance("%+4c", -1);
}

TEST(conformance, Strings) {
    CheckConformance("%10s", "hello");
    CheckConformance("%10s", "hello world!");
    //    CheckConformance("%.10s", "hello world this string is > 10");
}

TEST(conformance, UnsignedInt) {
    CheckConformance("%u", 0);
    CheckConformance("%u", UINT_MAX);
    CheckConformance("%+u", 0);
    CheckConformance("%+u", 1);
    CheckConformance("%+4u", 1);
    CheckConformance("%+4u", 0);
    CheckConformance("% 6u", 0);
    CheckConformance("% 6u", 1);
}

TEST(conformance, Octal) {
    CheckConformance("%o", 0);
    CheckConformance("%#o", 0);
    CheckConformance("%o", UINT_MAX);
    CheckConformance("%10o", 1234);
    CheckConformance("%#10o", 1234);
    CheckConformance("%04o", 1);
    CheckConformance("%04o", 0);
    CheckConformance("%+o", 0);
    CheckConformance("%+o", 1);
    CheckConformance("%+4o", 1);
    CheckConformance("%+4o", 0);
    CheckConformance("% 6o", 0);
    CheckConformance("% 6o", 1);
}

TEST(conformance, SignedInt) {
    CheckConformance("%i", INT_MIN);
    CheckConformance("%i", 0);
    CheckConformance("%i", INT_MAX);
    CheckConformance("%+i", -1);
    CheckConformance("%+i", 0);
    CheckConformance("%+i", 1);
    CheckConformance("% 6i", -1);
    CheckConformance("% 6i", 0);
    CheckConformance("% 6i", 1);
    CheckConformance("%+4i", 1);
    CheckConformance("%+4i", 0);
    CheckConformance("%+4i", -1);
    CheckConformance("%04i", 1);
    CheckConformance("%04i", 0);
    CheckConformance("%04i", -1);
    // CheckConformance("%.-123i", 400); xcode libc doesn't ignore negative
}

TEST(conformance, Hex) {
    CheckConformance("%x", 0);
    CheckConformance("%x", 0x12345678);
    CheckConformance("%x", UINT_MAX);
    CheckConformance("%X", 0);
    CheckConformance("%X", 0x90ABCDEF);
    CheckConformance("%X", UINT_MAX);
    CheckConformance("%#x", 0);
    CheckConformance("%10x", 0x1234);
    CheckConformance("%#10x", 0x1234);
    CheckConformance("%04u", 1);
    CheckConformance("%04u", 0);
    CheckConformance("% 6x", 0);
    CheckConformance("% 6x", 1);
    CheckConformance("%+x", 0);
    CheckConformance("%+x", 1);
}

TEST(conformance, Pointer) {
    // CheckConformance("%p", nullptr); implementation defined
    int x, *p = &x;
    CheckConformance("%p", p);
    CheckConformance("%30p", p);
    CheckConformance("% 30p", p);
    // CheckConformance("%030p", p); 0x comes before zero pad
    // CheckConformance("%.30p", p); 0x comes before precision
}
