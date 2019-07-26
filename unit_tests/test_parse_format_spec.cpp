#include "CppUTest/TestHarness.h"
#include "nanoprintf_in_unit_tests.h"

TEST_GROUP(npf__parse_format_spec) { npf__format_spec_t spec; };

TEST(npf__parse_format_spec, ReturnsZeroIfFirstCharIsntPercent) {
    CHECK_EQUAL(0, npf__parse_format_spec("abcd", &spec));
}

TEST(npf__parse_format_spec, ReturnsZeroIfPercentEndsString) {
    CHECK_EQUAL(0, npf__parse_format_spec("%", &spec));
}

// Printf behavior is specified in ISO/IEC 9899:201x 7.21.6.1

// Otional flags are defined in 7.21.6.1.6

/*
    '-' flag: The result of the conversion is left-justified within the field.
   (It is right-justified if this flag is not specified.)
*/

TEST(npf__parse_format_spec, FlagLeftJustifiedAloneNotParsed) {
    CHECK_EQUAL(0, npf__parse_format_spec("%-", &spec));
}

TEST(npf__parse_format_spec, FlagLeftJustifiedOffByDefault) {
    CHECK_EQUAL(2, npf__parse_format_spec("%u", &spec));
    CHECK_EQUAL(0, spec.left_justified);
}

TEST(npf__parse_format_spec, FlagLeftJustified) {
    CHECK_EQUAL(3, npf__parse_format_spec("%-u", &spec));
    CHECK_EQUAL(1, spec.left_justified);
}

TEST(npf__parse_format_spec, FlagLeftJustifiedMultiple) {
    CHECK_EQUAL(7, npf__parse_format_spec("%-----u", &spec));
    CHECK_EQUAL(1, spec.left_justified);
}

/*
    '+': The result of a signed conversion always begins with a plus or minus
   sign. (It begins with a sign only when a negative value is converted if this
   flag is not specified.) The results of all floating conversions of a negative
   zero, and of negative values that round to zero, include a minus sign.
*/

TEST(npf__parse_format_spec, FlagPrependSignAloneNotParsed) {
    CHECK_EQUAL(0, npf__parse_format_spec("%+", &spec));
}

TEST(npf__parse_format_spec, FlagPrependSignOffByDefault) {
    CHECK_EQUAL(2, npf__parse_format_spec("%u", &spec));
    CHECK_EQUAL(0, spec.prepend_sign);
}

TEST(npf__parse_format_spec, FlagPrependSign) {
    CHECK_EQUAL(3, npf__parse_format_spec("%+u", &spec));
    CHECK_EQUAL(1, spec.prepend_sign);
}

TEST(npf__parse_format_spec, FlagPrependSignMultiple) {
    CHECK_EQUAL(7, npf__parse_format_spec("%+++++u", &spec));
    CHECK_EQUAL(1, spec.prepend_sign);
}

/*
    ' ': If the first character of a signed conversion is not a sign, or if a
   signed conversion results in no characters, a space is prefixed to the
   result. If the space and + flags both appear, the space flag is ignored.
*/

TEST(npf__parse_format_spec, FlagPrependSpaceAloneNotParsed) {
    CHECK_EQUAL(0, npf__parse_format_spec("% ", &spec));
}

TEST(npf__parse_format_spec, FlagPrependSpaceOffByDefault) {
    CHECK_EQUAL(2, npf__parse_format_spec("%u", &spec));
    CHECK_EQUAL(0, spec.prepend_space);
}

TEST(npf__parse_format_spec, FlagPrependSpace) {
    CHECK_EQUAL(3, npf__parse_format_spec("% u", &spec));
    CHECK_EQUAL(1, spec.prepend_space);
}

TEST(npf__parse_format_spec, FlagPrependSpaceMultiple) {
    CHECK_EQUAL(7, npf__parse_format_spec("%     u", &spec));
    CHECK_EQUAL(1, spec.prepend_space);
}

TEST(npf__parse_format_spec, FlagPrependSpaceIgnoredIfPrependSignPresent) {
    CHECK_EQUAL(4, npf__parse_format_spec("%+ u", &spec));
    CHECK_EQUAL(1, spec.prepend_sign);
    CHECK_EQUAL(0, spec.prepend_space);

    CHECK_EQUAL(4, npf__parse_format_spec("% +u", &spec));
    CHECK_EQUAL(1, spec.prepend_sign);
    CHECK_EQUAL(0, spec.prepend_space);

    CHECK_EQUAL(7, npf__parse_format_spec("% + + u", &spec));
    CHECK_EQUAL(1, spec.prepend_sign);
    CHECK_EQUAL(0, spec.prepend_space);
}

