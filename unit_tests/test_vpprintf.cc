#include "nanoprintf_in_unit_tests.h"

#include "CppUTest/TestHarness.h"

#include <cstring>
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

TEST_GROUP(Hex) { Recorder r; };

TEST(Hex, Zero) {
    CHECK_EQUAL(2, npf_pprintf(r.PutC, &r, "%x", 0));
    CHECK_EQUAL("0", r.String());
}

TEST(Hex, LessThanA) {
    CHECK_EQUAL(2, npf_pprintf(r.PutC, &r, "%x", 8));
    CHECK_EQUAL("8", r.String());
}

TEST(Hex, SingleDigitGreaterThan9) {
    CHECK_EQUAL(2, npf_pprintf(r.PutC, &r, "%x", 0xc));
    CHECK_EQUAL("c", r.String());
}

TEST(Hex, Large) {
    CHECK_EQUAL(9, npf_pprintf(r.PutC, &r, "%x", 0x9ABCDEF0));
    CHECK_EQUAL("9abcdef0", r.String());
}

TEST(Hex, UIntMax32) {
    CHECK_EQUAL(9, npf_pprintf(r.PutC, &r, "%x", 0xFFFFFFFF));
    CHECK_EQUAL("ffffffff", r.String());
}

TEST(Hex, Uppercase) {
    CHECK_EQUAL(9, npf_pprintf(r.PutC, &r, "%X", 0xabcdefab));
    CHECK_EQUAL("ABCDEFAB", r.String());
}

TEST_GROUP(Pointer) { Recorder r; };

TEST(Pointer, Null) {
    CHECK_EQUAL(7, npf_pprintf(r.PutC, &r, "%p", nullptr));
    CHECK_EQUAL("(null)", r.String());
}

TEST(Pointer, NonNull) {
    void *p;
    uintptr_t const u = 1234;
    memcpy(&p, &u, sizeof(p));
    int const n = npf_pprintf(r.PutC, &r, "%p", p);

    std::string const s = r.String();
    char const *sb = s.c_str();

    CHECK_COMPARE(n, >, 2);
    CHECK_EQUAL('0', *sb);
    ++sb;
    CHECK_EQUAL('x', *sb);
    ++sb;

    for (int i = 2; i < n - 1; ++i) {
        char const c = *sb++;
        CHECK((c >= '0' && c <= '9') || (c >= 'a' && c <= 'f'));
    }
}

// Position

TEST_GROUP(Position) { Recorder r; };

TEST(Position, Zero) {
    CHECK_EQUAL(2, npf_pprintf(r.PutC, &r, "%n"));
    CHECK_EQUAL("0", r.String());
}

TEST(Position, One) {
    CHECK_EQUAL(3, npf_pprintf(r.PutC, &r, " %n"));
    CHECK_EQUAL(" 1", r.String());
}

TEST(Position, AfterString) {
    CHECK_EQUAL(7, npf_pprintf(r.PutC, &r, "%s%n", "hello"));
    CHECK_EQUAL("hello5", r.String());
}

// Float

TEST_GROUP(Float) { Recorder r; };

TEST(Float, Zero) {
    CHECK_EQUAL(9, npf_pprintf(r.PutC, &r, "%f", (double)0.f));
    CHECK_EQUAL("0.000000", r.String());
}

TEST(Float, One) {
    CHECK_EQUAL(9, npf_pprintf(r.PutC, &r, "%f", 1.0));
    CHECK_EQUAL("1.000000", r.String());
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

// Prepend Sign flag

TEST_GROUP(PrependSignFlag) { Recorder r; };

TEST(PrependSignFlag, Negative) {
    CHECK_EQUAL(3, npf_pprintf(r.PutC, &r, "%+d", -2));
    CHECK_EQUAL("-2", r.String());
}

TEST(PrependSignFlag, PositiveForSignedConversion) {
    CHECK_EQUAL(3, npf_pprintf(r.PutC, &r, "%+d", 2));
    CHECK_EQUAL("+2", r.String());
}

TEST(PrependSignFlag, Zero) {
    CHECK_EQUAL(3, npf_pprintf(r.PutC, &r, "%+d", 0));
    CHECK_EQUAL("+0", r.String());
}

TEST(PrependSignFlag, NothingForUnsignedConversion) {
    CHECK_EQUAL(2, npf_pprintf(r.PutC, &r, "%+u", 1));
    CHECK_EQUAL("1", r.String());
}

// Prepend space flag

TEST_GROUP(PrependSpaceFlag) { Recorder r; };

TEST(PrependSpaceFlag, SpaceInsteadOfSignWhenPositive) {
    CHECK_EQUAL(3, npf_pprintf(r.PutC, &r, "% d", 1));
    CHECK_EQUAL(" 1", r.String());
}

TEST(PrependSpaceFlag, MinusWhenNegative) {
    CHECK_EQUAL(3, npf_pprintf(r.PutC, &r, "% d", -1));
    CHECK_EQUAL("-1", r.String());
}

// Leading zero pad flag

TEST_GROUP(LeadingZeroPadFlag) { Recorder r; };

TEST(LeadingZeroPadFlag, DoesNothingOnChar_Undefined) {
    CHECK_EQUAL(2, npf_pprintf(r.PutC, &r, "%010c", 'A'));
    CHECK_EQUAL("A", r.String());
}

TEST(LeadingZeroPadFlag, DoesNothingOnString_Undefined) {
    CHECK_EQUAL(5, npf_pprintf(r.PutC, &r, "%0s", "ABCD"));
    CHECK_EQUAL("ABCD", r.String());
}

// Alternative implementation

TEST_GROUP(AlternativeImplementationFlag) { Recorder r; };

TEST(AlternativeImplementationFlag, HexDoesntPrepend0xIfValueIsZero) {
    CHECK_EQUAL(2, npf_pprintf(r.PutC, &r, "%#x", 0));
    CHECK_EQUAL("0", r.String());
}

TEST(AlternativeImplementationFlag, HexUppercase0X) {
    CHECK_EQUAL(4, npf_pprintf(r.PutC, &r, "%#X", 1));
    CHECK_EQUAL("0X1", r.String());
}

TEST(AlternativeImplementationFlag, HexLowercase0x) {
    CHECK_EQUAL(4, npf_pprintf(r.PutC, &r, "%#x", 1));
    CHECK_EQUAL("0x1", r.String());
}

TEST(AlternativeImplementationFlag, OctalDoesntPrepent0IfValueIsZero) {
    CHECK_EQUAL(2, npf_pprintf(r.PutC, &r, "%#o", 0));
    CHECK_EQUAL("0", r.String());
}

TEST(AlternativeImplementationFlag, OctalNonZero) {
    CHECK_EQUAL(3, npf_pprintf(r.PutC, &r, "%#o", 2));
    CHECK_EQUAL("02", r.String());
}

// Precision

TEST_GROUP(Precision) { Recorder r; };

// TEST(Precision,

