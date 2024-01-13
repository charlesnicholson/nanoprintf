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
    REQUIRE(npf_snprintf(buf, 5, "abcd") == 4);
    REQUIRE(buf[0] == 'a');
    REQUIRE(buf[1] == 'b');
    REQUIRE(buf[2] == 'c');
    REQUIRE(buf[3] == 'd');
    REQUIRE(buf[4] == '\0');
  }

  SUBCASE("last byte is null terminator") {
    REQUIRE(npf_snprintf(buf, 4, "abcd") == 4);
    REQUIRE(buf[0] == 'a');
    REQUIRE(buf[1] == 'b');
    REQUIRE(buf[2] == 'c');
    REQUIRE(buf[3] == '\0');
  }

  SUBCASE("terminates before end of buffer") {
    buf[3] = '*';
    REQUIRE(npf_snprintf(buf, 3, "abcd") == 4);
    REQUIRE(buf[0] == 'a');
    REQUIRE(buf[1] == 'b');
    REQUIRE(buf[2] == '\0');
    REQUIRE(buf[3] == '*');
  }

  SUBCASE("string trimming") {
    buf[0] = '@';
    buf[7] = '*';
    buf[8] = '!';

    SUBCASE("zero-sized buffer") {
      REQUIRE(npf_snprintf(buf, 0, "abc") == 3);
      REQUIRE(buf[0] == '@');
    }

    SUBCASE("small string") {
      REQUIRE(npf_snprintf(buf, 8, "abc") == 3);
      REQUIRE(std::string{buf} == "abc");
    }

    SUBCASE("exact fit string") {
      REQUIRE(npf_snprintf(buf, 8, "1234567") == 7);
      REQUIRE(buf[7] == '\0');
      REQUIRE(std::string{buf} == "1234567");
    }

    SUBCASE("if the null terminator doesn't fit, the string is trimmed") {
      REQUIRE(npf_snprintf(buf, 8, "12345678") == 8);
      REQUIRE(std::string{buf} == "1234567");
      REQUIRE(buf[8] == '!');
    }

    SUBCASE("if the string contents are too long, the string is trimmed") {
      REQUIRE(npf_snprintf(buf, 8, "123456789") == 9);
      REQUIRE(std::string{buf} == "1234567");
      REQUIRE(buf[8] == '!');
    }

    SUBCASE("null buffer with non-null length doesn't get terminated") {
      npf_snprintf(nullptr, 4, "abcd");
    }
  }
}
