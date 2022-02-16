#include "unit_nanoprintf.h"
#include "doctest.h"

#include <cstring>
#include <string>
#include <iostream>

#if NANOPRINTF_CLANG_OR_GCC_PAST_4_6
  #pragma GCC diagnostic push
  #pragma GCC diagnostic ignored "-Wc++98-compat-pedantic"
  #pragma GCC diagnostic ignored "-Wold-style-cast"
#endif

TEST_CASE("npf_snprintf") {
  char buf[128];

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
}
