#include "unit_nanoprintf.h"

#include <cstring>
#include <string>

TEST_CASE("npf_parse_format_spec") {
  npf_format_spec_t spec;
  memset(&spec, 0xCD, sizeof(spec));

  SUBCASE("Optional flags") {
    // Printf behavior is specified in ISO/IEC 9899:201x 7.21.6.1
    // Optional flags are defined in 7.21.6.1.6

    /*
        '-' flag: The result of the conversion is left-justified within the field.
       (It is right-justified if this flag is not specified.)
    */

    REQUIRE(!npf_parse_format_spec("%-", &spec)); // left-justify alone

    SUBCASE("left-justify off by default") {
      REQUIRE(npf_parse_format_spec("%u", &spec) == 2);
      REQUIRE(!spec.left_justified);
    }

    SUBCASE("left-justify specified") {
      REQUIRE(npf_parse_format_spec("%-u", &spec) == 3);
      REQUIRE(spec.left_justified);
    }

    SUBCASE("left-justify specified multiple times") {
      REQUIRE(npf_parse_format_spec("%-----u", &spec) == 7);
      REQUIRE(spec.left_justified);
    }

    /*
        '+': The result of a signed conversion always begins with a plus or minus
       sign. (It begins with a sign only when a negative value is converted if this
       flag is not specified.) The results of all floating conversions of a negative
       zero, and of negative values that round to zero, include a minus sign.
    */

    REQUIRE(!npf_parse_format_spec("%+", &spec)); // prepend sign alone

    SUBCASE("prepend sign off by default") {
      REQUIRE(npf_parse_format_spec("%u", &spec) == 2);
      REQUIRE(!spec.prepend);
    }

    SUBCASE("prepend sign specified") {
      REQUIRE(npf_parse_format_spec("%+u", &spec) == 3);
      REQUIRE(spec.prepend == '+');
    }

    SUBCASE("prepend sign specified multiple times") {
      REQUIRE(npf_parse_format_spec("%+++++u", &spec) == 7);
      REQUIRE(spec.prepend == '+');
    }

    /*
        ' ': If the first character of a signed conversion is not a sign, or if a
       signed conversion results in no characters, a space is prefixed to the
       result. If the space and + flags both appear, the space flag is ignored.
    */

    REQUIRE(!npf_parse_format_spec("% ", &spec));  // space flag alone

    SUBCASE("prepend space off by default") {
      REQUIRE(npf_parse_format_spec("%u", &spec) == 2);
      REQUIRE(!spec.prepend);
    }

    SUBCASE("prepend space specified") {
      REQUIRE(npf_parse_format_spec("% u", &spec) == 3);
      REQUIRE(spec.prepend == ' ');
    }

    SUBCASE("prepend space specified multiple times") {
      REQUIRE(npf_parse_format_spec("%     u", &spec) == 7);
      REQUIRE(spec.prepend == ' ');
    }

    SUBCASE("prepend space ignored if prepend sign flag is present") {
      REQUIRE(npf_parse_format_spec("%+ u", &spec) == 4);
      REQUIRE(spec.prepend == '+');

      REQUIRE(npf_parse_format_spec("% +u", &spec) == 4);
      REQUIRE(spec.prepend == '+');

      REQUIRE(npf_parse_format_spec("% + + u", &spec) == 7);
      REQUIRE(spec.prepend == '+');
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

    REQUIRE(!npf_parse_format_spec("%#", &spec)); // alternative form alone

    SUBCASE("alternative form off by default") {
      REQUIRE(npf_parse_format_spec("%u", &spec) == 2);
      REQUIRE(!spec.alt_form);
    }

    SUBCASE("alternative form specified") {
      REQUIRE(npf_parse_format_spec("%#u", &spec) == 3);
      REQUIRE(spec.alt_form);
    }

    SUBCASE("alternative form specified multiple times") {
      REQUIRE(npf_parse_format_spec("%#####u", &spec) == 7);
      REQUIRE(spec.alt_form);
    }

    /*
        '0': For d, i, o, u, x, X, a, A, e, E, f, F, g, and G conversions, leading
       zeros (following any indication of sign or base) are used to pad to the field
       width rather than performing space padding, except when converting an
       infinity or NaN. If the 0 and - flags both appear, the 0 flag is ignored. For
       d, i, o, u, x, and X conversions, if a precision is specified, the 0 flag is
       ignored. For other conversions, the behavior is undefined.
    */

    REQUIRE(!npf_parse_format_spec("%0", &spec)); // leading zero alone

    SUBCASE("leading zero off by default") {
      REQUIRE(npf_parse_format_spec("%u", &spec) == 2);
      REQUIRE(!spec.leading_zero_pad);
    }

    SUBCASE("leading zero specified") {
      REQUIRE(npf_parse_format_spec("%0u", &spec) == 3);
      REQUIRE(spec.leading_zero_pad == 1);
    }

    SUBCASE("leading zero specified multiple times") {
      REQUIRE(npf_parse_format_spec("%00000u", &spec) == 7);
      REQUIRE(spec.leading_zero_pad == 1);
    }

    SUBCASE("leading zero with left justification is accepted") {
      REQUIRE(npf_parse_format_spec("%0-u", &spec) == 4);
      REQUIRE(spec.left_justified);
      REQUIRE(spec.leading_zero_pad);

      REQUIRE(npf_parse_format_spec("%0-u", &spec) == 4);
      REQUIRE(spec.left_justified);
      REQUIRE(spec.leading_zero_pad);

      REQUIRE(npf_parse_format_spec("%-0u", &spec) == 4);
      REQUIRE(spec.left_justified);
      REQUIRE(spec.leading_zero_pad);

      REQUIRE(npf_parse_format_spec("%0-0-0-u", &spec) == 8);
      REQUIRE(spec.left_justified);
      REQUIRE(spec.leading_zero_pad);
    }

    SUBCASE("0 flag is parsed when precision is specified") {
      REQUIRE(npf_parse_format_spec("%0.1u", &spec) == 5);
      REQUIRE(spec.leading_zero_pad);
    }
  }

  SUBCASE("field width") {
    /*
       An optional minimum field width. If the converted value has fewer characters
       than the field width, it is padded with spaces (by default) on the left (or
       right, if the left adjustment flag, described later, has been given) to the
       field width. The field width takes the form of an asterisk * (described
       later) or a nonnegative decimal integer. Note that 0 is taken as a flag, not
       as the beginning of a field width.
    */

    SUBCASE("field width is none if not specified") {
      REQUIRE(npf_parse_format_spec("%u", &spec) == 2);
      REQUIRE(spec.field_width_opt == NPF_FMT_SPEC_OPT_NONE);
    }

    SUBCASE("field width star is captured") {
      REQUIRE(npf_parse_format_spec("%*u", &spec) == 3);
      REQUIRE(spec.field_width_opt == NPF_FMT_SPEC_OPT_STAR);
    }

    SUBCASE("field width is literal") {
      REQUIRE(npf_parse_format_spec("%123u", &spec) == 5);
      // A literal field width and an absent one are the same instruction to the
      // caller -- use field_width as-is -- so only STAR gets its own opt value.
      REQUIRE(spec.field_width_opt == NPF_FMT_SPEC_OPT_NONE);
      REQUIRE(spec.field_width == 123);
    }

    SUBCASE("field width digits stop before overflowing int") {
      // The ceiling belongs to npf_vpprintf. All the parser owes it is a value
      // that never wraps negative and still outranks the ceiling, so that the
      // clamp there lands on NPF_FMT_NUM_MAX.
      REQUIRE(npf_parse_format_spec("%2147483648u", &spec) == 12);
      REQUIRE(spec.field_width > NPF_FMT_NUM_MAX);

      REQUIRE(npf_parse_format_spec("%99999999999999999999u", &spec) == 22);
      REQUIRE(spec.field_width > NPF_FMT_NUM_MAX);
    }
  }

  SUBCASE("precision") {
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

    SUBCASE("precision default is 0 before star-args are read") {
      REQUIRE(npf_parse_format_spec("%f", &spec) == 2);
      REQUIRE(spec.prec_opt == NPF_FMT_SPEC_OPT_NONE);
      REQUIRE(spec.prec == 0);

        /*
            Not supported yet

            CHECK_EQUAL(2, npf_parse_format_spec("%g", &spec));
            CHECK_EQUAL(NPF_FMT_SPEC_OPT_NONE, spec.prec_opt);
            CHECK_EQUAL(6, spec.prec);
            CHECK_EQUAL(2, npf_parse_format_spec("%e", &spec));
            CHECK_EQUAL(NPF_FMT_SPEC_OPT_NONE, spec.prec_opt);
            CHECK_EQUAL(6, spec.prec);
        */
    }

    SUBCASE("precision captures star") {
      REQUIRE(npf_parse_format_spec("%.*u", &spec) == 4);
      REQUIRE(spec.prec_opt == NPF_FMT_SPEC_OPT_STAR);
    }

    SUBCASE("precision is literal zero if only a period is specified") {
      REQUIRE(npf_parse_format_spec("%.u", &spec) == 3);
      REQUIRE(spec.prec_opt == NPF_FMT_SPEC_OPT_LITERAL);
      REQUIRE(!spec.prec);
    }

    SUBCASE("precision is literal value if period followed by number") {
      REQUIRE(npf_parse_format_spec("%.12345u", &spec) == 8);
      REQUIRE(spec.prec_opt == NPF_FMT_SPEC_OPT_LITERAL);
      REQUIRE(spec.prec == 12345);
    }

    SUBCASE("precision is none when a negative literal is provided") {
      REQUIRE(npf_parse_format_spec("%.-34u", &spec) == 6);
      REQUIRE(spec.prec_opt == NPF_FMT_SPEC_OPT_NONE);
    }

    SUBCASE("precision digits stop before overflowing int") {
      REQUIRE(npf_parse_format_spec("%.2147483648u", &spec) == 13);
      REQUIRE(spec.prec_opt == NPF_FMT_SPEC_OPT_LITERAL);
      REQUIRE(spec.prec > NPF_FMT_NUM_MAX);

      REQUIRE(npf_parse_format_spec("%.99999999999999999999u", &spec) == 23);
      REQUIRE(spec.prec > NPF_FMT_NUM_MAX);
    }

    SUBCASE("period alone is precision zero") {
      REQUIRE(npf_parse_format_spec("%.u", &spec) == 3);
      REQUIRE(spec.prec_opt == NPF_FMT_SPEC_OPT_LITERAL);
      REQUIRE(spec.prec == 0);
    }
  }

  SUBCASE("length modifiers") {
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

    REQUIRE(!npf_parse_format_spec("%hh", &spec));  // length mod w/o coversion spec.

#if NANOPRINTF_USE_SMALL_FORMAT_SPECIFIERS == 1
    SUBCASE("hh") {
      REQUIRE(npf_parse_format_spec("%hhu", &spec) == 4);
      REQUIRE(spec.length_modifier == NPF_FMT_SPEC_LEN_MOD_CHAR);
    }

    SUBCASE("h") {
      REQUIRE(npf_parse_format_spec("%hu", &spec) == 3);
      REQUIRE(spec.length_modifier == NPF_FMT_SPEC_LEN_MOD_SHORT);
    }
#endif

    SUBCASE("l") {
      REQUIRE(npf_parse_format_spec("%lu", &spec) == 3);
      REQUIRE(spec.length_modifier == NPF_FMT_SPEC_LEN_MOD_LONG);
    }

#if NANOPRINTF_USE_LARGE_FORMAT_SPECIFIERS == 1
    SUBCASE("ll") {
      REQUIRE(npf_parse_format_spec("%llu", &spec) == 4);
      REQUIRE(spec.length_modifier == NPF_FMT_SPEC_LEN_MOD_LARGE_LONG_LONG);
    }

    SUBCASE("j") {
      REQUIRE(npf_parse_format_spec("%ju", &spec) == 3);
      REQUIRE(spec.length_modifier == NPF_FMT_SPEC_LEN_MOD_LARGE_INTMAX);
    }

    SUBCASE("z") {
      REQUIRE(npf_parse_format_spec("%zu", &spec) == 3);
      REQUIRE(spec.length_modifier == NPF_FMT_SPEC_LEN_MOD_LARGE_SIZET);
    }

    SUBCASE("t") {
      REQUIRE(npf_parse_format_spec("%tu", &spec) == 3);
      REQUIRE(spec.length_modifier == NPF_FMT_SPEC_LEN_MOD_LARGE_PTRDIFFT);
    }

    SUBCASE("L") {
      REQUIRE(npf_parse_format_spec("%Lu", &spec) == 3);
      REQUIRE(spec.length_modifier == NPF_FMT_SPEC_LEN_MOD_LONG_DOUBLE);
    }
#endif

#if NANOPRINTF_USE_FIXED_WIDTH_FORMAT_SPECIFIERS == 1
    /*
       wN Specifies that a following b, d, i, o, u, x, or X conversion specifier
       applies to an integer argument with a specific width where N is a positive
       decimal integer with no leading zeros [...] or that a following n
       conversion specifier applies to a pointer to an integer type argument with
       a width of N bits.

       wfN the same, for the fastest minimum-width types.

       nanoprintf resolves these to whichever length modifier already carries the
       stdint type, so the expectation is derived from that type's size rather
       than from the parser's own mapping.
    */
    SUBCASE("wN and wfN") {
      auto const len_mod_for = [](size_t size) -> unsigned {
        if (size == sizeof(signed char)) { return NPF_FMT_SPEC_LEN_MOD_CHAR; }
        if (size == sizeof(short)) { return NPF_FMT_SPEC_LEN_MOD_SHORT; }
        if (size == sizeof(int)) { return NPF_FMT_SPEC_LEN_MOD_NONE; }
#if NANOPRINTF_USE_LARGE_FORMAT_SPECIFIERS == 1
        if (size == sizeof(long)) { return NPF_FMT_SPEC_LEN_MOD_LONG; }
        return NPF_FMT_SPEC_LEN_MOD_LARGE_LONG_LONG;
#else
        // Without 'll' there is nothing wider; NPF_W_BITS_MAX caps N to suit.
        return NPF_FMT_SPEC_LEN_MOD_LONG;
#endif
      };

      REQUIRE(npf_parse_format_spec("%w8u", &spec) == 4);
      REQUIRE(spec.length_modifier == len_mod_for(sizeof(uint_least8_t)));
      REQUIRE(npf_parse_format_spec("%w16u", &spec) == 5);
      REQUIRE(spec.length_modifier == len_mod_for(sizeof(uint_least16_t)));
      REQUIRE(npf_parse_format_spec("%w32u", &spec) == 5);
      REQUIRE(spec.length_modifier == len_mod_for(sizeof(uint_least32_t)));
      REQUIRE(npf_parse_format_spec("%wf8u", &spec) == 5);
      REQUIRE(spec.length_modifier == len_mod_for(sizeof(uint_fast8_t)));
      REQUIRE(npf_parse_format_spec("%wf16u", &spec) == 6);
      REQUIRE(spec.length_modifier == len_mod_for(sizeof(uint_fast16_t)));
      REQUIRE(npf_parse_format_spec("%wf32u", &spec) == 6);
      REQUIRE(spec.length_modifier == len_mod_for(sizeof(uint_fast32_t)));
#if NPF_W_BITS_MAX == 64
      REQUIRE(npf_parse_format_spec("%w64u", &spec) == 5);
      REQUIRE(spec.length_modifier == len_mod_for(sizeof(uint_least64_t)));
      REQUIRE(npf_parse_format_spec("%wf64u", &spec) == 6);
      REQUIRE(spec.length_modifier == len_mod_for(sizeof(uint_fast64_t)));
#else
      REQUIRE(!npf_parse_format_spec("%w64u", &spec));
      REQUIRE(!npf_parse_format_spec("%wf64u", &spec));
#endif

      // 'w' composes with the flags, field width and precision like any other
      // length modifier, and applies to every integer conversion.
      REQUIRE(npf_parse_format_spec("%-+#08.4w16x", &spec) == 12);
      REQUIRE(spec.length_modifier == len_mod_for(sizeof(uint_least16_t)));
      REQUIRE(spec.conv_spec == NPF_FMT_SPEC_CONV_HEX_INT);
      REQUIRE(npf_parse_format_spec("%w8d", &spec) == 4);
      REQUIRE(npf_parse_format_spec("%w8i", &spec) == 4);
      REQUIRE(npf_parse_format_spec("%w8o", &spec) == 4);
      REQUIRE(npf_parse_format_spec("%w8X", &spec) == 4);
#if NANOPRINTF_USE_BINARY_FORMAT_SPECIFIERS == 1
      REQUIRE(npf_parse_format_spec("%w8b", &spec) == 4);
#endif
#if NANOPRINTF_USE_WRITEBACK_FORMAT_SPECIFIERS == 1
      REQUIRE(npf_parse_format_spec("%wf16n", &spec) == 6);
      REQUIRE(spec.length_modifier == len_mod_for(sizeof(int_fast16_t)));
#endif

      // NPF_LM_OF_W picks the family; each half must equal NPF_LM_OF of that
      // family's type, whether or not the two fold together on this target.
      REQUIRE(NPF_LM_OF_W(0, 8) == NPF_LM_OF(INT_LEAST8_MAX));
      REQUIRE(NPF_LM_OF_W(1, 8) == NPF_LM_OF(INT_FAST8_MAX));
      REQUIRE(NPF_LM_OF_W(0, 16) == NPF_LM_OF(INT_LEAST16_MAX));
      REQUIRE(NPF_LM_OF_W(1, 16) == NPF_LM_OF(INT_FAST16_MAX));
      REQUIRE(NPF_LM_OF_W(0, 32) == NPF_LM_OF(INT_LEAST32_MAX));
      REQUIRE(NPF_LM_OF_W(1, 32) == NPF_LM_OF(INT_FAST32_MAX));
#if NPF_W_BITS_MAX == 64
      REQUIRE(NPF_LM_OF_W(0, 64) == NPF_LM_OF(INT_LEAST64_MAX));
      REQUIRE(NPF_LM_OF_W(1, 64) == NPF_LM_OF(INT_FAST64_MAX));
#endif

      // A width never resolves to a modifier whose type is too narrow to hold it.
      auto const mod_size = [](unsigned mod) -> size_t {
        switch (mod) {
          case NPF_FMT_SPEC_LEN_MOD_CHAR: return sizeof(signed char);
          case NPF_FMT_SPEC_LEN_MOD_SHORT: return sizeof(short);
          case NPF_FMT_SPEC_LEN_MOD_NONE: return sizeof(int);
          case NPF_FMT_SPEC_LEN_MOD_LONG: return sizeof(long);
          default: return sizeof(long long);
        }
      };
      REQUIRE(mod_size(NPF_LM_OF_W(0, 8)) == sizeof(int_least8_t));
      REQUIRE(mod_size(NPF_LM_OF_W(1, 8)) == sizeof(int_fast8_t));
      REQUIRE(mod_size(NPF_LM_OF_W(0, 16)) == sizeof(int_least16_t));
      REQUIRE(mod_size(NPF_LM_OF_W(1, 16)) == sizeof(int_fast16_t));
      REQUIRE(mod_size(NPF_LM_OF_W(0, 32)) == sizeof(int_least32_t));
      REQUIRE(mod_size(NPF_LM_OF_W(1, 32)) == sizeof(int_fast32_t));
#if NPF_W_BITS_MAX == 64
      REQUIRE(mod_size(NPF_LM_OF_W(0, 64)) == sizeof(int_least64_t));
      REQUIRE(mod_size(NPF_LM_OF_W(1, 64)) == sizeof(int_fast64_t));
#endif

      // A 'w' specifier consumes exactly its own characters and nothing after.
      REQUIRE(npf_parse_format_spec("%w8dxyz", &spec) == 4);
      REQUIRE(npf_parse_format_spec("%wf16uxyz", &spec) == 6);
      REQUIRE(npf_parse_format_spec("%-8.4w16dxyz", &spec) == 9);

      // Only the widths stdint.h must define are supported.
      REQUIRE(!npf_parse_format_spec("%w", &spec));
      REQUIRE(!npf_parse_format_spec("%wf", &spec));
      REQUIRE(!npf_parse_format_spec("%wd", &spec));
      REQUIRE(!npf_parse_format_spec("%wfd", &spec));
      REQUIRE(!npf_parse_format_spec("%w0d", &spec));
      REQUIRE(!npf_parse_format_spec("%w1d", &spec));
      REQUIRE(!npf_parse_format_spec("%w08d", &spec));   // N has no leading zeros
      REQUIRE(!npf_parse_format_spec("%wf08d", &spec));
      REQUIRE(!npf_parse_format_spec("%w7d", &spec));
      REQUIRE(!npf_parse_format_spec("%w9d", &spec));
      REQUIRE(!npf_parse_format_spec("%w24d", &spec));
      REQUIRE(!npf_parse_format_spec("%w33d", &spec));
      REQUIRE(!npf_parse_format_spec("%w48d", &spec));
      REQUIRE(!npf_parse_format_spec("%w65d", &spec));
      REQUIRE(!npf_parse_format_spec("%w99d", &spec));
      REQUIRE(!npf_parse_format_spec("%w128d", &spec));
      REQUIRE(!npf_parse_format_spec("%w160d", &spec));
      REQUIRE(!npf_parse_format_spec("%w644d", &spec));
      REQUIRE(!npf_parse_format_spec("%w4294967296d", &spec));
      REQUIRE(!npf_parse_format_spec("%wf7d", &spec));
      REQUIRE(!npf_parse_format_spec("%wf24d", &spec));
      REQUIRE(!npf_parse_format_spec("%wf128d", &spec));
      REQUIRE(!npf_parse_format_spec("%ww8d", &spec));
      REQUIRE(!npf_parse_format_spec("%wff8d", &spec));
      // a modifier with no conversion behind it, and truncated widths
      REQUIRE(!npf_parse_format_spec("%w8", &spec));
      REQUIRE(!npf_parse_format_spec("%w16", &spec));
      REQUIRE(!npf_parse_format_spec("%w3", &spec));
      REQUIRE(!npf_parse_format_spec("%w6", &spec));
      REQUIRE(!npf_parse_format_spec("%wf1", &spec));

      // 'w' before a conversion it does not apply to is undefined behavior, and
      // the parser no more rejects it than it rejects "%hhf": the modifier parses
      // and the conversion ignores it.
      REQUIRE(npf_parse_format_spec("%w8f", &spec) == 4);
      REQUIRE(npf_parse_format_spec("%w16s", &spec) == 5);
      REQUIRE(npf_parse_format_spec("%w8c", &spec) == 4);
    }
#endif
  }

  SUBCASE("conversion specifiers") {
    // All conversion specifiers are defined in 7.21.6.1.8

    SUBCASE("% literal") {
      REQUIRE(npf_parse_format_spec("%%", &spec) == 2);
      REQUIRE(spec.conv_spec == NPF_FMT_SPEC_CONV_PERCENT);
    }

    SUBCASE("c") {
      REQUIRE(npf_parse_format_spec("%c", &spec) == 2);
      REQUIRE(spec.conv_spec == NPF_FMT_SPEC_CONV_CHAR);
    }

    SUBCASE("s") {
      REQUIRE(npf_parse_format_spec("%s", &spec) == 2);
      REQUIRE(spec.conv_spec == NPF_FMT_SPEC_CONV_STRING);
    }

    SUBCASE("s preserves leading 0") {
      REQUIRE(npf_parse_format_spec("%0s", &spec) == 3);
      REQUIRE(spec.leading_zero_pad);
    }

    SUBCASE("string negative left-justify field width") {
      REQUIRE(npf_parse_format_spec("%-15s", &spec) == 5);
      REQUIRE(spec.conv_spec == NPF_FMT_SPEC_CONV_STRING);
      REQUIRE(spec.left_justified);
      REQUIRE(spec.field_width == 15);
    }

    SUBCASE("i") {
      REQUIRE(npf_parse_format_spec("%i", &spec) == 2);
      REQUIRE(spec.conv_spec == NPF_FMT_SPEC_CONV_SIGNED_INT);
    }

    SUBCASE("d") {
      REQUIRE(npf_parse_format_spec("%d", &spec) == 2);
      REQUIRE(spec.conv_spec == NPF_FMT_SPEC_CONV_SIGNED_INT);
    }

    SUBCASE("o") {
      REQUIRE(npf_parse_format_spec("%o", &spec) == 2);
      REQUIRE(spec.conv_spec == NPF_FMT_SPEC_CONV_OCTAL);
    }

    SUBCASE("x") {
      REQUIRE(npf_parse_format_spec("%x", &spec) == 2);
      REQUIRE(spec.conv_spec == NPF_FMT_SPEC_CONV_HEX_INT);
      REQUIRE(spec.case_adjust == 'a' - 'A');
    }

    SUBCASE("X") {
      REQUIRE(npf_parse_format_spec("%X", &spec) == 2);
      REQUIRE(spec.conv_spec == NPF_FMT_SPEC_CONV_HEX_INT);
      REQUIRE(spec.case_adjust == 0);
    }

    SUBCASE("u") {
      REQUIRE(npf_parse_format_spec("%u", &spec) == 2);
      REQUIRE(spec.conv_spec == NPF_FMT_SPEC_CONV_UNSIGNED_INT);
    }

    SUBCASE("n") {
      REQUIRE(npf_parse_format_spec("%n", &spec) == 2);
      REQUIRE(spec.conv_spec == NPF_FMT_SPEC_CONV_WRITEBACK);
    }

    SUBCASE("n clears precision") {
      REQUIRE(npf_parse_format_spec("%.4n", &spec) == 4);
      REQUIRE(spec.conv_spec == NPF_FMT_SPEC_CONV_WRITEBACK);
      REQUIRE(spec.prec_opt == NPF_FMT_SPEC_OPT_LITERAL);
      REQUIRE(spec.prec == 4);
    }

    SUBCASE("p") {
      REQUIRE(npf_parse_format_spec("%p", &spec) == 2);
      REQUIRE(spec.conv_spec == NPF_FMT_SPEC_CONV_POINTER);
    }

    SUBCASE("f") {
      REQUIRE(npf_parse_format_spec("%f", &spec) == 2);
      REQUIRE(spec.conv_spec == NPF_FMT_SPEC_CONV_FLOAT_DEC);
      REQUIRE(spec.case_adjust == 'a' - 'A');
    }

    SUBCASE("F") {
      REQUIRE(npf_parse_format_spec("%F", &spec) == 2);
      REQUIRE(spec.conv_spec == NPF_FMT_SPEC_CONV_FLOAT_DEC);
      REQUIRE(spec.case_adjust == 0);
    }

    SUBCASE("e") {
      REQUIRE(npf_parse_format_spec("%e", &spec) == 2);
      REQUIRE(spec.conv_spec == NPF_FMT_SPEC_CONV_FLOAT_SCI);
      REQUIRE(spec.case_adjust == 'a' - 'A');
    }

    SUBCASE("E") {
      REQUIRE(npf_parse_format_spec("%E", &spec) == 2);
      REQUIRE(spec.conv_spec == NPF_FMT_SPEC_CONV_FLOAT_SCI);
      REQUIRE(spec.case_adjust == 0);
    }

    SUBCASE("g") {
      REQUIRE(npf_parse_format_spec("%g", &spec) == 2);
      REQUIRE(spec.conv_spec == NPF_FMT_SPEC_CONV_FLOAT_SHORTEST);
      REQUIRE(spec.case_adjust == 'a' - 'A');
    }

    SUBCASE("G") {
      REQUIRE(npf_parse_format_spec("%G", &spec) == 2);
      REQUIRE(spec.conv_spec == NPF_FMT_SPEC_CONV_FLOAT_SHORTEST);
      REQUIRE(spec.case_adjust == 0);
    }

    SUBCASE("a") {
      REQUIRE(npf_parse_format_spec("%a", &spec) == 2);
      REQUIRE(spec.conv_spec == NPF_FMT_SPEC_CONV_FLOAT_HEX);
      REQUIRE(spec.case_adjust == 'a' - 'A');
    }

    SUBCASE("A") {
      REQUIRE(npf_parse_format_spec("%A", &spec) == 2);
      REQUIRE(spec.conv_spec == NPF_FMT_SPEC_CONV_FLOAT_HEX);
      REQUIRE(spec.case_adjust == 0);
    }

#if NANOPRINTF_USE_BINARY_FORMAT_SPECIFIERS == 1
    SUBCASE("b") {
      REQUIRE(npf_parse_format_spec("%b", &spec) == 2);
      REQUIRE(spec.conv_spec == NPF_FMT_SPEC_CONV_BINARY);
    }
#endif
  }
}

