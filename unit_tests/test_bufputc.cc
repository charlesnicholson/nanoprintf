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

TEST(npf__bufputc, MultipleCallsWriteSequentially) {
    CHECK_EQUAL('A', npf__bufputc('A', &bpc));
    CHECK_EQUAL('B', npf__bufputc('B', &bpc));
    CHECK_EQUAL('C', npf__bufputc('C', &bpc));
    CHECK_EQUAL('D', npf__bufputc('D', &bpc));
    CHECK_EQUAL('E', npf__bufputc('E', &bpc));
    CHECK_EQUAL('F', npf__bufputc('F', &bpc));
    CHECK_EQUAL(6, bpc.cur);
    bpc.dst[6] = '\0';
    STRCMP_EQUAL("ABCDEF", bpc.dst);
}
