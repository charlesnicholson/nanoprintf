#include "unit_nanoprintf.h"

#include <climits>
#include <cstring>
#include <string>
#include <vector>

#if NANOPRINTF_HAVE_GCC_WARNING_PRAGMAS
  #pragma GCC diagnostic push
  #if NANOPRINTF_CLANG
    #pragma GCC diagnostic ignored "-Wformat-pedantic"
    #pragma GCC diagnostic ignored "-Wold-style-cast"
  #else
    #pragma GCC diagnostic ignored "-Wformat-overflow"
  #endif
  #pragma GCC diagnostic ignored "-Wformat"
  #pragma GCC diagnostic ignored "-Wformat-zero-length"
  #pragma GCC diagnostic ignored "-Wformat-security"
#endif

struct Recorder {
  static void PutC(int c, void *ctx) {
    static_cast<Recorder*>(ctx)->calls.push_back((char)c);
  }

  std::string String() const {
    return calls.empty() ? std::string() : std::string(calls.begin(), calls.end());
  }

  std::vector<char> calls;
};

TEST_CASE("npf_vpprintf") {
  Recorder r;

  SUBCASE("empty string never calls callback") {
    REQUIRE(npf_pprintf(r.PutC, &r, "") == 0);
    REQUIRE(r.calls.empty());
  }

  SUBCASE("single character string") {
    REQUIRE(npf_pprintf(r.PutC, &r, "A") == 1);
    REQUIRE(r.calls.size() == 1);
    REQUIRE(r.calls[0] == 'A');
  }

  SUBCASE("string without format specifiers") {
    std::string const s{"Hello from nanoprintf!"};
    REQUIRE(npf_pprintf(r.PutC, &r, s.c_str()) == (int)s.size());
    REQUIRE(s == r.String());
  }

  SUBCASE("percent literal") {
    REQUIRE(npf_pprintf(r.PutC, &r, "%%") == 1);
    REQUIRE(r.String() == std::string{"%"});
  }

  SUBCASE("character single") {
    REQUIRE(npf_pprintf(r.PutC, &r, "%c", 'A') == 1);
    REQUIRE(r.String() == std::string{"A"});
  }

  SUBCASE("character multiple") {
    REQUIRE(npf_pprintf(r.PutC, &r, "%c%c%c%c", 'A', 'B', 'C', 'D') == 4);
    REQUIRE(r.String() == std::string{"ABCD"});
  }

  SUBCASE("string single") {
    REQUIRE(npf_pprintf(r.PutC, &r, "%s", "abcd") == 4);
    REQUIRE(r.String() == std::string{"abcd"});
  }

  SUBCASE("string empty") {
    REQUIRE(npf_pprintf(r.PutC, &r, "%s", "") == 0);
    REQUIRE(r.String().empty());
  }

  SUBCASE("string multiple") {
    REQUIRE(npf_pprintf(r.PutC, &r, "%s%s%s", "abcd", "e", "fgh") == 8);
    REQUIRE(r.String() == std::string{"abcdefgh"});
  }

  SUBCASE("string precision zero null pointer") {
    REQUIRE(npf_pprintf(r.PutC, &r, "%.0s", nullptr) == 0);
    REQUIRE(r.String() == std::string{""});
  }

  SUBCASE("signed int zero") {
    REQUIRE(npf_pprintf(r.PutC, &r, "%i", 0) == 1);
    REQUIRE(r.String() == std::string{"0"});
  }

  SUBCASE("signed int positive") {
    REQUIRE(npf_pprintf(r.PutC, &r, "%i", 123) == 3);
    REQUIRE(r.String() == std::string{"123"});
  }

  SUBCASE("signed int negative") {
    REQUIRE(npf_pprintf(r.PutC, &r, "%i", -456) == 4);
    REQUIRE(r.String() == std::string{"-456"});
  }

  SUBCASE("signed int max") {
    REQUIRE(npf_pprintf(r.PutC, &r, "%d", INT_MAX) == 10);
    REQUIRE(r.String() == std::string{"2147483647"});
  }

  SUBCASE("signed int min") {
    REQUIRE(npf_pprintf(r.PutC, &r, "%d", INT_MIN) == 11);
    REQUIRE(r.String() == std::string{"-2147483648"});
  }

  SUBCASE("unsigned int zero") {
    REQUIRE(npf_pprintf(r.PutC, &r, "%u", 0) == 1);
    REQUIRE(r.String() == std::string{"0"});
  }

  SUBCASE("unsigned int positive") {
    REQUIRE(npf_pprintf(r.PutC, &r, "%u", 45678) == 5);
    REQUIRE(r.String() == std::string{"45678"});
  }

  SUBCASE("unsigned int max u32") {
    REQUIRE(npf_pprintf(r.PutC, &r, "%u", 4294967295u) == 10);
    REQUIRE(r.String() == std::string{"4294967295"});
  }

  SUBCASE("octal zero") {
    REQUIRE(npf_pprintf(r.PutC, &r, "%o", 0) == 1);
    REQUIRE(r.String() == std::string{"0"});
  }

  SUBCASE("octal positive") {
    REQUIRE(npf_pprintf(r.PutC, &r, "%o", 1234) == 4);
    REQUIRE(r.String() == std::string{"2322"});
  }

  SUBCASE("octal max u32") {
    REQUIRE(npf_pprintf(r.PutC, &r, "%o", 4294967295u) == 11);
    REQUIRE(r.String() == std::string{"37777777777"});
  }

  SUBCASE("hex zero") {
    REQUIRE(npf_pprintf(r.PutC, &r, "%x", 0) == 1);
    REQUIRE(r.String() == std::string{"0"});
  }

  SUBCASE("hex single digit numeric") {
    REQUIRE(npf_pprintf(r.PutC, &r, "%x", 8) == 1);
    REQUIRE(r.String() == std::string{"8"});
  }

  SUBCASE("hex single digit alpha") {
    REQUIRE(npf_pprintf(r.PutC, &r, "%x", 0xc) == 1);
    REQUIRE(r.String() == std::string{"c"});
  }

  SUBCASE("hex large") {
    REQUIRE(npf_pprintf(r.PutC, &r, "%x", 0x9ABCDEF0) == 8);
    REQUIRE(r.String() == std::string{"9abcdef0"});
  }

  SUBCASE("hex max u32") {
    REQUIRE(npf_pprintf(r.PutC, &r, "%x", 0xFFFFFFFF) == 8);
    REQUIRE(r.String() == std::string{"ffffffff"});
  }

  SUBCASE("hex uppercase") {
    REQUIRE(npf_pprintf(r.PutC, &r, "%X", 0xabcdefab) == 8);
    REQUIRE(r.String() == std::string{"ABCDEFAB"});
  }

  SUBCASE("pointer null") {
    REQUIRE(npf_pprintf(r.PutC, &r, "%p", nullptr) == 3);
    REQUIRE(r.String() == std::string{"0x0"});
  }

  SUBCASE("pointer") {
    void *p;
    uintptr_t const u = 1234;
    memcpy(&p, &u, sizeof(p));
    int const n = npf_pprintf(r.PutC, &r, "%p", p);

    std::string const s = r.String();
    char const *sb = s.c_str();

    REQUIRE(n > 2);
    REQUIRE(*sb == '0');
    ++sb;
    REQUIRE(*sb == 'x');
    ++sb;

    for (int i = 2; i < n - 1; ++i) {
      char const c = *sb++;
      REQUIRE([c]{ return (c >= '0' && c <= '9') || (c >= 'a' && c <= 'f'); }());
    }
  }

  SUBCASE("float zero") {
    REQUIRE(npf_pprintf(r.PutC, &r, "%f", (double)0.f) == 8);
    REQUIRE(r.String() == std::string{"0.000000"});
  }

  SUBCASE("float one") {
    REQUIRE(npf_pprintf(r.PutC, &r, "%f", 1.0) == 8);
    REQUIRE(r.String() == std::string{"1.000000"});
  }

  SUBCASE("field width 1 has no effect") {
    REQUIRE(npf_pprintf(r.PutC, &r, "%1c", 'A') == 1);
    REQUIRE(r.String() == std::string{"A"});
  }

  SUBCASE("field width right justified by default") {
    REQUIRE(npf_pprintf(r.PutC, &r, "%2c", 'A') == 2);
    REQUIRE(r.String() == std::string{" A"});
  }

  SUBCASE("field width left justify flag") {
    REQUIRE(npf_pprintf(r.PutC, &r, "%-2c", 'A') == 2);
    REQUIRE(r.String() == std::string{"A "});
  }

  SUBCASE("prepend sign flag negative") {
    REQUIRE(npf_pprintf(r.PutC, &r, "%+d", -2) == 2);
    REQUIRE(r.String() == std::string{"-2"});
  }

  SUBCASE("prepend sign flag positive for signed conversion") {
    REQUIRE(npf_pprintf(r.PutC, &r, "%+d", 2) == 2);
    REQUIRE(r.String() == std::string{"+2"});
  }

  SUBCASE("prepend sign flag zero") {
    REQUIRE(npf_pprintf(r.PutC, &r, "%+d", 0) == 2);
    REQUIRE(r.String() == std::string{"+0"});
  }

  SUBCASE("prepend sign flag does nothing for unsigned conversion") {
    REQUIRE(npf_pprintf(r.PutC, &r, "%+u", 1) == 1);
    REQUIRE(r.String() == std::string{"1"});
  }

  SUBCASE("prepend space emits space instead of sign when positive") {
    REQUIRE(npf_pprintf(r.PutC, &r, "% d", 1) == 2);
    REQUIRE(r.String() == std::string{" 1"});
  }

  SUBCASE("prepend space emits minus sign when negative") {
    REQUIRE(npf_pprintf(r.PutC, &r, "% d", -1) == 2);
    REQUIRE(r.String() == std::string{"-1"});
  }

  SUBCASE("leading zero-pad flag does nothing on char (undefined)") {
    REQUIRE(npf_pprintf(r.PutC, &r, "%010c", 'A') == 1);
    REQUIRE(r.String() == std::string{"A"});
  }

  SUBCASE("leading zero-pad flag does nothing on string (undefined)") {
    REQUIRE(npf_pprintf(r.PutC, &r, "%0s", "ABCD") == 4);
    REQUIRE(r.String() == std::string{"ABCD"});
  }

  SUBCASE("alternative flag: hex doesn't prepend 0x if value is 0") {
    REQUIRE(npf_pprintf(r.PutC, &r, "%#x", 0) == 1);
    REQUIRE(r.String() == std::string{"0"});
  }

  SUBCASE("alternative flag: hex uppercase 0X") {
    REQUIRE(npf_pprintf(r.PutC, &r, "%#X", 1) == 3);
    REQUIRE(r.String() == std::string{"0X1"});
  }

  SUBCASE("alternative flag: hex lowercase 0x") {
    REQUIRE(npf_pprintf(r.PutC, &r, "%#x", 1) == 3);
    REQUIRE(r.String() == std::string{"0x1"});
  }

  SUBCASE("alternative flag: octal doesn't prepend 0 if value is 0") {
    REQUIRE(npf_pprintf(r.PutC, &r, "%#o", 0) == 1);
    REQUIRE(r.String() == std::string{"0"});
  }

  SUBCASE("alterinative flag: octal non-zero value") {
    REQUIRE(npf_pprintf(r.PutC, &r, "%#o", 2) == 2);
    REQUIRE(r.String() == std::string{"02"});
  }
}
