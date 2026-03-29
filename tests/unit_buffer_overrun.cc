#include "unit_nanoprintf.h"

#include <cstdarg>
#include <cstring>
#include <string>

namespace {

constexpr int BUF_PAD = 16;
constexpr int BUF_MAX = 512;
constexpr unsigned char FILLER = 0xFF;

bool is_filled(char const *b, unsigned char value, size_t n_bytes) {
  // Branchless: accumulate any mismatch via XOR + OR.
  // All-equal is the common case; no early-bailout branch in the loop.
  unsigned res = 0;
  for (size_t i = 0; i < n_bytes; ++i) {
    res |= (unsigned)(unsigned char)b[i] ^ (unsigned)value;
  }
  return !res;
}

void check_v(char const *expected, int buf_sz, char const *fmt, va_list args) {
  REQUIRE(buf_sz > 0);
  REQUIRE(buf_sz <= BUF_MAX);

  char buf[BUF_PAD + BUF_MAX + BUF_PAD];
  memset(buf, FILLER, sizeof(buf));
  char *dst = buf + BUF_PAD;

  int const n = npf_vsnprintf(dst, (size_t)buf_sz, fmt, args);

  // strnlen never reads past the buffer; detects missing terminator.
  size_t const len = strnlen(dst, (size_t)buf_sz);
  REQUIRE(len < (size_t)buf_sz);

  // Add terminator so subsequent checks produce nice error messages, not crashes.
  dst[buf_sz - 1] = '\0';

  // Return value: as-if length (may exceed buf_sz on truncation).
  REQUIRE(n >= 0);
  if (n < buf_sz) {
    REQUIRE((size_t)n == len);
  } else {
    REQUIRE(len == (size_t)(buf_sz - 1));
  }

  // Check expected content.
  if (expected) {
    REQUIRE(std::string(dst) == std::string(expected));
  }

  // Restore the safety terminator, clear bytes npf wrote, then verify nothing else changed.
  dst[buf_sz - 1] = (char)FILLER;
  memset(dst, FILLER, len + 1);
  REQUIRE(is_filled(buf, FILLER, sizeof(buf)));
}

void check_string(char const *expected, char const *fmt, ...) {
  va_list args;
  va_start(args, fmt);
  check_v(expected, 256, fmt, args);
  va_end(args);
}

void check_string_n(char const *expected, int buf_sz, char const *fmt, ...) {
  va_list args;
  va_start(args, fmt);
  check_v(expected, buf_sz, fmt, args);
  va_end(args);
}

} // namespace

// ---------------------------------------------------------------------------
// Basic sanity
// ---------------------------------------------------------------------------

TEST_CASE("overrun - basic") {
  SUBCASE("empty format") { check_string("", ""); }
  SUBCASE("single char") { check_string("x", "x"); }
  SUBCASE("short string") { check_string("hello", "hello"); }
  SUBCASE("format with arg") { check_string("abc123", "abc%d", 123); }

  SUBCASE("exact fit (255 chars + NUL = 256)") {
    std::string s(255, 'A');
    check_string(s.c_str(), "%s", s.c_str());
  }
}

// ---------------------------------------------------------------------------
// Long input strings to %s
// ---------------------------------------------------------------------------

TEST_CASE("overrun - long %s") {
  SUBCASE("string longer than buffer") {
    std::string s(300, 'B');
    check_string(s.substr(0, 255).c_str(), "%s", s.c_str());
  }

  SUBCASE("string much longer than buffer") {
    std::string s(1000, 'C');
    check_string(s.substr(0, 255).c_str(), "%s", s.c_str());
  }

  SUBCASE("one byte over") {
    std::string s(256, 'E');
    check_string(s.substr(0, 255).c_str(), "%s", s.c_str());
  }

  SUBCASE("string with small buffer") {
    std::string s(50, 'D');
    check_string_n(std::string(7, 'D').c_str(), 8, "%s", s.c_str());
  }
}

// ---------------------------------------------------------------------------
// Large widths
// ---------------------------------------------------------------------------

TEST_CASE("overrun - large width") {
  SUBCASE("width larger than buffer") {
    check_string(nullptr, "%300d", 1);
  }

  SUBCASE("width exactly at buffer limit") {
    check_string(nullptr, "%255d", 1);
  }

  SUBCASE("width with small buffer") {
    check_string_n(nullptr, 8, "%20d", 1);
  }

  SUBCASE("large width with string") {
    check_string(nullptr, "%300s", "hi");
  }

  SUBCASE("left-justified large width") {
    check_string(nullptr, "%-300d", 42);
  }

  SUBCASE("zero-padded large width") {
    check_string(nullptr, "%0300d", 42);
  }
}

