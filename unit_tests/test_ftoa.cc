#include "CppUTest/TestHarness.h"
#include "nanoprintf_in_unit_tests.h"

TEST_GROUP(ftoa){};

TEST(ftoa, derp) {
    char buf[128];
    npf__ftoa(buf, 1.0f);
    npf__ftoa(buf, 1.1234f);
    npf__ftoa(buf, 0.f);
    npf__ftoa(buf, -1.0f);
    npf__ftoa(buf, 1000.0f);
    npf__ftoa(buf, 12345.0f);
}