/*
    '#': The result is converted to an ‘‘alternative form’’. For o conversion,
   it increases the precision, if and only if necessary, to force the first
   digit of the result to be a zero (if the value and precision are both 0, a
   single 0 is printed). For x (or X) conversion, a nonzero result has 0x (or
   0X) prefixed to it. For a, A, e, E, f, F, g, and G conversions, the result of
   converting a floating-point number always contains a decimal-point character,
   even if no digits follow it. (Normally, a decimal-point character appears in
   the result of these conversions only if a digit follows it.) For g and G
   conversions, trailing zeros are not removed from the result. For other
   conversions, the behavior is undefined.
*/

TEST(npf__parse_format_spec, FlagAlternativeFormAloneNotParsed) {
    CHECK_EQUAL(0, npf__parse_format_spec("%#", &spec));
}

TEST(npf__parse_format_spec, FlagAlternativeFormOffByDefault) {
    CHECK_EQUAL(2, npf__parse_format_spec("%u", &spec));
    CHECK_EQUAL(0, spec.alternative_form);
}

TEST(npf__parse_format_spec, FlagAlternativeForm) {
    CHECK_EQUAL(3, npf__parse_format_spec("%#u", &spec));
    CHECK_EQUAL(1, spec.alternative_form);
}

TEST(npf__parse_format_spec, FlagAlternativeFormMultiple) {
    CHECK_EQUAL(7, npf__parse_format_spec("%#####u", &spec));
    CHECK_EQUAL(1, spec.alternative_form);
}

/*
    '0': For d, i, o, u, x, X, a, A, e, E, f, F, g, and G conversions, leading
   zeros (following any indication of sign or base) are used to pad to the field
   width rather than performing space padding, except when converting an
   infinity or NaN. If the 0 and - flags both appear, the 0 flag is ignored. For
   d, i, o, u, x, and X conversions, if a precision is specified, the 0 flag is
   ignored. For other conversions, the behavior is undefined.
*/

TEST(npf__parse_format_spec, FlagLeadingZeroPadAloneNotParsed) {
    CHECK_EQUAL(0, npf__parse_format_spec("%0", &spec));
}

TEST(npf__parse_format_spec, FlagLeadingZeroPadOffByDefault) {
    CHECK_EQUAL(2, npf__parse_format_spec("%u", &spec));
    CHECK_EQUAL(0, spec.leading_zero_pad);
}

TEST(npf__parse_format_spec, FlagLeadingZeroPad) {
    CHECK_EQUAL(3, npf__parse_format_spec("%0u", &spec));
    CHECK_EQUAL(1, spec.leading_zero_pad);
}

TEST(npf__parse_format_spec, FlagLeadingZeroPadMultiple) {
    CHECK_EQUAL(7, npf__parse_format_spec("%00000u", &spec));
    CHECK_EQUAL(1, spec.leading_zero_pad);
}

TEST(npf__parse_format_spec, FlagLeadingZeroPadIgnoredWhenLeftJustified) {
    CHECK_EQUAL(4, npf__parse_format_spec("%0-u", &spec));
    CHECK_EQUAL(1, spec.left_justified);
    CHECK_EQUAL(0, spec.leading_zero_pad);

    CHECK_EQUAL(4, npf__parse_format_spec("%-0u", &spec));
    CHECK_EQUAL(1, spec.left_justified);
    CHECK_EQUAL(0, spec.leading_zero_pad);

    CHECK_EQUAL(8, npf__parse_format_spec("%0-0-0-u", &spec));
    CHECK_EQUAL(1, spec.left_justified);
    CHECK_EQUAL(0, spec.leading_zero_pad);
}

/*
   An optional minimum field width. If the converted value has fewer characters
   than the field width, it is padded with spaces (by default) on the left (or
   right, if the left adjustment flag, described later, has been given) to the
   field width. The field width takes the form of an asterisk * (described
   later) or a nonnegative decimal integer. Note that 0 is taken as a flag, not
   as the beginning of a field width.
*/