// ---------------------------------------------------------------------------
// Large precisions
// ---------------------------------------------------------------------------

TEST_CASE("overrun - large precision") {
  SUBCASE("precision larger than buffer") {
    check_string(nullptr, "%.300d", 1);
  }

  SUBCASE("precision with small buffer") {
    check_string_n(nullptr, 8, "%.20d", 1);
  }

  SUBCASE("precision limiting string") {
    check_string("hello", "%.5s", "hello world");
  }

  SUBCASE("large float precision") {
    check_string(nullptr, "%.100f", 3.14);
  }

  SUBCASE("width and precision both large") {
    check_string(nullptr, "%300.200d", 42);
  }
}

// ---------------------------------------------------------------------------
// Multiple specifiers that only overflow when concatenated
// ---------------------------------------------------------------------------

TEST_CASE("overrun - multiple specifiers concatenated") {
  SUBCASE("three strings that overflow together") {
    std::string a(100, 'A');
    std::string b(100, 'B');
    std::string c(100, 'C');
    std::string expected = a + b + c.substr(0, 55);
    check_string(expected.c_str(), "%s%s%s", a.c_str(), b.c_str(), c.c_str());
  }

  SUBCASE("integers that overflow in small buffer") {
    check_string_n(nullptr, 16, "%d%d%d%d%d",
                   12345, 67890, 11111, 22222, 33333);
  }

  SUBCASE("mixed types that overflow") {
    std::string s(200, 'X');
    check_string(nullptr, "%s%d", s.c_str(), 1234567890);
  }

  SUBCASE("each fits alone but not together") {
    check_string_n(nullptr, 16, "%d-%d-%d-%d", 1234, 5678, 9012, 3456);
  }

  SUBCASE("literals plus conversions overflow") {
    check_string_n(nullptr, 16, "abc%ddef%dghi%d", 111, 222, 333);
  }
}

// ---------------------------------------------------------------------------
// Long literal characters in the format string
// ---------------------------------------------------------------------------

TEST_CASE("overrun - long literal format string") {
  SUBCASE("all literals exceeding buffer") {
    std::string fmt(300, 'L');
    check_string(fmt.substr(0, 255).c_str(), fmt.c_str());
  }

  SUBCASE("literals fill buffer exactly") {
    std::string fmt(255, 'R');
    check_string(fmt.c_str(), fmt.c_str());
  }

  SUBCASE("literals one byte over") {
    std::string fmt(256, 'S');
    check_string(fmt.substr(0, 255).c_str(), fmt.c_str());
  }

  SUBCASE("literals plus conversion at end (fits)") {
    std::string prefix(250, 'P');
    std::string fmt = prefix + "%d";
    check_string((prefix + "42").c_str(), fmt.c_str(), 42);
  }

  SUBCASE("literals cause conversion to be truncated") {
    std::string prefix(254, 'Q');
    std::string fmt = prefix + "%d";
    check_string((prefix + "4").c_str(), fmt.c_str(), 42);
  }

  SUBCASE("literals fill buffer, conversion entirely truncated") {
    std::string prefix(255, 'T');
    std::string fmt = prefix + "%d";
    check_string(prefix.c_str(), fmt.c_str(), 42);
  }
}

// ---------------------------------------------------------------------------
// Small buffers (edge cases)
// ---------------------------------------------------------------------------

TEST_CASE("overrun - small buffers") {
  SUBCASE("buffer size 1") {
    check_string_n("", 1, "hello");
  }

  SUBCASE("buffer size 2") {
    check_string_n("h", 2, "hello");
  }

  SUBCASE("buffer size 2 with conversion") {
    check_string_n("4", 2, "%d", 42);
  }

  SUBCASE("buffer size 1 with %s") {
    check_string_n("", 1, "%s", "anything");
  }

  SUBCASE("buffer size 3 with long format") {
    check_string_n("he", 3, "hello %s world", "big");
  }

  SUBCASE("buffer size 4 with exact truncation") {
    check_string_n("abc", 4, "abcdef");
  }

  SUBCASE("buffer size 4 with exact fit") {
    check_string_n("abc", 4, "abc");
  }
}
