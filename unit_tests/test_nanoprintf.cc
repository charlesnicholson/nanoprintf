#include "CppUTest/TestHarness.h"
#include "nanoprintf_in_unit_tests.h"

TEST_GROUP(npf__parse_format_spec) { npf__format_spec_t spec; };

TEST(npf__parse_format_spec, ReturnsZeroIfFirstCharIsntPercent) {
    CHECK_EQUAL(0, npf__parse_format_spec("abcd", &spec));
}

TEST(npf__parse_format_spec, ReturnsZeroIfPercentEndsString) {
    CHECK_EQUAL(0, npf__parse_format_spec("%", &spec));
}

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