TEST(npf__parse_format_spec, FieldWidthIsNoneIfNotSpecified) {
    CHECK_EQUAL(2, npf__parse_format_spec("%u", &spec));
    CHECK_EQUAL(NPF_FMT_SPEC_FIELD_WIDTH_NONE, spec.field_width_type);
}

TEST(npf__parse_format_spec, FieldWidthStar) {
    CHECK_EQUAL(3, npf__parse_format_spec("%*u", &spec));
    CHECK_EQUAL(NPF_FMT_SPEC_FIELD_WIDTH_STAR, spec.field_width_type);
}

TEST(npf__parse_format_spec, FieldWidthReadFromLiteral) {
    CHECK_EQUAL(5, npf__parse_format_spec("%123u", &spec));
    CHECK_EQUAL(NPF_FMT_SPEC_FIELD_WIDTH_LITERAL, spec.field_width_type);
    CHECK_EQUAL(123, spec.field_width);
}

/*
   An optional precision that gives the minimum number of digits to appear for
   the d, i, o, u, x, and X conversions, the number of digits to appear after
   the decimal-point character for a, A, e, E, f, and F conversions, the maximum
   number of significant digits for the g and G conversions, or the maximum
   number of bytes to be written for s conversions. The precision takes the form
   of a period (.) followed either by an asterisk * (described later) or by an
   optional decimal integer; if only the period is specified, the precision is
   taken as zero. If a precision appears with any other conversion specifier,
   the behavior is undefined.
*/

TEST(npf__parse_format_spec, PrecisionDefaultIsOneForNumbers) {
    spec.precision = 1234;
    CHECK_EQUAL(2, npf__parse_format_spec("%u", &spec));
    CHECK_EQUAL(NPF_FMT_SPEC_PRECISION_NONE, spec.precision_type);
    CHECK_EQUAL(1, spec.precision);

    spec.precision = 1234;
    CHECK_EQUAL(2, npf__parse_format_spec("%i", &spec));
    CHECK_EQUAL(NPF_FMT_SPEC_PRECISION_NONE, spec.precision_type);
    CHECK_EQUAL(1, spec.precision);

    spec.precision = 1234;
    CHECK_EQUAL(2, npf__parse_format_spec("%o", &spec));
    CHECK_EQUAL(NPF_FMT_SPEC_PRECISION_NONE, spec.precision_type);
    CHECK_EQUAL(1, spec.precision);

    spec.precision = 1234;
    CHECK_EQUAL(2, npf__parse_format_spec("%x", &spec));
    CHECK_EQUAL(NPF_FMT_SPEC_PRECISION_NONE, spec.precision_type);
    CHECK_EQUAL(1, spec.precision);
}

TEST(npf__parse_format_spec, PrecisionDefaultIsSixIfFloat) {
    CHECK_EQUAL(2, npf__parse_format_spec("%f", &spec));
    CHECK_EQUAL(NPF_FMT_SPEC_PRECISION_NONE, spec.precision_type);
    CHECK_EQUAL(6, spec.precision);
    /*
        Not supported yet

        CHECK_EQUAL(2, npf__parse_format_spec("%g", &spec));
        CHECK_EQUAL(NPF_FMT_SPEC_PRECISION_NONE, spec.precision_type);
        CHECK_EQUAL(6, spec.precision);
        CHECK_EQUAL(2, npf__parse_format_spec("%e", &spec));
        CHECK_EQUAL(NPF_FMT_SPEC_PRECISION_NONE, spec.precision_type);
        CHECK_EQUAL(6, spec.precision);
    */
}

TEST(npf__parse_format_spec, PrecisionStar) {
    CHECK_EQUAL(4, npf__parse_format_spec("%.*u", &spec));
    CHECK_EQUAL(NPF_FMT_SPEC_PRECISION_STAR, spec.precision_type);
}

TEST(npf__parse_format_spec, PrecisionIsLiteralZeroIfJustPeriod) {
    CHECK_EQUAL(3, npf__parse_format_spec("%.u", &spec));
    CHECK_EQUAL(NPF_FMT_SPEC_PRECISION_LITERAL, spec.precision_type);
    CHECK_EQUAL(0, spec.precision);
}

