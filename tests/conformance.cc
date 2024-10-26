// CMake drives the conformance test with a large flag matrix.
// All of the nanoprintf configuration preprocessor symbols are injected.

#ifdef _MSC_VER
  #pragma warning(disable:4464) // relative include uses ..
  #pragma warning(disable:4514) // unreferenced inline function removed
  #pragma warning(disable:5039) // extern "c" throw
  #pragma warning(disable:4710) // function not inlined
  #pragma warning(disable:4711) // selected for inline
  #pragma warning(disable:5264) // const variable not used (shut up doctest)
#endif

#define NANOPRINTF_IMPLEMENTATION
#include "../nanoprintf.h"

#include <string>
#include <limits.h>
#include <cmath>

#if NANOPRINTF_HAVE_GCC_WARNING_PRAGMAS
  #pragma GCC diagnostic push
  #if NANOPRINTF_CLANG
    #pragma GCC diagnostic ignored "-Wc++98-compat-pedantic"
    #pragma GCC diagnostic ignored "-Wformat-pedantic"
    #pragma GCC diagnostic ignored "-Wformat-nonliteral"
    #pragma GCC diagnostic ignored "-Wold-style-cast"
    #ifndef __APPLE__
      #pragma GCC diagnostic ignored "-Wreserved-identifier"
      #pragma GCC diagnostic ignored "-Wunsafe-buffer-usage"
    #endif
  #endif
  #pragma GCC diagnostic ignored "-Wformat"
#endif

#include "doctest.h"

namespace {
void require_conform(const std::string& expected, char const *fmt, ...) {
  char buf[256];

  std::string sys_printf_result; {
    va_list args;
    va_start(args, fmt);
    vsnprintf(buf, sizeof(buf), fmt, args);
    va_end(args);
    buf[sizeof(buf)-1] = '\0';
    sys_printf_result = buf;
  }

  std::string npf_result; {
    va_list args;
    va_start(args, fmt);
    npf_vsnprintf(buf, sizeof(buf), fmt, args);
    va_end(args);
    buf[sizeof(buf)-1] = '\0';
    npf_result = buf;
  }

  REQUIRE(sys_printf_result == expected);
  REQUIRE(npf_result == expected);
}
}

