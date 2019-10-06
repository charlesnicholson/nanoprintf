#include "nanoprintf_in_unit_tests.h"

#include "CppUTest/TestHarness.h"

#include <cstring>
#include <vector>

struct Recorder {
    static void PutC(int c, void *ctx) {
        Recorder &r = *(Recorder *)ctx;
        r.calls.push_back(c);
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
    CHECK_EQUAL(0, npf_pprintf(r.PutC, &r, ""));
    CHECK_EQUAL(1, r.calls.size());
    CHECK_EQUAL('\0', r.calls[0]);
}

TEST(npf_vpprintf, PutsOnlyCharacterThenNullTerm) {
    CHECK_EQUAL(1, npf_pprintf(r.PutC, &r, "A"));
    CHECK_EQUAL(2, r.calls.size());
    CHECK_EQUAL('A', r.calls[0]);
    CHECK_EQUAL('\0', r.calls[1]);
}

TEST(npf_vpprintf, PutsEntireStringThenNullTerm_NoFormatSpecifiers) {
    char const *s = "Hello from nanoprintf!";
    CHECK_EQUAL((int)strlen(s), npf_pprintf(r.PutC, &r, s));
    STRCMP_EQUAL(s, r.String().c_str());
}

// Conversion Specifiers

TEST_GROUP(Percent) { Recorder r; };

TEST(Percent, Literal) {
    CHECK_EQUAL(1, npf_pprintf(r.PutC, &r, "%%"));
    STRCMP_EQUAL("%", r.String().c_str());
}

TEST_GROUP(Char) { Recorder r; };

TEST(Char, Single) {
    CHECK_EQUAL(1, npf_pprintf(r.PutC, &r, "%c", 'A'));
    STRCMP_EQUAL("A", r.String().c_str());
}

TEST(Char, Multiple) {
    CHECK_EQUAL(4, npf_pprintf(r.PutC, &r, "%c%c%c%c", 'A', 'B', 'C', 'D'));
    STRCMP_EQUAL("ABCD", r.String().c_str());
}

TEST_GROUP(String) { Recorder r; };

TEST(String, Single) {
    CHECK_EQUAL(4, npf_pprintf(r.PutC, &r, "%s", "abcd"));
    STRCMP_EQUAL("abcd", r.String().c_str());
}

TEST(String, Empty) {
    CHECK_EQUAL(0, npf_pprintf(r.PutC, &r, "%s", ""));
    STRCMP_EQUAL("", r.String().c_str());
}

TEST(String, Multiple) {
    CHECK_EQUAL(8, npf_pprintf(r.PutC, &r, "%s%s%s", "abcd", "e", "fgh"));
    STRCMP_EQUAL("abcdefgh", r.String().c_str());
}

TEST_GROUP(SignedInt) { Recorder r; };

TEST(SignedInt, Zero) {
    CHECK_EQUAL(1, npf_pprintf(r.PutC, &r, "%i", 0));
    STRCMP_EQUAL("0", r.String().c_str());
}

TEST(SignedInt, Positive) {
    CHECK_EQUAL(3, npf_pprintf(r.PutC, &r, "%i", 123));
    STRCMP_EQUAL("123", r.String().c_str());
}

TEST(SignedInt, Negative) {
    CHECK_EQUAL(4, npf_pprintf(r.PutC, &r, "%i", -456));
    STRCMP_EQUAL("-456", r.String().c_str());
}

TEST(SignedInt, IntMax) {
    CHECK_EQUAL(10, npf_pprintf(r.PutC, &r, "%d", INT_MAX));
    STRCMP_EQUAL("2147483647", r.String().c_str());
}

TEST(SignedInt, IntMin) {
    npf_pprintf(r.PutC, &r, "%d", INT_MIN);
    STRCMP_EQUAL("-2147483648", r.String().c_str());
}

TEST_GROUP(UnsignedInt) { Recorder r; };

TEST(UnsignedInt, Zero) {
    CHECK_EQUAL(1, npf_pprintf(r.PutC, &r, "%u", 0));
    STRCMP_EQUAL("0", r.String().c_str());
}

TEST(UnsignedInt, Positive) {
    CHECK_EQUAL(5, npf_pprintf(r.PutC, &r, "%u", 45678));
    STRCMP_EQUAL("45678", r.String().c_str());
}

TEST(UnsignedInt, UIntMax32) {
    CHECK_EQUAL(10, npf_pprintf(r.PutC, &r, "%u", 4294967295u));
    STRCMP_EQUAL("4294967295", r.String().c_str());
}

TEST_GROUP(Octal) { Recorder r; };

TEST(Octal, Zero) {
    CHECK_EQUAL(1, npf_pprintf(r.PutC, &r, "%o", 0));
    STRCMP_EQUAL("0", r.String().c_str());
}

TEST(Octal, Positive) {
    CHECK_EQUAL(4, npf_pprintf(r.PutC, &r, "%o", 1234));
    STRCMP_EQUAL("2322", r.String().c_str());
}

TEST(Octal, UIntMax32) {
    CHECK_EQUAL(11, npf_pprintf(r.PutC, &r, "%o", 4294967295u));
    STRCMP_EQUAL("37777777777", r.String().c_str());
}

TEST_GROUP(Hex) { Recorder r; };

TEST(Hex, Zero) {
    CHECK_EQUAL(1, npf_pprintf(r.PutC, &r, "%x", 0));
    STRCMP_EQUAL("0", r.String().c_str());
}

TEST(Hex, LessThanA) {
    CHECK_EQUAL(1, npf_pprintf(r.PutC, &r, "%x", 8));
    STRCMP_EQUAL("8", r.String().c_str());
}

TEST(Hex, SingleDigitGreaterThan9) {
    CHECK_EQUAL(1, npf_pprintf(r.PutC, &r, "%x", 0xc));
    STRCMP_EQUAL("c", r.String().c_str());
}

TEST(Hex, Large) {
    CHECK_EQUAL(8, npf_pprintf(r.PutC, &r, "%x", 0x9ABCDEF0));
    STRCMP_EQUAL("9abcdef0", r.String().c_str());
}

TEST(Hex, UIntMax32) {
    CHECK_EQUAL(8, npf_pprintf(r.PutC, &r, "%x", 0xFFFFFFFF));
    STRCMP_EQUAL("ffffffff", r.String().c_str());
}

TEST(Hex, Uppercase) {
    CHECK_EQUAL(8, npf_pprintf(r.PutC, &r, "%X", 0xabcdefab));
    STRCMP_EQUAL("ABCDEFAB", r.String().c_str());
}

TEST_GROUP(Pointer) { Recorder r; };

TEST(Pointer, Null) {
    CHECK_EQUAL(3, npf_pprintf(r.PutC, &r, "%p", nullptr));
    STRCMP_EQUAL("0x0", r.String().c_str());
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

// Float

TEST_GROUP(Float) { Recorder r; };

TEST(Float, Zero) {
    CHECK_EQUAL(8, npf_pprintf(r.PutC, &r, "%f", (double)0.f));
    STRCMP_EQUAL("0.000000", r.String().c_str());
}

TEST(Float, One) {
    CHECK_EQUAL(8, npf_pprintf(r.PutC, &r, "%f", 1.0));
    STRCMP_EQUAL("1.000000", r.String().c_str());
}

// Field Width

TEST_GROUP(FieldWidth) { Recorder r; };

TEST(FieldWidth, OneDoesNothing) {
    CHECK_EQUAL(1, npf_pprintf(r.PutC, &r, "%1c", 'A'));
    STRCMP_EQUAL("A", r.String().c_str());
}

TEST(FieldWidth, RightJustifiedByDefault) {
    CHECK_EQUAL(2, npf_pprintf(r.PutC, &r, "%2c", 'A'));
    STRCMP_EQUAL(" A", r.String().c_str());
}

TEST(FieldWidth, FlagLeftJustified) {
    CHECK_EQUAL(2, npf_pprintf(r.PutC, &r, "%-2c", 'A'));
    STRCMP_EQUAL("A ", r.String().c_str());
}

// Prepend Sign flag

TEST_GROUP(PrependSignFlag) { Recorder r; };

TEST(PrependSignFlag, Negative) {
    CHECK_EQUAL(2, npf_pprintf(r.PutC, &r, "%+d", -2));
    STRCMP_EQUAL("-2", r.String().c_str());
}

TEST(PrependSignFlag, PositiveForSignedConversion) {
    CHECK_EQUAL(2, npf_pprintf(r.PutC, &r, "%+d", 2));
    STRCMP_EQUAL("+2", r.String().c_str());
}

TEST(PrependSignFlag, Zero) {
    CHECK_EQUAL(2, npf_pprintf(r.PutC, &r, "%+d", 0));
    STRCMP_EQUAL("+0", r.String().c_str());
}

TEST(PrependSignFlag, NothingForUnsignedConversion) {
    CHECK_EQUAL(1, npf_pprintf(r.PutC, &r, "%+u", 1));
    STRCMP_EQUAL("1", r.String().c_str());
}

// Prepend space flag

TEST_GROUP(PrependSpaceFlag) { Recorder r; };

TEST(PrependSpaceFlag, SpaceInsteadOfSignWhenPositive) {
    CHECK_EQUAL(2, npf_pprintf(r.PutC, &r, "% d", 1));
    STRCMP_EQUAL(" 1", r.String().c_str());
}

TEST(PrependSpaceFlag, MinusWhenNegative) {
    CHECK_EQUAL(2, npf_pprintf(r.PutC, &r, "% d", -1));
    STRCMP_EQUAL("-1", r.String().c_str());
}

// Leading zero pad flag

TEST_GROUP(LeadingZeroPadFlag) { Recorder r; };

TEST(LeadingZeroPadFlag, DoesNothingOnChar_Undefined) {
    CHECK_EQUAL(1, npf_pprintf(r.PutC, &r, "%010c", 'A'));
    STRCMP_EQUAL("A", r.String().c_str());
}

TEST(LeadingZeroPadFlag, DoesNothingOnString_Undefined) {
    CHECK_EQUAL(4, npf_pprintf(r.PutC, &r, "%0s", "ABCD"));
    STRCMP_EQUAL("ABCD", r.String().c_str());
}

// Alternative implementation

TEST_GROUP(AlternativeImplementationFlag) { Recorder r; };

TEST(AlternativeImplementationFlag, HexDoesntPrepend0xIfValueIsZero) {
    CHECK_EQUAL(1, npf_pprintf(r.PutC, &r, "%#x", 0));
    STRCMP_EQUAL("0", r.String().c_str());
}

TEST(AlternativeImplementationFlag, HexUppercase0X) {
    CHECK_EQUAL(3, npf_pprintf(r.PutC, &r, "%#X", 1));
    STRCMP_EQUAL("0X1", r.String().c_str());
}

TEST(AlternativeImplementationFlag, HexLowercase0x) {
    CHECK_EQUAL(3, npf_pprintf(r.PutC, &r, "%#x", 1));
    STRCMP_EQUAL("0x1", r.String().c_str());
}

TEST(AlternativeImplementationFlag, OctalDoesntPrepent0IfValueIsZero) {
    CHECK_EQUAL(1, npf_pprintf(r.PutC, &r, "%#o", 0));
    STRCMP_EQUAL("0", r.String().c_str());
}

TEST(AlternativeImplementationFlag, OctalNonZero) {
    CHECK_EQUAL(2, npf_pprintf(r.PutC, &r, "%#o", 2));
    STRCMP_EQUAL("02", r.String().c_str());
}