TEST(npf__parse_format_spec, PrecisionIsLiteralIfPeriodThenNumber) {
    CHECK_EQUAL(8, npf__parse_format_spec("%.12345u", &spec));
    CHECK_EQUAL(NPF_FMT_SPEC_PRECISION_LITERAL, spec.precision_type);
    CHECK_EQUAL(12345, spec.precision);
}

TEST(npf__parse_format_spec, PrecisionNoneDefaultWhenNegativeLiteral) {
    CHECK_EQUAL(6, npf__parse_format_spec("%.-34u", &spec));
    CHECK_EQUAL(NPF_FMT_SPEC_PRECISION_NONE, spec.precision_type);
    CHECK_EQUAL(1, spec.precision);
}

/*
   The length modifiers and their meanings are:

   hh Specifies that a following d, i, o, u, x, or X conversion specifier
   applies to a signed char or unsigned char argument (the argument will have
   been promoted according to the integer promotions, but its value shall be
   converted to signed char or unsigned char before printing); or that a
   following n conversion specifier applies to a pointer to a signed char
   argument.

   h Specifies that a following d, i, o, u, x, or X conversion specifier applies
   to a short int or unsigned short int argument (the argument will have been
   promoted according to the integer promotions, but its value shall be
   converted to short int or unsigned short int before printing); or that a
   following n conversion specifier applies to a pointer to a short int
   argument.

   l (ell) Specifies that a following d, i, o, u, x, or X conversion specifier
   applies to a long int or unsigned long int argument; that a following n
   conversion specifier applies to a pointer to a long int argument; that a
   following c conversion specifier applies to a wint_t argument; that a
   following s conversion specifier applies to a pointer to a wchar_t argument;
   or has no effect on a following a, A, e, E, f, F, g, or G conversion
   specifier.

   ll (ell-ell) Specifies that a following d, i, o, u, x, or X
   conversion specifier applies to a long long int or unsigned long long int
   argument; or that a following n conversion specifier applies to a pointer to
   a long long int argument.

   j Specifies that a following d, i, o, u, x, or X
   conversion specifier applies to an intmax_t or uintmax_t argument; or that a
   following n conversion specifier applies to a pointer to an intmax_t
   argument.

   z Specifies that a following d, i, o, u, x, or X conversion specifier applies
   to a size_t or the corresponding signed integer type argument; or that a
   following n conversion specifier applies to a pointer to a signed integer
   type corresponding to size_t argument

   t Specifies that a following d, i, o, u, x, or X conversion specifier applies
   to a ptrdiff_t or the corresponding unsigned integer type argument; or that a
   following n conversion specifier applies to a pointer to a ptrdiff_t
   argument.

   L Specifies that a following a, A, e, E, f, F, g, or G conversion specifier
   applies to a long double argument.
*/

TEST(npf__parse_format_spec, LengthModWithoutConversionSpecifierReturnsZero) {
    CHECK_EQUAL(0, npf__parse_format_spec("%hh", &spec));
}

TEST(npf__parse_format_spec, LengthMod_hh) {
    CHECK_EQUAL(4, npf__parse_format_spec("%hhu", &spec));
    CHECK_EQUAL(NPF_FMT_SPEC_LEN_MOD_CHAR, spec.length_modifier);
}

TEST(npf__parse_format_spec, LengthMode_h) {
    CHECK_EQUAL(3, npf__parse_format_spec("%hu", &spec));
    CHECK_EQUAL(NPF_FMT_SPEC_LEN_MOD_SHORT, spec.length_modifier);
}

TEST(npf__parse_format_spec, LengthMode_l) {
    CHECK_EQUAL(3, npf__parse_format_spec("%lu", &spec));
    CHECK_EQUAL(NPF_FMT_SPEC_LEN_MOD_LONG, spec.length_modifier);
}

#if NANOPRINTF_USE_LARGE_FORMAT_SPECIFIERS == 1
TEST(npf__parse_format_spec, LengthMode_ll) {
    CHECK_EQUAL(4, npf__parse_format_spec("%llu", &spec));
    CHECK_EQUAL(NPF_FMT_SPEC_LEN_MOD_LARGE_LONG_LONG, spec.length_modifier);
}

TEST(npf__parse_format_spec, LengthMode_j) {
    CHECK_EQUAL(3, npf__parse_format_spec("%ju", &spec));
    CHECK_EQUAL(NPF_FMT_SPEC_LEN_MOD_LARGE_INTMAX, spec.length_modifier);
}