TEST_CASE("conformance to system printf") {
  SUBCASE("percent") {
    require_conform("%", "%%");
#if NANOPRINTF_USE_FIELD_WIDTH_FORMAT_SPECIFIERS == 1
    require_conform("%", "%-%");
#endif // NANOPRINTF_USE_FIELD_WIDTH_FORMAT_SPECIFIERS
    require_conform("%", "% %");
    require_conform("%", "%+%");
    require_conform("%", "%#%");
    // require_conform("         %", "%10%"); clang adds width, gcc doesn't
    // require_conform("%         ", "%-10%"); clang adds -width, gcc doesn't
    // require_conform("         %", "%10.10%"); clang adds width + precision.
    // require_conform("%012%"); Undefined
  }

  SUBCASE("char") {
    // every char
    for (int i = CHAR_MIN; i < CHAR_MAX; ++i) {
        char output[2] = {(char)i, 0};
        require_conform(output, "%c", i);
    }

    require_conform("A", "%+c", 'A');
    require_conform("", "%+c", 0);

#if NANOPRINTF_USE_FIELD_WIDTH_FORMAT_SPECIFIERS == 1
    // right justify field width
    require_conform("A", "%1c", 'A');
    require_conform(" A", "%2c", 'A');
    require_conform("  A", "%3c", 'A');

    // left justify field width
    require_conform("A", "%-1c", 'A');
    require_conform("A ", "%-2c", 'A');
    require_conform("A  ", "%-3c", 'A');

    require_conform("     A", "% 6c", 'A');
    require_conform("   A", "%+4c", 'A');
#endif // NANOPRINTF_USE_FIELD_WIDTH_FORMAT_SPECIFIERS
  }

  SUBCASE("string") {
    require_conform("one", "%s", "one");
    require_conform("onetwothree", "%s%s%s", "one", "two", "three");

#if NANOPRINTF_USE_FIELD_WIDTH_FORMAT_SPECIFIERS == 1
    require_conform("       two", "%10s", "two");
    require_conform("B---       E", "B%-10sE", "---");
    require_conform("B       ---E", "B%10sE", "---");
#endif // NANOPRINTF_USE_FIELD_WIDTH_FORMAT_SPECIFIERS

#if NANOPRINTF_USE_PRECISION_FORMAT_SPECIFIERS == 1
    require_conform("thr", "%.3s", "three");
    require_conform("four", "%.100s", "four");
    // require_conform("abc", "%010s", "abc");  // undefined
    require_conform("", "%.0s", "five");
#endif // NANOPRINTF_USE_PRECISION_FORMAT_SPECIFIERS

#if (NANOPRINTF_USE_FIELD_WIDTH_FORMAT_SPECIFIERS == 1) && \
    (NANOPRINTF_USE_PRECISION_FORMAT_SPECIFIERS == 1)
    require_conform("       six", "%10.3s", "sixAAAAAAAA");
#endif
  }

  SUBCASE("unsigned int") {
    require_conform("0", "%u", 0);
    require_conform("4294967295", "%u", UINT_MAX);
    require_conform("0", "%+u", 0);
    require_conform("1", "%+u", 1);
    require_conform("13", "%hu", (1 << 21u) + 13u);  // "short" mod clips
#if ULONG_MAX > UINT_MAX
    require_conform("4294967296", "%lu", (unsigned long)UINT_MAX + 1u);
#endif
    require_conform("0", "%hhu", 256u);

#if NANOPRINTF_USE_FIELD_WIDTH_FORMAT_SPECIFIERS == 1
    require_conform("   1", "%+4u", 1);  // undefined but usually skips +
    require_conform("     0", "% 6u", 0);
#endif // NANOPRINTF_USE_FIELD_WIDTH_FORMAT_SPECIFIERS

#if NANOPRINTF_USE_PRECISION_FORMAT_SPECIFIERS == 1
    require_conform("", "%.0u", 0);
    require_conform("01", "%.2u", 1);
#endif // NANOPRINTF_USE_PRECISION_FORMAT_SPECIFIERS

#if (NANOPRINTF_USE_FIELD_WIDTH_FORMAT_SPECIFIERS == 1) && \
    (NANOPRINTF_USE_PRECISION_FORMAT_SPECIFIERS == 1)
    require_conform("    0123", "%8.4u", 123);
#endif

#if (NANOPRINTF_USE_LARGE_FORMAT_SPECIFIERS == 1)
#if ULLONG_MAX == 18446744073709551615llu
    require_conform("18446744073709551615", "%llu", ULLONG_MAX);
#else
    require_conform("4294967295", "%llu", ULLONG_MAX);
#endif
#if UINTMAX_MAX == 18446744073709551615llu
    require_conform("18446744073709551615", "%ju", UINTMAX_MAX);
#else
    require_conform("4294967295", "%ju", UINTMAX_MAX);
#endif
#if SIZE_MAX == 18446744073709551615llu
    require_conform("18446744073709551615", "%zu", SIZE_MAX);
    require_conform("18446744073709551615", "%tu", SIZE_MAX);
#else
    require_conform("4294967295", "%zu", SIZE_MAX);
    require_conform("4294967295", "%tu", SIZE_MAX);
#endif
#endif
  }

  SUBCASE("signed int") {
    require_conform("-2147483648", "%i", INT_MIN);
    require_conform("0", "%i", 0);
    require_conform("2147483647", "%i", INT_MAX);
    require_conform("-1", "%+i", -1);
    require_conform("+0", "%+i", 0);
    require_conform("+1", "%+i", 1);
    // require_conform("%.-123i", 400); xcode libc doesn't ignore negative
    require_conform("-128", "%hhi", 128);

#if NANOPRINTF_USE_FIELD_WIDTH_FORMAT_SPECIFIERS == 1
    require_conform("  -1", "% 4i", -1);
    require_conform("   0", "% 4i", 0);
    require_conform("   1", "% 4i", 1);
    require_conform("  +1", "%+4i", 1);
    require_conform("  +0", "%+4i", 0);
    require_conform("  -1", "%+4i", -1);
    require_conform("0001", "%04i", 1);
    require_conform("0000", "%04i", 0);
    require_conform("-001", "%04i", -1);
    require_conform("+001", "%+04i", 1);
#endif // NANOPRINTF_USE_FIELD_WIDTH_FORMAT_SPECIFIERS

#if NANOPRINTF_USE_PRECISION_FORMAT_SPECIFIERS == 1
    require_conform("", "%.0i", 0);
    require_conform("+", "%+.0i", 0);
    require_conform("+01", "%+.2i", 1);
#endif // NANOPRINTF_USE_PRECISION_FORMAT_SPECIFIERS

#if (NANOPRINTF_USE_FIELD_WIDTH_FORMAT_SPECIFIERS == 1) && \
    (NANOPRINTF_USE_PRECISION_FORMAT_SPECIFIERS == 1)
    require_conform(" +01", "%+4.2i", 1);
    require_conform(" 0", "%02.1d", 0);
#endif

#if (NANOPRINTF_USE_LARGE_FORMAT_SPECIFIERS == 1)
#if LLONG_MAX == 9223372036854775807ll
    require_conform("9223372036854775807", "%lli", LLONG_MAX);
#else
    require_conform("2147483647", "%lli", LLONG_MAX);
#endif

#if INTMAX_MAX == 9223372036854775807ll
    require_conform("9223372036854775807", "%ji", INTMAX_MAX);
#else
    require_conform("2147483647", "%ji", INTMAX_MAX);
#endif

#ifdef _MSC_VER
#define SSIZE_MAX LONG_MAX
#endif // _MSC_VER

#if SSIZE_MAX == 2147483647
    require_conform("2147483647", "%zi", SSIZE_MAX);
#else
    require_conform("9223372036854775807", "%zi", SSIZE_MAX);
#endif

#if PTRDIFF_MAX == 9223372036854775807ll
    require_conform("9223372036854775807", "%ti", PTRDIFF_MAX);
#else
    require_conform("2147483647", "%ti", PTRDIFF_MAX);
#endif
#endif // NANOPRINTF_USE_LARGE_FORMAT_SPECIFIERS
  }

  SUBCASE("octal") {
    require_conform("0", "%o", 0);
    require_conform("0", "%#o", 0);
    require_conform("37777777777", "%o", UINT_MAX);
    require_conform("17", "%ho", (1 << 29u) + 15u);
#if ULONG_MAX > UINT_MAX
    require_conform("40000000000", "%lo", (unsigned long)UINT_MAX + 1u);
#endif
    require_conform("2", "%hho", 258u);

#if NANOPRINTF_USE_FIELD_WIDTH_FORMAT_SPECIFIERS == 1
    require_conform("      2322", "%10o", 1234);
    require_conform("     02322", "%#10o", 1234);
    require_conform("0001", "%04o", 1);
    require_conform("0000", "%04o", 0);
    require_conform("0", "%+o", 0);
    require_conform("1", "%+o", 1);
    require_conform("   1", "%+4o", 1);
    require_conform("     1", "% 6o", 1);
#endif // NANOPRINTF_USE_FIELD_WIDTH_FORMAT_SPECIFIERS

#if NANOPRINTF_USE_PRECISION_FORMAT_SPECIFIERS == 1
    require_conform("", "%.0o", 0);
    require_conform("0", "%#.0o", 0);
#endif // NANOPRINTF_USE_PRECISION_FORMAT_SPECIFIERS

#if (NANOPRINTF_USE_LARGE_FORMAT_SPECIFIERS == 1)
#if ULLONG_MAX == 01777777777777777777777llu
    require_conform("1777777777777777777777", "%llo", ULLONG_MAX);
#else
    require_conform("37777777777", "%llo", ULLONG_MAX);
#endif

#if UINTMAX_MAX == 01777777777777777777777llu
    require_conform("1777777777777777777777", "%jo", UINTMAX_MAX);
#else
    require_conform("37777777777", "%jo", UINTMAX_MAX);
#endif

#if SIZE_MAX == 01777777777777777777777llu
    require_conform("1777777777777777777777", "%zo", SIZE_MAX);
    require_conform("1777777777777777777777", "%to", SIZE_MAX);
#else
    require_conform("37777777777", "%zo", SIZE_MAX);
    require_conform("37777777777", "%to", SIZE_MAX);
#endif
#endif // NANOPRINTF_USE_LARGE_FORMAT_SPECIFIERS
  }

  SUBCASE("hex") {
    require_conform("0", "%x", 0);
    require_conform("12345678", "%x", 0x12345678);
    require_conform("ffffffff", "%x", UINT_MAX);
    require_conform("0", "%X", 0);
    require_conform("90ABCDEF", "%X", 0x90ABCDEF);
    require_conform("FFFFFFFF", "%X", UINT_MAX);
    require_conform("0", "%#x", 0);
    require_conform("0", "%+x", 0);
    require_conform("1", "%+x", 1);
    require_conform("7b", "%hx", (1 << 26u) + 123u);
#if ULONG_MAX > UINT_MAX
    require_conform("100000000", "%lx", (unsigned long)UINT_MAX + 1u);
#endif
    require_conform("b", "%hhx", 256u + 0xb);

#if NANOPRINTF_USE_FIELD_WIDTH_FORMAT_SPECIFIERS == 1
    require_conform("      1234", "%10x", 0x1234);
    require_conform("    0x1234", "%#10x", 0x1234);
    require_conform("0001", "%04u", 1);
    require_conform("0000", "%04u", 0);
    require_conform("     0", "% 6x", 0);
    require_conform("     1", "% 6x", 1);
#endif // NANOPRINTF_USE_FIELD_WIDTH_FORMAT_SPECIFIERS

#if NANOPRINTF_USE_PRECISION_FORMAT_SPECIFIERS == 1
    require_conform("", "%.0x", 0);
    require_conform("", "%.0X", 0);
    require_conform("", "%#.0X", 0);
#endif // NANOPRINTF_USE_PRECISION_FORMAT_SPECIFIERS

#if (NANOPRINTF_USE_LARGE_FORMAT_SPECIFIERS == 1)
#if ULLONG_MAX == 0xffffffffffffffffllu
    require_conform("ffffffffffffffff", "%llx", ULLONG_MAX);
#else
    require_conform("ffffffff", "%llx", ULLONG_MAX);
#endif

#if UINTMAX_MAX == 0xffffffffffffffffllu
    require_conform("ffffffffffffffff", "%jx", UINTMAX_MAX);
#else
    require_conform("ffffffff", "%jx", UINTMAX_MAX);
#endif

#if SIZE_MAX == 0xffffffffffffffffllu
    require_conform("ffffffffffffffff", "%zx", SIZE_MAX);
    require_conform("ffffffffffffffff", "%tx", SIZE_MAX);
#else
    require_conform("ffffffff", "%zx", SIZE_MAX);
    require_conform("ffffffff", "%tx", SIZE_MAX);
#endif
#endif // NANOPRINTF_USE_LARGE_FORMAT_SPECIFIERS
  }

#if !defined(_MSC_VER)  // Visual Studio prints "00000ABCDEF" (upper, no 0x)
  SUBCASE("pointer") {
    // require_conform("%p", nullptr); implementation defined
    int x, *p = &x;
    char buf[32];
    snprintf(buf, sizeof(buf), "%p", (void *)p);
    require_conform(buf, "%p", p);
    // require_conform("%030p", p); 0 flag + 'p' is undefined
    // require_conform("%.30p", p); precision + 'p' is undefined

#if NANOPRINTF_USE_FIELD_WIDTH_FORMAT_SPECIFIERS == 1
    snprintf(buf, sizeof(buf), "%30p", (void *)p);
    require_conform(buf, "%30p", p);
#endif // NANOPRINTF_USE_FIELD_WIDTH_FORMAT_SPECIFIERS
  }
#endif // _MSC_VER

#if NANOPRINTF_USE_WRITEBACK_FORMAT_SPECIFIERS == 1
  SUBCASE("writeback int") {
    int writeback = -1;
    npf_pprintf(+[](int, void*) {}, nullptr, "%n", &writeback);
    REQUIRE(writeback == 0);
    npf_pprintf(+[](int, void*) {}, nullptr, " %n", &writeback);
    REQUIRE(writeback == 1);
    npf_pprintf(+[](int, void*) {}, nullptr, "  %n", &writeback);
    REQUIRE(writeback == 2);
    npf_pprintf(+[](int, void*) {}, nullptr, "%s%n", "abcd", &writeback);
    REQUIRE(writeback == 4);
    npf_pprintf(+[](int, void*) {}, nullptr, "%u%s%n", 0, "abcd", &writeback);
    REQUIRE(writeback == 5);
  }

  SUBCASE("writeback short") {
    short writeback = -1;
    npf_pprintf(+[](int, void*) {}, nullptr, "1234%hn", &writeback);
    REQUIRE(writeback == 4);
  }

  SUBCASE("writeback long") {
    long writeback = -1;
    npf_pprintf(+[](int, void*) {}, nullptr, "1234567%ln", &writeback);
    REQUIRE(writeback == 7);
  }

  SUBCASE("writeback char") {
    signed char writeback = -1;
    npf_pprintf(+[](int, void*) {}, nullptr, "1234567%hhn", &writeback);
    REQUIRE(writeback == 7);
  }

#if NANOPRINTF_USE_LARGE_FORMAT_SPECIFIERS == 1
  SUBCASE("writeback long long") {
    long long writeback = -1;
    npf_pprintf(+[](int, void*) {}, nullptr, "12345678%lln", &writeback);
    REQUIRE(writeback == 8);
  }

  SUBCASE("writeback intmax_t") {
    intmax_t writeback = -1;
    npf_pprintf(+[](int, void*) {}, nullptr, "12345678%jn", &writeback);
    REQUIRE(writeback == 8);
  }

  SUBCASE("writeback size_t") {
    intmax_t writeback = 100000;
    npf_pprintf(+[](int, void*) {}, nullptr, "12345678%zn", &writeback);
    REQUIRE(writeback == 8);
  }

  SUBCASE("writeback ptrdiff_t") {
    ptrdiff_t writeback = -1;
    npf_pprintf(+[](int, void*) {}, nullptr, "12345678%tn", &writeback);
    REQUIRE(writeback == 8);
  }
#endif // NANOPRINTF_USE_LARGE_FORMAT_SPECIFIERS
#endif // NANOPRINTF_USE_WRITEBACK_FORMAT_SPECIFIERS

  SUBCASE("star args") {
#if NANOPRINTF_USE_FIELD_WIDTH_FORMAT_SPECIFIERS == 1
    require_conform("         Z", "%*c", 10, 'Z');
    require_conform("5     ", "%*u", -6, 5);  // * fw < 0 => '-' and abs(fw)
#endif // NANOPRINTF_USE_FIELD_WIDTH_FORMAT_SPECIFIERS

#if NANOPRINTF_USE_PRECISION_FORMAT_SPECIFIERS == 1
    require_conform("01", "%.*i", 2, 1);
    require_conform("h", "%.*s", 1, "hello world");
    require_conform("1", "%.*u", -123, 1);  // ignore negative * precision
#endif // NANOPRINTF_USE_PRECISION_FORMAT_SPECIFIERS

#if (NANOPRINTF_USE_FIELD_WIDTH_FORMAT_SPECIFIERS == 1) && \
    (NANOPRINTF_USE_PRECISION_FORMAT_SPECIFIERS == 1)
    require_conform("        07", "%*.*i", 10, 2, 7);
#endif
  }

#if NANOPRINTF_USE_FLOAT_FORMAT_SPECIFIERS == 1
  SUBCASE("float NaN") {
    std::string const lowercase_nan = []{
      char buf[32];
      REQUIRE(npf_snprintf(buf, sizeof(buf), "%f", (double)NAN) == 3);
      return std::string{buf};
    }();

    // doctest can't do || inside REQUIRE
    if (lowercase_nan != "nan") {
      REQUIRE(lowercase_nan == "-nan");
    } else {
      REQUIRE(lowercase_nan == "nan");
    }

    std::string const uppercase_nan = []{
      char buf[32];
      REQUIRE(npf_snprintf(buf, sizeof(buf), "%F", (double)NAN) == 3);
      return std::string{buf};
    }();

    // doctest can't do || inside REQUIRE
    if (uppercase_nan != "NAN") {
      REQUIRE(uppercase_nan == "-NAN");
    } else {
      REQUIRE(uppercase_nan == "NAN");
    }
  }

  SUBCASE("float") {
    require_conform("inf", "%f", (double)INFINITY);
#if NANOPRINTF_USE_FIELD_WIDTH_FORMAT_SPECIFIERS == 1
    require_conform(" inf", "%4f", (double)INFINITY);
#endif // NANOPRINTF_USE_FIELD_WIDTH_FORMAT_SPECIFIERS
    require_conform("inf", "%.100f", (double)INFINITY);
    require_conform("inf", "%.10f", (double)INFINITY);
    require_conform("inf", "%.10e", (double)INFINITY);
    require_conform("inf", "%.10g", (double)INFINITY);
    require_conform("inf", "%.10a", (double)INFINITY);
    require_conform("INF", "%F", (double)INFINITY);
    require_conform("0.000000", "%f", 0.0);
    require_conform("0.00", "%.2f", 0.0);
    require_conform("1.0", "%.1f", 1.0);
    require_conform("1", "%.0f", 1.0);
    require_conform("1.", "%#.0f", 1.0);
    require_conform("1.00000000000", "%.11f", 1.0);
    require_conform("1.5", "%.1f", 1.5);
    require_conform("+1.5", "%+.1f", 1.5);
    require_conform("-1.5", "%.1f", -1.5);
    require_conform(" 1.5", "% .1f", 1.5);
#if NANOPRINTF_USE_FIELD_WIDTH_FORMAT_SPECIFIERS == 1
    require_conform(" 1.0", "%4.1f", 1.0);
    require_conform(" 1.500", "%6.3f", 1.5);
    require_conform("0001.500", "%08.3f", 1.5);
    require_conform("+001.500", "%+08.3f", 1.5);
    require_conform("-001.500", "%+08.3f", -1.5);
#endif // NANOPRINTF_USE_FIELD_WIDTH_FORMAT_SPECIFIERS
    require_conform("1.50000000000000000", "%.17f", 1.5);
    require_conform("0.003906", "%f", 0.00390625);
    require_conform("0.0039", "%.4f", 0.00390625);
    require_conform("0.00390625", "%.8f", 0.00390625);
    require_conform("0.00390625", "%.8Lf", (long double)0.00390625);
    require_conform("-0.00390625", "%.8f", -0.00390625);
    require_conform("-0.00390625", "%.8Lf", (long double)-0.00390625);
  }
#endif // NANOPRINTF_USE_FLOAT_FORMAT_SPECIFIERS
}
