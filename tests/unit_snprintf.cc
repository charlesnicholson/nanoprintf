#include "unit_nanoprintf.h"
#include "doctest.h"

#include <string>
#include <iostream>

TEST_CASE("npf_snprintf") {
  char buf[128];

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