TEST(npf__parse_format_spec, LengthMode_z) {
    CHECK_EQUAL(3, npf__parse_format_spec("%zu", &spec));
    CHECK_EQUAL(NPF_FMT_SPEC_LEN_MOD_LARGE_SIZET, spec.length_modifier);
}

TEST(npf__parse_format_spec, LengthMode_t) {
    CHECK_EQUAL(3, npf__parse_format_spec("%tu", &spec));
    CHECK_EQUAL(NPF_FMT_SPEC_LEN_MOD_LARGE_PTRDIFFT, spec.length_modifier);
}

TEST(npf__parse_format_spec, LengthMode_L) {
    CHECK_EQUAL(3, npf__parse_format_spec("%Lu", &spec));
    CHECK_EQUAL(NPF_FMT_SPEC_LEN_MOD_LONG_DOUBLE, spec.length_modifier);
}
#endif

// All conversion specifiers are defined in 7.21.6.1.8

TEST(npf__parse_format_spec, PercentLiteral) {
    CHECK_EQUAL(2, npf__parse_format_spec("%%", &spec));
    CHECK_EQUAL(NPF_FMT_SPEC_CONV_PERCENT, spec.conv_spec);
}

TEST(npf__parse_format_spec, PercentClearsPrecision) {
    CHECK_EQUAL(4, npf__parse_format_spec("%.9%", &spec));
    CHECK_EQUAL(NPF_FMT_SPEC_PRECISION_NONE, spec.precision_type);
}

TEST(npf__parse_format_spec, c) {
    CHECK_EQUAL(2, npf__parse_format_spec("%c", &spec));
    CHECK_EQUAL(NPF_FMT_SPEC_CONV_CHAR, spec.conv_spec);
}

TEST(npf__parse_format_spec, cClearsPrecision) {
    CHECK_EQUAL(4, npf__parse_format_spec("%.9c", &spec));
    CHECK_EQUAL(NPF_FMT_SPEC_PRECISION_NONE, spec.precision_type);
}

TEST(npf__parse_format_spec, s) {
    CHECK_EQUAL(2, npf__parse_format_spec("%s", &spec));
    CHECK_EQUAL(NPF_FMT_SPEC_CONV_STRING, spec.conv_spec);
}

TEST(npf__parse_format_spec, cClearsLeadingZero) {
    CHECK_EQUAL(3, npf__parse_format_spec("%0s", &spec));
    CHECK_EQUAL(0, spec.leading_zero_pad);
}

TEST(npf__parse_format_spec, i) {
    CHECK_EQUAL(2, npf__parse_format_spec("%i", &spec));
    CHECK_EQUAL(NPF_FMT_SPEC_CONV_SIGNED_INT, spec.conv_spec);
}

TEST(npf__parse_format_spec, d) {
    CHECK_EQUAL(2, npf__parse_format_spec("%d", &spec));
    CHECK_EQUAL(NPF_FMT_SPEC_CONV_SIGNED_INT, spec.conv_spec);
}

TEST(npf__parse_format_spec, o) {
    CHECK_EQUAL(2, npf__parse_format_spec("%o", &spec));
    CHECK_EQUAL(NPF_FMT_SPEC_CONV_OCTAL, spec.conv_spec);
}

TEST(npf__parse_format_spec, x) {
    CHECK_EQUAL(2, npf__parse_format_spec("%x", &spec));
    CHECK_EQUAL(NPF_FMT_SPEC_CONV_HEX_INT, spec.conv_spec);
    CHECK_EQUAL(NPF_FMT_SPEC_CONV_CASE_LOWER, spec.conv_spec_case);
}

TEST(npf__parse_format_spec, X) {
    CHECK_EQUAL(2, npf__parse_format_spec("%X", &spec));
    CHECK_EQUAL(NPF_FMT_SPEC_CONV_HEX_INT, spec.conv_spec);
    CHECK_EQUAL(NPF_FMT_SPEC_CONV_CASE_UPPER, spec.conv_spec_case);
}

TEST(npf__parse_format_spec, u) {
    CHECK_EQUAL(2, npf__parse_format_spec("%u", &spec));
    CHECK_EQUAL(NPF_FMT_SPEC_CONV_UNSIGNED_INT, spec.conv_spec);
}

