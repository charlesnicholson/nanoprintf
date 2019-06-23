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
    CheckConformance("%0%");
    CheckConformance("%+%");
    CheckConformance("%#%");
    CheckConformance("%.10%");
    // CheckConformance("%-10%");
}

TEST(conformance, Char) {
    for (int i = CHAR_MIN; i < CHAR_MAX; ++i) {
        CheckConformance("%c", i);
    }

    /*
    for (int precision = 0; precision < 20; ++precision) {
        char fmt[8];
        sprintf(fmt, "%%%dc", precision);
        CheckConformance(fmt, 'A');
    }
    */
}

TEST(conformance, UnsignedInt) {
    CheckConformance("%u", 0);
    CheckConformance("%u", UINT_MAX);
}
