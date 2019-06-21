#include "CppUTest/TestHarness.h"
#include "nanoprintf_in_unit_tests.h"

TEST_GROUP(npf__parse_format_spec) { npf__format_spec_t spec; };

TEST(npf__parse_format_spec, ReturnsZeroIfFirstCharIsntPercent) {
    CHECK_EQUAL(0, npf__parse_format_spec("abcd", &spec));
}

TEST(npf__parse_format_spec, ReturnsZeroIfPercentEndsString) {
    CHECK_EQUAL(0, npf__parse_format_spec("%", &spec));
}

// All conversion specifiers are defined in 7.21.6.1.8

TEST(npf__parse_format_spec, PercentLiteral) {
    CHECK_EQUAL(2, npf__parse_format_spec("%%", &spec));
    CHECK_EQUAL(NPF_FMT_SPEC_CONV_PERCENT, spec.conversion_specifier);
}

TEST(npf__parse_format_spec, c) {
    CHECK_EQUAL(2, npf__parse_format_spec("%c", &spec));
    CHECK_EQUAL(NPF_FMT_SPEC_CONV_CHAR, spec.conversion_specifier);
}

TEST(npf__parse_format_spec, s) {
    CHECK_EQUAL(2, npf__parse_format_spec("%s", &spec));
    CHECK_EQUAL(NPF_FMT_SPEC_CONV_STRING, spec.conversion_specifier);
}

TEST(npf__parse_format_spec, i) {
    CHECK_EQUAL(2, npf__parse_format_spec("%i", &spec));
    CHECK_EQUAL(NPF_FMT_SPEC_CONV_SIGNED_INT, spec.conversion_specifier);
}

TEST(npf__parse_format_spec, d) {
    CHECK_EQUAL(2, npf__parse_format_spec("%d", &spec));
    CHECK_EQUAL(NPF_FMT_SPEC_CONV_SIGNED_INT, spec.conversion_specifier);
}

TEST(npf__parse_format_spec, o) {
    CHECK_EQUAL(2, npf__parse_format_spec("%o", &spec));
    CHECK_EQUAL(NPF_FMT_SPEC_CONV_OCTAL, spec.conversion_specifier);
}

TEST(npf__parse_format_spec, x) {
    CHECK_EQUAL(2, npf__parse_format_spec("%x", &spec));
    CHECK_EQUAL(NPF_FMT_SPEC_CONV_HEX_INT, spec.conversion_specifier);
    CHECK_EQUAL(NPF_FMT_SPEC_CONV_CASE_LOWER, spec.conversion_specifier_case);
}

TEST(npf__parse_format_spec, X) {
    CHECK_EQUAL(2, npf__parse_format_spec("%X", &spec));
    CHECK_EQUAL(NPF_FMT_SPEC_CONV_HEX_INT, spec.conversion_specifier);
    CHECK_EQUAL(NPF_FMT_SPEC_CONV_CASE_UPPER, spec.conversion_specifier_case);
}

TEST(npf__parse_format_spec, u) {
    CHECK_EQUAL(2, npf__parse_format_spec("%u", &spec));
    CHECK_EQUAL(NPF_FMT_SPEC_CONV_UNSIGNED_INT, spec.conversion_specifier);
}

TEST(npf__parse_format_spec, n) {
    CHECK_EQUAL(2, npf__parse_format_spec("%n", &spec));
    CHECK_EQUAL(NPF_FMT_SPEC_CONV_CHARS_WRITTEN, spec.conversion_specifier);
}

TEST(npf__parse_format_spec, p) {
    CHECK_EQUAL(2, npf__parse_format_spec("%p", &spec));
    CHECK_EQUAL(NPF_FMT_SPEC_CONV_POINTER, spec.conversion_specifier);
}

TEST(npf__parse_format_spec, f) {
    CHECK_EQUAL(2, npf__parse_format_spec("%f", &spec));
    CHECK_EQUAL(NPF_FMT_SPEC_CONV_FLOAT_DECIMAL, spec.conversion_specifier);
    CHECK_EQUAL(NPF_FMT_SPEC_CONV_CASE_LOWER, spec.conversion_specifier_case);
}

TEST(npf__parse_format_spec, F) {
    CHECK_EQUAL(2, npf__parse_format_spec("%F", &spec));
    CHECK_EQUAL(NPF_FMT_SPEC_CONV_FLOAT_DECIMAL, spec.conversion_specifier);
    CHECK_EQUAL(NPF_FMT_SPEC_CONV_CASE_UPPER, spec.conversion_specifier_case);
}

TEST(npf__parse_format_spec, e) {
    CHECK_EQUAL(2, npf__parse_format_spec("%e", &spec));
    CHECK_EQUAL(NPF_FMT_SPEC_CONV_FLOAT_EXPONENT, spec.conversion_specifier);
    CHECK_EQUAL(NPF_FMT_SPEC_CONV_CASE_LOWER, spec.conversion_specifier_case);
}

TEST(npf__parse_format_spec, E) {
    CHECK_EQUAL(2, npf__parse_format_spec("%E", &spec));
    CHECK_EQUAL(NPF_FMT_SPEC_CONV_FLOAT_EXPONENT, spec.conversion_specifier);
    CHECK_EQUAL(NPF_FMT_SPEC_CONV_CASE_UPPER, spec.conversion_specifier_case);
}

TEST(npf__parse_format_spec, a) {
    CHECK_EQUAL(2, npf__parse_format_spec("%a", &spec));
    CHECK_EQUAL(NPF_FMT_SPEC_CONV_C99_FLOAT_HEX, spec.conversion_specifier);
    CHECK_EQUAL(NPF_FMT_SPEC_CONV_CASE_LOWER, spec.conversion_specifier_case);
}

TEST(npf__parse_format_spec, A) {
    CHECK_EQUAL(2, npf__parse_format_spec("%A", &spec));
    CHECK_EQUAL(NPF_FMT_SPEC_CONV_C99_FLOAT_HEX, spec.conversion_specifier);
    CHECK_EQUAL(NPF_FMT_SPEC_CONV_CASE_UPPER, spec.conversion_specifier_case);
}

TEST(npf__parse_format_spec, g) {
    CHECK_EQUAL(2, npf__parse_format_spec("%g", &spec));
    CHECK_EQUAL(NPF_FMT_SPEC_CONV_FLOAT_DYNAMIC, spec.conversion_specifier);
    CHECK_EQUAL(NPF_FMT_SPEC_CONV_CASE_LOWER, spec.conversion_specifier_case);
}

TEST(npf__parse_format_spec, G) {
    CHECK_EQUAL(2, npf__parse_format_spec("%G", &spec));
    CHECK_EQUAL(NPF_FMT_SPEC_CONV_FLOAT_DYNAMIC, spec.conversion_specifier);
    CHECK_EQUAL(NPF_FMT_SPEC_CONV_CASE_UPPER, spec.conversion_specifier_case);
}

// All optional flags are defined in 7.21.6.1.6

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