TEST(npf__parse_format_spec, n) {
    CHECK_EQUAL(2, npf__parse_format_spec("%n", &spec));
    CHECK_EQUAL(NPF_FMT_SPEC_CONV_WRITEBACK, spec.conv_spec);
}

TEST(npf__parse_format_spec, nClearsPrecision) {
    CHECK_EQUAL(4, npf__parse_format_spec("%.4n", &spec));
    CHECK_EQUAL(NPF_FMT_SPEC_PRECISION_NONE, spec.precision_type);
}

TEST(npf__parse_format_spec, p) {
    CHECK_EQUAL(2, npf__parse_format_spec("%p", &spec));
    CHECK_EQUAL(NPF_FMT_SPEC_CONV_POINTER, spec.conv_spec);
}

TEST(npf__parse_format_spec, pClearsPrecision) {
    CHECK_EQUAL(4, npf__parse_format_spec("%.4p", &spec));
    CHECK_EQUAL(NPF_FMT_SPEC_PRECISION_NONE, spec.precision_type);
}

TEST(npf__parse_format_spec, f) {
    CHECK_EQUAL(2, npf__parse_format_spec("%f", &spec));
    CHECK_EQUAL(NPF_FMT_SPEC_CONV_FLOAT_DECIMAL, spec.conv_spec);
    CHECK_EQUAL(NPF_FMT_SPEC_CONV_CASE_LOWER, spec.conv_spec_case);
}

TEST(npf__parse_format_spec, F) {
    CHECK_EQUAL(2, npf__parse_format_spec("%F", &spec));
    CHECK_EQUAL(NPF_FMT_SPEC_CONV_FLOAT_DECIMAL, spec.conv_spec);
    CHECK_EQUAL(NPF_FMT_SPEC_CONV_CASE_UPPER, spec.conv_spec_case);
}

/*
    Not implemented yet

TEST(npf__parse_format_spec, e) {
    CHECK_EQUAL(2, npf__parse_format_spec("%e", &spec));
    CHECK_EQUAL(NPF_FMT_SPEC_CONV_FLOAT_EXPONENT, spec.conv_spec);
    CHECK_EQUAL(NPF_FMT_SPEC_CONV_CASE_LOWER, spec.conv_spec_case);
}

TEST(npf__parse_format_spec, E) {
    CHECK_EQUAL(2, npf__parse_format_spec("%E", &spec));
    CHECK_EQUAL(NPF_FMT_SPEC_CONV_FLOAT_EXPONENT, spec.conv_spec);
    CHECK_EQUAL(NPF_FMT_SPEC_CONV_CASE_UPPER, spec.conv_spec_case);
}

TEST(npf__parse_format_spec, a) {
    CHECK_EQUAL(2, npf__parse_format_spec("%a", &spec));
    CHECK_EQUAL(NPF_FMT_SPEC_CONV_FLOAT_HEXPONENT, spec.conv_spec);
    CHECK_EQUAL(NPF_FMT_SPEC_CONV_CASE_LOWER, spec.conv_spec_case);
}

TEST(npf__parse_format_spec, A) {
    CHECK_EQUAL(2, npf__parse_format_spec("%A", &spec));
    CHECK_EQUAL(NPF_FMT_SPEC_CONV_FLOAT_HEXPONENT, spec.conv_spec);
    CHECK_EQUAL(NPF_FMT_SPEC_CONV_CASE_UPPER, spec.conv_spec_case);
}

TEST(npf__parse_format_spec, g) {
    CHECK_EQUAL(2, npf__parse_format_spec("%g", &spec));
    CHECK_EQUAL(NPF_FMT_SPEC_CONV_FLOAT_DYNAMIC, spec.conv_spec);
    CHECK_EQUAL(NPF_FMT_SPEC_CONV_CASE_LOWER, spec.conv_spec_case);
}

TEST(npf__parse_format_spec, G) {
    CHECK_EQUAL(2, npf__parse_format_spec("%G", &spec));
    CHECK_EQUAL(NPF_FMT_SPEC_CONV_FLOAT_DYNAMIC, spec.conv_spec);
    CHECK_EQUAL(NPF_FMT_SPEC_CONV_CASE_UPPER, spec.conv_spec_case);
}
*/

