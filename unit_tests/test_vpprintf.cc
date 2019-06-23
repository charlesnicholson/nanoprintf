#include "CppUTest/TestHarness.h"
#include "nanoprintf_in_unit_tests.h"

#include <string>
#include <vector>

struct Recorder {
    static int PutC(int c, void *ctx) {
        Recorder &r = *(Recorder *)ctx;
        r.calls.push_back(c);
        return (--r.eof_count <= 0) ? NPF_EOF : c;
    }
    std::string String() const {
        return std::string(calls.begin(), calls.end() - 1);  // trim \0
    }
    std::vector<int> calls;
    int eof_count = 10000;
};

TEST_GROUP(npf_vpprintf) { Recorder r; };

TEST(npf_vpprintf, WritesNullTermWhenEmptyString) {
    CHECK_EQUAL(1, npf_pprintf(r.PutC, &r, ""));
    CHECK_EQUAL(1, r.calls.size());
    CHECK_EQUAL('\0', r.calls[0]);
}

TEST(npf_vpprintf, ReturnsZeroOnFirstPutEof) {
    r.eof_count = 0;
    CHECK_EQUAL(0, npf_pprintf(r.PutC, &r, ""));
}

TEST(npf_vpprintf, PutsOnlyCharacterThenNullTerm) {
    CHECK_EQUAL(2, npf_pprintf(r.PutC, &r, "A"));
    CHECK_EQUAL(2, r.calls.size());
    CHECK_EQUAL('A', r.calls[0]);
    CHECK_EQUAL('\0', r.calls[1]);
}

TEST(npf_vpprintf, PutsEntireStringThenNullTerm_NoFormatSpecifiers) {
    char const *s = "Hello from nanoprintf!";
    CHECK_EQUAL((int)(strlen(s) + 1), npf_pprintf(r.PutC, &r, s));
    CHECK_EQUAL(s, r.String());
}

TEST(npf_vpprintf, PercentLiteral) {
    CHECK_EQUAL(2, npf_pprintf(r.PutC, &r, "%%"));
    CHECK_EQUAL("%", r.String());
}

TEST(npf_vpprintf, Char) {
    CHECK_EQUAL(2, npf_pprintf(r.PutC, &r, "%c", 'A'));
    CHECK_EQUAL("A", r.String());
}

TEST(npf_vpprintf, CharMany) {
    CHECK_EQUAL(5, npf_pprintf(r.PutC, &r, "%c%c%c%c", 'A', 'B', 'C', 'D'));
    CHECK_EQUAL("ABCD", r.String());
}

TEST(npf_vpprintf, String) {
    CHECK_EQUAL(5, npf_pprintf(r.PutC, &r, "%s", "abcd"));
    CHECK_EQUAL("abcd", r.String());
}

TEST(npf_vpprintf, StringEmpty) {
    CHECK_EQUAL(1, npf_pprintf(r.PutC, &r, "%s", ""));
    CHECK_EQUAL("", r.String());
}

TEST(npf_vpprintf, StringMany) {
    CHECK_EQUAL(9, npf_pprintf(r.PutC, &r, "%s%s%s", "abcd", "e", "fgh"));
    CHECK_EQUAL("abcdefgh", r.String());
}

TEST(npf_vpprintf, SignedInt_Zero) {
    CHECK_EQUAL(2, npf_pprintf(r.PutC, &r, "%i", 0));
    CHECK_EQUAL("0", r.String());
}

TEST(npf_vpprintf, SignedInt_Positive) {
    CHECK_EQUAL(4, npf_pprintf(r.PutC, &r, "%i", 123));
    CHECK_EQUAL("123", r.String());
}

TEST(npf_vpprintf, SignedInt_Negative) {
    CHECK_EQUAL(5, npf_pprintf(r.PutC, &r, "%i", -456));
    CHECK_EQUAL("-456", r.String());
}

TEST(npf_vpprintf, SignedInt_IntMax) {
    CHECK_EQUAL(11, npf_pprintf(r.PutC, &r, "%d", INT_MAX));
    CHECK_EQUAL("2147483647", r.String());
}

TEST(npf_vpprintf, SignedInt_IntMin) {
    npf_pprintf(r.PutC, &r, "%d", INT_MIN);
    CHECK_EQUAL("-2147483648", r.String());
}

TEST(npf_vpprintf, UnsignedInt_Zero) {
    CHECK_EQUAL(2, npf_pprintf(r.PutC, &r, "%u", 0));
    CHECK_EQUAL("0", r.String());
}

