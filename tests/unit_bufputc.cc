#include "unit_nanoprintf.h"

#include <string>

TEST_CASE("npf_bufputc") {
  npf_bufputc_ctx_t bpc;
  char buf[32];
  bpc.len = sizeof(buf);
  bpc.dst = buf;

  SUBCASE("Writes start at beginning of buffer") {
    npf_bufputc('A', &bpc);
    REQUIRE(buf[0] == 'A');
  }

  SUBCASE("Advances dst after write") {
    npf_bufputc('A', &bpc);
    REQUIRE(bpc.dst == buf + 1);
  }

  SUBCASE("Decrements len after write") {
    npf_bufputc('A', &bpc);
    REQUIRE(bpc.len == sizeof(buf) - 1);
  }

  SUBCASE("Writes to final byte of buffer") {
    buf[sizeof(buf) - 1] = '*';
    bpc.dst = &buf[sizeof(buf) - 1];
    bpc.len = 1;
    npf_bufputc('A', &bpc);
    REQUIRE(buf[sizeof(buf) - 1] == 'A');
  }

  SUBCASE("Doesn't write past final byte of buffer") {
    buf[3] = '*';
    bpc.dst = &buf[3];
    bpc.len = 0;
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
    REQUIRE(bpc.dst == buf + 6);
    buf[6] = '\0';
    REQUIRE(std::string(buf) == "ABCDEF");
  }
}
