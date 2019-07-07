#include "nanoprintf_in_unit_tests.h"

#include "CppUTest/TestHarness.h"

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

TEST(npf__bufputc, DoesntWriteToFinalByte) {
    buf[sizeof(buf) - 1] = '*';
    bpc.cur = bpc.len - 1;
    npf__bufputc('A', &bpc);
    CHECK_EQUAL('*', buf[sizeof(buf) - 1]);
}

TEST(npf__bufputc, MultipleCallsWriteSequentially) {
    npf__bufputc('A', &bpc);
    npf__bufputc('B', &bpc);
    npf__bufputc('C', &bpc);
    npf__bufputc('D', &bpc);
    npf__bufputc('E', &bpc);
    npf__bufputc('F', &bpc);
    CHECK_EQUAL(6, bpc.cur);
    bpc.dst[6] = '\0';
    STRCMP_EQUAL("ABCDEF", bpc.dst);
}
