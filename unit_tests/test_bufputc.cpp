#include "nanoprintf_in_unit_tests.h"

#include "CppUTest/TestHarness.h"

TEST_GROUP(npf_bufputc) {
    void setup() override {
        bpc.cur = 0;
        bpc.len = sizeof(buf);
        bpc.dst = buf;
    }

    npf_bufputc_ctx_t bpc;
    char buf[32];
};

TEST(npf_bufputc, WritesCToZeroWhenCurZero) {
    npf_bufputc('A', &bpc);
    CHECK_EQUAL('A', bpc.dst[0]);
}

TEST(npf_bufputc, IncrementsCurAfterWrite) {
    npf_bufputc('A', &bpc);
    CHECK_EQUAL(1, bpc.cur);
}

TEST(npf_bufputc, DoesntWriteToFinalByte) {
    buf[sizeof(buf) - 1] = '*';
    bpc.cur = bpc.len - 1;
    npf_bufputc('A', &bpc);
    CHECK_EQUAL('*', buf[sizeof(buf) - 1]);
}

TEST(npf_bufputc, MultipleCallsWriteSequentially) {
    npf_bufputc('A', &bpc);
    npf_bufputc('B', &bpc);
    npf_bufputc('C', &bpc);
    npf_bufputc('D', &bpc);
    npf_bufputc('E', &bpc);
    npf_bufputc('F', &bpc);
    CHECK_EQUAL(6, bpc.cur);
    bpc.dst[6] = '\0';
    STRCMP_EQUAL("ABCDEF", bpc.dst);
}
