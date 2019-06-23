#include "CppUTest/TestHarness.h"
#include "nanoprintf_in_unit_tests.h"

TEST_GROUP(npf__bufputc) {
    void setup() override {
        bpc.cur = 0;
        bpc.len = sizeof(buf);
        bpc.dst = buf;
    }

    npf__bufputc_ctx_t bpc;
    char buf[32];
};

TEST(npf__bufputc, WritesCToZeroWhenCurZero) {
    npf__bufputc('A', &bpc);
    CHECK_EQUAL('A', bpc.dst[0]);
}

TEST(npf__bufputc, IncrementsCurAfterWrite) {
    npf__bufputc('A', &bpc);
    CHECK_EQUAL(1, bpc.cur);
}

TEST(npf__bufputc, ReturnsCWhenWriteSucceded) {
    CHECK_EQUAL('A', npf__bufputc('A', &bpc));
}

TEST(npf__bufputc, ReturnsEofWhenCurIsLen) {
    bpc.cur = bpc.len;
    CHECK_EQUAL(NPF_EOF, npf__bufputc('A', &bpc));
}
