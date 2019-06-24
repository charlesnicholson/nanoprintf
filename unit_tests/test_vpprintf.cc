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
        return calls.empty() ? std::string()
                             : std::string(calls.begin(), calls.end() - 1);
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

// Conversion Specifiers

TEST_GROUP(Percent) { Recorder r; };

TEST(Percent, Literal) {
    CHECK_EQUAL(2, npf_pprintf(r.PutC, &r, "%%"));
    CHECK_EQUAL("%", r.String());
}

TEST_GROUP(Char) { Recorder r; };

TEST(Char, Single) {
    CHECK_EQUAL(2, npf_pprintf(r.PutC, &r, "%c", 'A'));
    CHECK_EQUAL("A", r.String());
}

TEST(Char, Multiple) {
    CHECK_EQUAL(5, npf_pprintf(r.PutC, &r, "%c%c%c%c", 'A', 'B', 'C', 'D'));
    CHECK_EQUAL("ABCD", r.String());
}

TEST_GROUP(String) { Recorder r; };

TEST(String, Single) {
    CHECK_EQUAL(5, npf_pprintf(r.PutC, &r, "%s", "abcd"));
    CHECK_EQUAL("abcd", r.String());
}

TEST(String, Empty) {
    CHECK_EQUAL(1, npf_pprintf(r.PutC, &r, "%s", ""));
    CHECK_EQUAL("", r.String());
}

TEST(String, Multiple) {
    CHECK_EQUAL(9, npf_pprintf(r.PutC, &r, "%s%s%s", "abcd", "e", "fgh"));
    CHECK_EQUAL("abcdefgh", r.String());
}

TEST_GROUP(SignedInt) { Recorder r; };

TEST(SignedInt, Zero) {
    CHECK_EQUAL(2, npf_pprintf(r.PutC, &r, "%i", 0));
    CHECK_EQUAL("0", r.String());
}

TEST(SignedInt, Positive) {
    CHECK_EQUAL(4, npf_pprintf(r.PutC, &r, "%i", 123));
    CHECK_EQUAL("123", r.String());
}

TEST(SignedInt, Negative) {
    CHECK_EQUAL(5, npf_pprintf(r.PutC, &r, "%i", -456));
    CHECK_EQUAL("-456", r.String());
}

TEST(SignedInt, IntMax) {
    CHECK_EQUAL(11, npf_pprintf(r.PutC, &r, "%d", INT_MAX));
    CHECK_EQUAL("2147483647", r.String());
}

TEST(SignedInt, IntMin) {
    npf_pprintf(r.PutC, &r, "%d", INT_MIN);
    CHECK_EQUAL("-2147483648", r.String());
}

TEST_GROUP(UnsignedInt) { Recorder r; };

TEST(UnsignedInt, Zero) {
    CHECK_EQUAL(2, npf_pprintf(r.PutC, &r, "%u", 0));
    CHECK_EQUAL("0", r.String());
}

TEST(UnsignedInt, Positive) {
    CHECK_EQUAL(6, npf_pprintf(r.PutC, &r, "%u", 45678));
    CHECK_EQUAL("45678", r.String());
}

TEST(UnsignedInt, UIntMax32) {
    CHECK_EQUAL(11, npf_pprintf(r.PutC, &r, "%u", 4294967295u));
    CHECK_EQUAL("4294967295", r.String());
}

TEST_GROUP(Octal) { Recorder r; };

TEST(Octal, Zero) {
    CHECK_EQUAL(2, npf_pprintf(r.PutC, &r, "%o", 0));
    CHECK_EQUAL("0", r.String());
}

TEST(Octal, Positive) {
    CHECK_EQUAL(5, npf_pprintf(r.PutC, &r, "%o", 1234));
    CHECK_EQUAL("2322", r.String());
}

TEST(Octal, UIntMax32) {
    CHECK_EQUAL(12, npf_pprintf(r.PutC, &r, "%o", 4294967295u));
    CHECK_EQUAL("37777777777", r.String());
}

// Field Width

TEST_GROUP(FieldWidth) { Recorder r; };

TEST(FieldWidth, OneDoesNothing) {
    CHECK_EQUAL(2, npf_pprintf(r.PutC, &r, "%1c", 'A'));
    CHECK_EQUAL("A", r.String());
}

TEST(FieldWidth, RightJustifiedByDefault) {
    CHECK_EQUAL(3, npf_pprintf(r.PutC, &r, "%2c", 'A'));
    CHECK_EQUAL(" A", r.String());
}

TEST(FieldWidth, FlagLeftJustified) {
    CHECK_EQUAL(3, npf_pprintf(r.PutC, &r, "%-2c", 'A'));
    CHECK_EQUAL("A ", r.String());
}
