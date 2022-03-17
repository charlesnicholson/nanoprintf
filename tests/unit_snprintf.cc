#include "unit_nanoprintf.h"

#include <cstring>
#include <string>

#if NANOPRINTF_HAVE_GCC_WARNING_PRAGMAS
  #pragma GCC diagnostic push
  #if NANOPRINTF_CLANG
    #pragma GCC diagnostic ignored "-Wformat-pedantic"
  #endif
  #pragma GCC diagnostic ignored "-Wformat-zero-length"
#endif

TEST_CASE("npf_snprintf") {
  char buf[128];
  buf[0] = '$';

  SUBCASE("zero-sized buffer") {
    REQUIRE(npf_snprintf(buf, 0, "abcd") == 4);
    REQUIRE(buf[0] == '$');
  }

  SUBCASE("empty string has null terminator") {
    memset(buf, 0xFF, sizeof(buf));
    npf_snprintf(buf, sizeof(buf), "");
    REQUIRE(buf[0] == '\0');
  }

  SUBCASE("string has null terminator") {
    memset(buf, 0xFF, sizeof(buf));
    npf_snprintf(buf, sizeof(buf), "Hello");
    REQUIRE(buf[5] == '\0');
    REQUIRE(std::string{buf} == "Hello");
  }

  SUBCASE("returns number of printed characters without null terminator") {
    REQUIRE(!npf_snprintf(buf, sizeof(buf), ""));
    REQUIRE(npf_snprintf(buf, sizeof(buf), "a") == 1);
    REQUIRE(npf_snprintf(buf, sizeof(buf), "%s", "abcdefg") == 7);
  }

  SUBCASE("prints string to buffer") {
    REQUIRE(npf_snprintf(buf, sizeof(buf), "hello %s", "world") == 11);
    REQUIRE(std::string{buf} == std::string{"hello world"});
  }

  SUBCASE("returns len if buf is null") {
    REQUIRE(npf_snprintf(nullptr, 0, "hello %s", "world") == 11);
  }

  SUBCASE("positive and negative integers in the same format string") {
    npf_snprintf(buf, sizeof(buf), "%i %u", -100, 100u);
    REQUIRE(std::string{"-100 100"} == std::string{buf});
  }

  SUBCASE("fills buffer fully") {
    REQUIRE(npf_snprintf(buf, 4, "abcd") == 4);
    REQUIRE(buf[0] == 'a');
    REQUIRE(buf[1] == 'b');
    REQUIRE(buf[2] == 'c');
    REQUIRE(buf[3] == 'd');
  }

  SUBCASE("doesn't write past end of buffer") {
    buf[3] = '*';
    REQUIRE(npf_snprintf(buf, 3, "abcd") == 4);
    REQUIRE(buf[0] == 'a');
    REQUIRE(buf[1] == 'b');
    REQUIRE(buf[2] == 'c');
    REQUIRE(buf[3] == '*');
  }
}
