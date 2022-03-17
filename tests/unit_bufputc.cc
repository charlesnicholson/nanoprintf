#include "unit_nanoprintf.h"

#include <string>

TEST_CASE("npf_bufputc") {
  npf_bufputc_ctx_t bpc;
  char buf[32];
  bpc.cur = 0;
  bpc.len = sizeof(buf);
  bpc.dst = buf;

  SUBCASE("Writes start at beginning of buffer") {
    npf_bufputc('A', &bpc);
    REQUIRE(bpc.dst[0] == 'A');
  }

  SUBCASE("Increments cur after write") {
    npf_bufputc('A', &bpc);
    REQUIRE(bpc.cur == 1);
  }

  SUBCASE("Writes to final byte of buffer") {
    buf[sizeof(buf) - 1] = '*';
    bpc.cur = bpc.len - 1;
    npf_bufputc('A', &bpc);
    REQUIRE(buf[sizeof(buf) - 1] == 'A');
  }

  SUBCASE("Doesn't write past final byte of buffer") {
    buf[3] = '*';
    bpc.len = 3;
    bpc.cur = 3;
    npf_bufputc('A', &bpc);
    REQUIRE(buf[3] == '*');
  }

  SUBCASE("Multiple calls write sequential bytes") {
    npf_bufputc('A', &bpc);
    npf_bufputc('B', &bpc);
    npf_bufputc('C', &bpc);
    npf_bufputc('D', &bpc);
    npf_bufputc('E', &bpc);
    npf_bufputc('F', &bpc);
    REQUIRE(bpc.cur == 6);
    bpc.dst[6] = '\0';
    REQUIRE(std::string(bpc.dst) == "ABCDEF");
  }
}
