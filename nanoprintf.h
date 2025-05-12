/* nanoprintf v0.5.5: a tiny embeddable printf replacement written in C.
   https://github.com/charlesnicholson/nanoprintf
   charles.nicholson+nanoprintf@gmail.com
   dual-licensed under 0bsd and unlicense, take your pick. see eof for details. */

#ifndef NANOPRINTF_H_INCLUDED
#define NANOPRINTF_H_INCLUDED

#include <stdarg.h>
#include <stddef.h>

// Define this to fully sandbox nanoprintf inside of a translation unit.
#ifdef NANOPRINTF_VISIBILITY_STATIC
  #define NPF_VISIBILITY static
#else
  #define NPF_VISIBILITY extern
#endif

#if defined(__clang__) || defined(__GNUC__) || defined(__GNUG__)
  #define NPF_PRINTF_ATTR(FORMAT_INDEX, VARGS_INDEX) \
    __attribute__((format(printf, FORMAT_INDEX, VARGS_INDEX)))
#else
  #define NPF_PRINTF_ATTR(FORMAT_INDEX, VARGS_INDEX)
#endif

// Public API

#ifdef __cplusplus
extern "C" {
#endif

// The npf_ functions all return the number of bytes required to express the
// fully-formatted string, not including the null terminator character.
// The npf_ functions do not return negative values, since the lack of 'l' length
// modifier support makes encoding errors impossible.

NPF_VISIBILITY int npf_snprintf(
  char *buffer, size_t bufsz, const char *format, ...) NPF_PRINTF_ATTR(3, 4);

NPF_VISIBILITY int npf_vsnprintf(
  char *buffer, size_t bufsz, char const *format, va_list vlist) NPF_PRINTF_ATTR(3, 0);

typedef void (*npf_putc)(int c, void *ctx);
NPF_VISIBILITY int npf_pprintf(
  npf_putc pc, void *pc_ctx, char const *format, ...) NPF_PRINTF_ATTR(3, 4);

NPF_VISIBILITY int npf_vpprintf(
  npf_putc pc, void *pc_ctx, char const *format, va_list vlist) NPF_PRINTF_ATTR(3, 0);

#ifdef __cplusplus
}
#endif

#endif // NANOPRINTF_H_INCLUDED

/* The implementation of nanoprintf begins here, to be compiled only if
   NANOPRINTF_IMPLEMENTATION is defined. In a multi-file library what follows would
   be nanoprintf.c. */

#ifdef NANOPRINTF_IMPLEMENTATION

#ifndef NANOPRINTF_IMPLEMENTATION_INCLUDED
#define NANOPRINTF_IMPLEMENTATION_INCLUDED

#include <limits.h>
#include <stdint.h>

/*
The conversion buffer must be large enough for the largest intermediate result,
which depends on the conversion specifier.
The sign, and space/0 padding are handled outside of this buffer. The precision
for non-float specifiers (integers, %s, %c) is handled outside as well, whereas
the precision for float specifiers is handled in the buffer itself.
The "0x" prefixes are handled outside, with the exception of the octal "0",
which is placed in the buffer.
Requirements for each specifier:
%a: "<digit>.<prec_digits>p<sign><exp_digits>"
  exp_digits is <= 4 (the exponent is the decimal representation of the binary
  exponent, which is in [-1077, 1023], for float64 -- [-1023, 1023] for the
  exponent, [-1074, 1023] considering denormals, [-1077, 1023] considering the
  freedom in choosing the most significant digit).
  The precision is user-provided, so there are no bounds on the buffer size.
  However, float64 has 52+1 bits of mantissa, which means 53*log16(2) ~= 13.25
  hex digits of precision (including the most significant digit), which means
  at most 14 chars.
  So, any float64 can be accurately represented with 1+1+13+1+1+4 = 21 chars
%b: does not use the buffer, outputs the chars directly
%c: does not use the buffer
%d/i: supported integers are at most 64 bits, so <= 20 digits
%e: not supported. When supported, it will use the buffer for:
  "<digit>.<prec_digits>e<sign><exp_digits>"
  exp_digits is <= 3 (the exponent is the decimal representation of the decimal
  exponent, which is in [-324, 308], for float64)
  The precision is user-provided, so there are no bounds on the buffer size.
  However, float64 has 52+1 bits of mantissa, which means 53*log10(2) ~= 15.95
  decimal digits of precision (including the most significant digit), which
  means at most 16 chars.
  So, any float64 can be accurately represented with 1+1+15+1+1+3 = 22 chars
%f: "<integer_digits>.<prec_digits>"
  The precision is user-provided, so there are no bounds on the buffer size.
  However, float64 has 52+1 bits of mantissa, which means 53*log10(2) ~= 15.95
  decimal digits of precision (in total, among the integer and fractional parts),
  which means at most 16 chars.
  So, any float64 can be accurately represented with 1+16 = 17 chars
%g: not supported. When supported, it will use either %e or %f, with possible
  final tweaks to remove (never add) some digits.
  So, the same limits and considerations as %e and %f apply.
%n: does not use the buffer
%o: "0<octal_digits>". supported integers are at most 64 bits, so <= 22 digits
  Plus the "0" prefix, we need 23 chars.
%p: equivalent to %x with appropriate flags. The same limits apply
%s: does not use the buffer, outputs the chars directly
%u: supported integers are at most 64 bits, so <= 20 digits
%x: supported integers are at most 64 bits, so <= 16 digits

In summary: the minimum buffer size is 23, so that we can fit any integer,
without adding overrun checks. For floats, 23 is also sufficient for perfect
representations (ie representation that can distinguish any two different float
values), for exponential formats, and for the %f format if the number is "close
to 0", ie if no extra space is required for leading/trailing zeros.
So, the default value is appropriate for most cases. The user might want to
increase it to fit more (useless) precision digits for exponential-format floats,
or to fit very large or very small %f-format floats.
*/
#ifndef NANOPRINTF_CONVERSION_BUFFER_SIZE
  #define NANOPRINTF_CONVERSION_BUFFER_SIZE    23
#endif
#if NANOPRINTF_CONVERSION_BUFFER_SIZE < 23
  #error The size of the conversion buffer must be at least 23 bytes.
#endif

// Pick reasonable defaults if nothing's been configured.
#if !defined(NANOPRINTF_USE_FIELD_WIDTH_FORMAT_SPECIFIERS) && \
    !defined(NANOPRINTF_USE_PRECISION_FORMAT_SPECIFIERS) && \
    !defined(NANOPRINTF_USE_FLOAT_FORMAT_SPECIFIERS) && \
    !defined(NANOPRINTF_USE_LARGE_FORMAT_SPECIFIERS) && \
    !defined(NANOPRINTF_USE_BINARY_FORMAT_SPECIFIERS) && \
    !defined(NANOPRINTF_USE_WRITEBACK_FORMAT_SPECIFIERS)
  #define NANOPRINTF_USE_FIELD_WIDTH_FORMAT_SPECIFIERS 1
  #define NANOPRINTF_USE_PRECISION_FORMAT_SPECIFIERS 1
  #define NANOPRINTF_USE_FLOAT_FORMAT_SPECIFIERS 1
  #define NANOPRINTF_USE_LARGE_FORMAT_SPECIFIERS 0
  #define NANOPRINTF_USE_BINARY_FORMAT_SPECIFIERS 0
  #define NANOPRINTF_USE_WRITEBACK_FORMAT_SPECIFIERS 0
#endif

// If anything's been configured, everything must be configured.
#ifndef NANOPRINTF_USE_FIELD_WIDTH_FORMAT_SPECIFIERS
  #error NANOPRINTF_USE_FIELD_WIDTH_FORMAT_SPECIFIERS must be #defined to 0 or 1
#endif
#ifndef NANOPRINTF_USE_PRECISION_FORMAT_SPECIFIERS
  #error NANOPRINTF_USE_PRECISION_FORMAT_SPECIFIERS must be #defined to 0 or 1
#endif
#ifndef NANOPRINTF_USE_FLOAT_FORMAT_SPECIFIERS
  #error NANOPRINTF_USE_FLOAT_FORMAT_SPECIFIERS must be #defined to 0 or 1
#endif
#ifndef NANOPRINTF_USE_LARGE_FORMAT_SPECIFIERS
  #error NANOPRINTF_USE_LARGE_FORMAT_SPECIFIERS must be #defined to 0 or 1
#endif
#ifndef NANOPRINTF_USE_BINARY_FORMAT_SPECIFIERS
  #error NANOPRINTF_USE_BINARY_FORMAT_SPECIFIERS must be #defined to 0 or 1
#endif
#ifndef NANOPRINTF_USE_WRITEBACK_FORMAT_SPECIFIERS
  #error NANOPRINTF_USE_WRITEBACK_FORMAT_SPECIFIERS must be #defined to 0 or 1
#endif

#if !defined(NANOPRINTF_USE_FLOAT_HEX_FORMAT_SPECIFIER)
  #define NANOPRINTF_USE_FLOAT_HEX_FORMAT_SPECIFIER    0
#endif
#if !defined(NANOPRINTF_USE_FLOAT_SCI_FORMAT_SPECIFIER)
  #define NANOPRINTF_USE_FLOAT_SCI_FORMAT_SPECIFIER    0
#endif
#if !defined(NANOPRINTF_USE_FLOAT_SHORTEST_FORMAT_SPECIFIER)
  #define NANOPRINTF_USE_FLOAT_SHORTEST_FORMAT_SPECIFIER    0
#endif
#if !defined(NANOPRINTF_USE_FLOAT_DEC_FORMAT_SPECIFIER)
  #define NANOPRINTF_USE_FLOAT_DEC_FORMAT_SPECIFIER    1
#endif

#if NANOPRINTF_USE_FLOAT_FORMAT_SPECIFIERS && !( \
    NANOPRINTF_USE_FLOAT_DEC_FORMAT_SPECIFIER == 1 \
    || NANOPRINTF_USE_FLOAT_SCI_FORMAT_SPECIFIER == 1 \
    || NANOPRINTF_USE_FLOAT_SHORTEST_FORMAT_SPECIFIER == 1 \
    || NANOPRINTF_USE_FLOAT_HEX_FORMAT_SPECIFIER == 1)
  #error Float support needs at least one float-specifier to be enabled
#endif
#if NANOPRINTF_USE_FLOAT_SCI_FORMAT_SPECIFIER == 1
  #error The %e specifier is not supported yet
#endif
#if NANOPRINTF_USE_FLOAT_SHORTEST_FORMAT_SPECIFIER == 1
  #error The %g specifier is not supported yet
#endif

// Ensure flags are compatible.
#if (NANOPRINTF_USE_FLOAT_FORMAT_SPECIFIERS == 1) && \
    (NANOPRINTF_USE_PRECISION_FORMAT_SPECIFIERS == 0)
  #error Precision format specifiers must be enabled if float support is enabled.
#endif

// intmax_t / uintmax_t require stdint from c99 / c++11
#if NANOPRINTF_USE_LARGE_FORMAT_SPECIFIERS == 1
  #ifndef _MSC_VER
    #ifdef __cplusplus
      #if __cplusplus < 201103L
        #error large format specifier support requires C++11 or later.
      #endif
    #else
      #if __STDC_VERSION__ < 199409L
        #error nanoprintf requires C99 or later.
      #endif
    #endif
  #endif
#endif

// Figure out if we can disable warnings with pragmas.
#ifdef __clang__
  #define NANOPRINTF_CLANG 1
  #define NANOPRINTF_GCC_PAST_4_6 0
#else
  #define NANOPRINTF_CLANG 0
  #if defined(__GNUC__) && ((__GNUC__ > 4) || ((__GNUC__ == 4) && (__GNUC_MINOR__ > 6)))
    #define NANOPRINTF_GCC_PAST_4_6 1
  #else
    #define NANOPRINTF_GCC_PAST_4_6 0
  #endif
#endif

#if NANOPRINTF_CLANG || NANOPRINTF_GCC_PAST_4_6
  #define NANOPRINTF_HAVE_GCC_WARNING_PRAGMAS 1
#else
  #define NANOPRINTF_HAVE_GCC_WARNING_PRAGMAS 0
#endif

#if NANOPRINTF_HAVE_GCC_WARNING_PRAGMAS
  #pragma GCC diagnostic push
  #pragma GCC diagnostic ignored "-Wunused-function"
  #pragma GCC diagnostic ignored "-Wimplicit-fallthrough"
  #pragma GCC diagnostic ignored "-Wunused-label"
  #ifdef __cplusplus
    #pragma GCC diagnostic ignored "-Wold-style-cast"
  #endif
  #pragma GCC diagnostic ignored "-Wpadded"
  #pragma GCC diagnostic ignored "-Wfloat-equal"
  #if NANOPRINTF_CLANG
    #pragma GCC diagnostic ignored "-Wc++98-compat-pedantic"
    #pragma GCC diagnostic ignored "-Wcovered-switch-default"
    #pragma GCC diagnostic ignored "-Wdeclaration-after-statement"
    #pragma GCC diagnostic ignored "-Wzero-as-null-pointer-constant"
    #ifndef __APPLE__
      #pragma GCC diagnostic ignored "-Wunsafe-buffer-usage"
    #endif
  #elif NANOPRINTF_GCC_PAST_4_6
    #pragma GCC diagnostic ignored "-Wmaybe-uninitialized"
  #endif
#endif

#ifdef _MSC_VER
  #pragma warning(push)
  #pragma warning(disable:4619) // there is no warning number 'number'
  // C4619 has to be disabled first!
  #pragma warning(disable:4102) // unused label
  #pragma warning(disable:4127) // conditional expression is constant
  #pragma warning(disable:4505) // unreferenced local function has been removed
  #pragma warning(disable:4514) // unreferenced inline function has been removed
  #pragma warning(disable:4701) // potentially uninitialized local variable used
  #pragma warning(disable:4706) // assignment within conditional expression
  #pragma warning(disable:4710) // function not inlined
  #pragma warning(disable:4711) // function selected for inline expansion
  #pragma warning(disable:4820) // padding added after struct member
  #pragma warning(disable:5039) // potentially throwing function passed to extern C function
  #pragma warning(disable:5045) // compiler will insert Spectre mitigation for memory load
  #pragma warning(disable:5262) // implicit switch fall-through
  #pragma warning(disable:26812) // enum type is unscoped
#endif

#if defined(__clang__) || defined(__GNUC__) || defined(__GNUG__)
  #define NPF_NOINLINE __attribute__((noinline))
  #define NPF_FORCE_INLINE inline __attribute__((always_inline))
#elif defined(_MSC_VER)
  #define NPF_NOINLINE __declspec(noinline)
  #define NPF_FORCE_INLINE inline __forceinline
#else
  #define NPF_NOINLINE
  #define NPF_FORCE_INLINE
#endif

#if (NANOPRINTF_USE_FIELD_WIDTH_FORMAT_SPECIFIERS == 1) || \
    (NANOPRINTF_USE_PRECISION_FORMAT_SPECIFIERS == 1)
enum {
  NPF_FMT_SPEC_OPT_NONE,
  NPF_FMT_SPEC_OPT_LITERAL,
  NPF_FMT_SPEC_OPT_STAR,
};
#endif

enum {
  NPF_FMT_SPEC_LEN_MOD_NONE,
  NPF_FMT_SPEC_LEN_MOD_SHORT,       // 'h'
  NPF_FMT_SPEC_LEN_MOD_LONG_DOUBLE, // 'L'
  NPF_FMT_SPEC_LEN_MOD_CHAR,        // 'hh'
  NPF_FMT_SPEC_LEN_MOD_LONG,        // 'l'
#if NANOPRINTF_USE_LARGE_FORMAT_SPECIFIERS == 1
  NPF_FMT_SPEC_LEN_MOD_LARGE_LONG_LONG, // 'll'
  NPF_FMT_SPEC_LEN_MOD_LARGE_INTMAX,    // 'j'
  NPF_FMT_SPEC_LEN_MOD_LARGE_SIZET,     // 'z'
  NPF_FMT_SPEC_LEN_MOD_LARGE_PTRDIFFT,  // 't'
#endif
};

enum {
  NPF_FMT_SPEC_CONV_NONE,
  NPF_FMT_SPEC_CONV_PERCENT,      // '%'
  NPF_FMT_SPEC_CONV_CHAR,         // 'c'
  NPF_FMT_SPEC_CONV_STRING,       // 's'
  NPF_FMT_SPEC_CONV_SIGNED_INT,   // 'i', 'd'
#if NANOPRINTF_USE_BINARY_FORMAT_SPECIFIERS == 1
  NPF_FMT_SPEC_CONV_BINARY,       // 'b'
#endif
  NPF_FMT_SPEC_CONV_OCTAL,        // 'o'
  NPF_FMT_SPEC_CONV_HEX_INT,      // 'x', 'X'
  NPF_FMT_SPEC_CONV_UNSIGNED_INT, // 'u'
  NPF_FMT_SPEC_CONV_POINTER,      // 'p'
#if NANOPRINTF_USE_WRITEBACK_FORMAT_SPECIFIERS == 1
  NPF_FMT_SPEC_CONV_WRITEBACK,    // 'n'
#endif
#if NANOPRINTF_USE_FLOAT_FORMAT_SPECIFIERS == 1
  #if NANOPRINTF_USE_FLOAT_DEC_FORMAT_SPECIFIER
    NPF_FMT_SPEC_CONV_FLOAT_DEC,      // 'f', 'F'
  #endif
  #if NANOPRINTF_USE_FLOAT_SCI_FORMAT_SPECIFIER
    NPF_FMT_SPEC_CONV_FLOAT_SCI,      // 'e', 'E'
  #endif
  #if NANOPRINTF_USE_FLOAT_SHORTEST_FORMAT_SPECIFIER
    NPF_FMT_SPEC_CONV_FLOAT_SHORTEST, // 'g', 'G'
  #endif
  #if NANOPRINTF_USE_FLOAT_HEX_FORMAT_SPECIFIER
    NPF_FMT_SPEC_CONV_FLOAT_HEX,      // 'a', 'A'
  #endif
#endif
};

typedef struct npf_format_spec {
#if NANOPRINTF_USE_FIELD_WIDTH_FORMAT_SPECIFIERS == 1
  int field_width;
  uint8_t field_width_opt;
  char left_justified;   // '-'
  char leading_zero_pad; // '0'
#endif
#if NANOPRINTF_USE_PRECISION_FORMAT_SPECIFIERS == 1
  int prec;
  uint8_t prec_opt;
#endif
  char prepend;          // ' ' or '+'
  char alt_form;         // '#'
  char case_adjust;      // 'a' - 'A'
  uint8_t length_modifier;
  uint8_t conv_spec;
} npf_format_spec_t;

#if NANOPRINTF_USE_LARGE_FORMAT_SPECIFIERS == 0
  typedef long npf_int_t;
  typedef unsigned long npf_uint_t;
#else
  typedef intmax_t npf_int_t;
  typedef uintmax_t npf_uint_t;
#endif

typedef struct npf_bufputc_ctx {
  char *dst;
  size_t len;
  size_t cur;
} npf_bufputc_ctx_t;

#if NANOPRINTF_USE_LARGE_FORMAT_SPECIFIERS == 1
  typedef char npf_size_is_ptrdiff[(sizeof(size_t) == sizeof(ptrdiff_t)) ? 1 : -1];
  typedef ptrdiff_t npf_ssize_t;
#endif

#ifdef _MSC_VER
  #include <intrin.h>
#endif

#define NPF_MIN(a, b)    ((a) <= (b) ? (a) : (b))
#define NPF_MAX(a, b)    ((a) >= (b) ? (a) : (b))
static int npf_max(int x, int y) { return (x > y) ? x : y; }

#if NANOPRINTF_USE_FLOAT_FORMAT_SPECIFIERS == 1

#include <float.h>

#if (DBL_MANT_DIG <= 11) && (DBL_MAX_EXP <= 16)
  typedef uint_fast16_t npf_double_bin_t;
  typedef int_fast8_t npf_ftoa_exp_t;
#elif (DBL_MANT_DIG <= 24) && (DBL_MAX_EXP <= 128)
  typedef uint_fast32_t npf_double_bin_t;
  typedef int_fast8_t npf_ftoa_exp_t;
#elif (DBL_MANT_DIG <= 53) && (DBL_MAX_EXP <= 1024)
  typedef uint_fast64_t npf_double_bin_t;
  typedef int_fast16_t npf_ftoa_exp_t;
#else
  #error Unsupported width of the double type.
#endif

// The floating point conversion code works with an unsigned integer type of any size.
#ifndef NANOPRINTF_CONVERSION_FLOAT_TYPE
  #define NANOPRINTF_CONVERSION_FLOAT_TYPE unsigned int
#endif
typedef NANOPRINTF_CONVERSION_FLOAT_TYPE npf_ftoa_man_t;

#if (NANOPRINTF_CONVERSION_BUFFER_SIZE <= UINT_FAST8_MAX) && (UINT_FAST8_MAX <= INT_MAX)
  typedef uint_fast8_t npf_ftoa_dec_t;
#else
  typedef int npf_ftoa_dec_t;
#endif

enum {
  NPF_DOUBLE_EXP_MASK = DBL_MAX_EXP * 2 - 1,
  NPF_DOUBLE_EXP_BIAS = DBL_MAX_EXP - 1,
  NPF_DOUBLE_MAN_BITS = DBL_MANT_DIG - 1, // these are the explicitly-stored bits, whereas DBL_MANT_DIG includes the leading implicit 1
  NPF_DOUBLE_BIN_BITS = sizeof(npf_double_bin_t) * CHAR_BIT,
  NPF_DOUBLE_SIGN_POS = sizeof(double) * CHAR_BIT - 1,
  NPF_FTOA_MAN_BITS   = sizeof(npf_ftoa_man_t) * CHAR_BIT,
  NPF_FTOA_SHIFT_BITS =
    ((NPF_FTOA_MAN_BITS < DBL_MANT_DIG) ? NPF_FTOA_MAN_BITS : DBL_MANT_DIG) - 1
};
#endif

static int npf_parse_format_spec(char const *format, npf_format_spec_t *out_spec) {
  char const *cur = format;

#if NANOPRINTF_USE_FIELD_WIDTH_FORMAT_SPECIFIERS == 1
  out_spec->left_justified = 0;
  out_spec->leading_zero_pad = 0;
#endif
  out_spec->case_adjust = 'a' - 'A'; // lowercase
  out_spec->prepend = 0;
  out_spec->alt_form = 0;

  while (*++cur) { // cur points at the leading '%' character
    switch (*cur) { // Optional flags
#if NANOPRINTF_USE_FIELD_WIDTH_FORMAT_SPECIFIERS == 1
      case '-': out_spec->left_justified = '-'; continue;
      case '0': out_spec->leading_zero_pad = 1; continue;
#endif
      case '+': out_spec->prepend = '+'; continue;
      case ' ': if (out_spec->prepend == 0) { out_spec->prepend = ' '; } continue;
      case '#': out_spec->alt_form = '#'; continue;
      default: break;
    }
    break;
  }

#if NANOPRINTF_USE_FIELD_WIDTH_FORMAT_SPECIFIERS == 1
  out_spec->field_width = 0;
  out_spec->field_width_opt = NPF_FMT_SPEC_OPT_NONE;
  if (*cur == '*') {
    out_spec->field_width_opt = NPF_FMT_SPEC_OPT_STAR;
    ++cur;
  } else {
    while ((*cur >= '0') && (*cur <= '9')) {
      out_spec->field_width_opt = NPF_FMT_SPEC_OPT_LITERAL;
      out_spec->field_width = (out_spec->field_width * 10) + (*cur++ - '0');
    }
  }
#endif

#if NANOPRINTF_USE_PRECISION_FORMAT_SPECIFIERS == 1
  out_spec->prec = 0;
  out_spec->prec_opt = NPF_FMT_SPEC_OPT_NONE;
  if (*cur == '.') {
    ++cur;
    if (*cur == '*') {
      out_spec->prec_opt = NPF_FMT_SPEC_OPT_STAR;
      ++cur;
    } else {
      if (*cur == '-') {
        ++cur;
      } else {
        out_spec->prec_opt = NPF_FMT_SPEC_OPT_LITERAL;
      }
      while ((*cur >= '0') && (*cur <= '9')) {
        out_spec->prec = (out_spec->prec * 10) + (*cur++ - '0');
      }
    }
  }
#endif

  uint_fast8_t tmp_conv = NPF_FMT_SPEC_CONV_NONE;
  out_spec->length_modifier = NPF_FMT_SPEC_LEN_MOD_NONE;
  switch (*cur++) { // Length modifier
    case 'h':
      out_spec->length_modifier = NPF_FMT_SPEC_LEN_MOD_SHORT;
      if (*cur == 'h') {
        out_spec->length_modifier = NPF_FMT_SPEC_LEN_MOD_CHAR;
        ++cur;
      }
      break;
    case 'l':
      out_spec->length_modifier = NPF_FMT_SPEC_LEN_MOD_LONG;
#if NANOPRINTF_USE_LARGE_FORMAT_SPECIFIERS == 1
      if (*cur == 'l') {
        out_spec->length_modifier = NPF_FMT_SPEC_LEN_MOD_LARGE_LONG_LONG;
        ++cur;
      }
#endif
      break;
#if NANOPRINTF_USE_FLOAT_FORMAT_SPECIFIERS == 1
    case 'L': out_spec->length_modifier = NPF_FMT_SPEC_LEN_MOD_LONG_DOUBLE; break;
#endif
#if NANOPRINTF_USE_LARGE_FORMAT_SPECIFIERS == 1
    case 'j': out_spec->length_modifier = NPF_FMT_SPEC_LEN_MOD_LARGE_INTMAX; break;
    case 'z': out_spec->length_modifier = NPF_FMT_SPEC_LEN_MOD_LARGE_SIZET; break;
    case 't': out_spec->length_modifier = NPF_FMT_SPEC_LEN_MOD_LARGE_PTRDIFFT; break;
#endif
    default: --cur; break;
  }

  switch (*cur++) { // Conversion specifier
    case '%': out_spec->conv_spec = NPF_FMT_SPEC_CONV_PERCENT;
#if NANOPRINTF_USE_PRECISION_FORMAT_SPECIFIERS == 1
      out_spec->prec_opt = NPF_FMT_SPEC_OPT_NONE;
#endif
      break;

    case 'c': out_spec->conv_spec = NPF_FMT_SPEC_CONV_CHAR;
#if NANOPRINTF_USE_PRECISION_FORMAT_SPECIFIERS == 1
      out_spec->prec_opt = NPF_FMT_SPEC_OPT_NONE;
#endif
      break;

    case 's': out_spec->conv_spec = NPF_FMT_SPEC_CONV_STRING;
#if NANOPRINTF_USE_FIELD_WIDTH_FORMAT_SPECIFIERS == 1
      out_spec->leading_zero_pad = 0;
#endif
      break;

    case 'i':
    case 'd': tmp_conv = NPF_FMT_SPEC_CONV_SIGNED_INT;
    case 'o':
      if (tmp_conv == NPF_FMT_SPEC_CONV_NONE) { tmp_conv = NPF_FMT_SPEC_CONV_OCTAL; }
    case 'u':
      if (tmp_conv == NPF_FMT_SPEC_CONV_NONE) { tmp_conv = NPF_FMT_SPEC_CONV_UNSIGNED_INT; }
    case 'X':
      if (tmp_conv == NPF_FMT_SPEC_CONV_NONE) { out_spec->case_adjust = 0; }
    case 'x':
      if (tmp_conv == NPF_FMT_SPEC_CONV_NONE) { tmp_conv = NPF_FMT_SPEC_CONV_HEX_INT; }
      out_spec->conv_spec = (uint8_t)tmp_conv;
#if (NANOPRINTF_USE_FIELD_WIDTH_FORMAT_SPECIFIERS == 1) && \
    (NANOPRINTF_USE_PRECISION_FORMAT_SPECIFIERS == 1)
      if (out_spec->prec_opt != NPF_FMT_SPEC_OPT_NONE) { out_spec->leading_zero_pad = 0; }
#endif
      break;

#if NANOPRINTF_USE_FLOAT_FORMAT_SPECIFIERS == 1
    case 'F': out_spec->case_adjust = 0;
    case 'f':
    case_f:
#if NANOPRINTF_USE_FLOAT_DEC_FORMAT_SPECIFIER == 1
      out_spec->conv_spec = NPF_FMT_SPEC_CONV_FLOAT_DEC;
#elif NANOPRINTF_USE_FLOAT_SHORTEST_FORMAT_SPECIFIER == 1
      goto case_g;
#elif NANOPRINTF_USE_FLOAT_SCI_FORMAT_SPECIFIER == 1
      goto case_e;
#elif NANOPRINTF_USE_FLOAT_HEX_FORMAT_SPECIFIER == 1
      goto case_a;
#endif
      break;

    case 'E': out_spec->case_adjust = 0;
    case 'e':
    case_e:
#if NANOPRINTF_USE_FLOAT_SCI_FORMAT_SPECIFIER == 1
      out_spec->conv_spec = NPF_FMT_SPEC_CONV_FLOAT_SCI;
#elif NANOPRINTF_USE_FLOAT_SHORTEST_FORMAT_SPECIFIER == 1
      goto case_g;
#elif NANOPRINTF_USE_FLOAT_DEC_FORMAT_SPECIFIER == 1
      goto case_f;
#elif NANOPRINTF_USE_FLOAT_HEX_FORMAT_SPECIFIER == 1
      goto case_a;
#endif
      break;

    case 'G': out_spec->case_adjust = 0;
    case 'g':
#if NANOPRINTF_USE_FLOAT_SHORTEST_FORMAT_SPECIFIER == 1
      out_spec->conv_spec = NPF_FMT_SPEC_CONV_FLOAT_SHORTEST;
#elif NANOPRINTF_USE_FLOAT_SCI_FORMAT_SPECIFIER == 1
      goto case_g;
#elif NANOPRINTF_USE_FLOAT_DEC_FORMAT_SPECIFIER == 1
      goto case_f;
#elif NANOPRINTF_USE_FLOAT_HEX_FORMAT_SPECIFIER == 1
      goto case_a;
#endif
      break;

    case 'A': out_spec->case_adjust = 0;
    case 'a':
    case_a:
#if NANOPRINTF_USE_FLOAT_HEX_FORMAT_SPECIFIER == 1
      out_spec->conv_spec = NPF_FMT_SPEC_CONV_FLOAT_HEX;
#elif NANOPRINTF_USE_FLOAT_SCI_FORMAT_SPECIFIER == 1
      goto case_e;
#elif NANOPRINTF_USE_FLOAT_SHORTEST_FORMAT_SPECIFIER == 1
      goto case_g;
#elif NANOPRINTF_USE_FLOAT_DEC_FORMAT_SPECIFIER == 1
      goto case_f;
#endif
      break;
#endif

#if NANOPRINTF_USE_WRITEBACK_FORMAT_SPECIFIERS == 1
    case 'n':
      out_spec->conv_spec = NPF_FMT_SPEC_CONV_WRITEBACK;
      break;
#endif

    case 'p':
      out_spec->conv_spec = NPF_FMT_SPEC_CONV_POINTER;
      break;

#if NANOPRINTF_USE_BINARY_FORMAT_SPECIFIERS == 1
    case 'B': out_spec->case_adjust = 0;
    case 'b':
      out_spec->conv_spec = NPF_FMT_SPEC_CONV_BINARY;
      break;
#endif

    default: return 0;
  }

  return (int)(cur - format);
}

static NPF_NOINLINE int npf_utoa_rev(
    npf_uint_t val, char *buf, uint_fast8_t base, char case_adj) {
  uint_fast8_t n = 0;
  do {
    int_fast8_t const d = (int_fast8_t)(val % base);
    *buf++ = (char)(((d < 10) ? '0' : ('A' - 10 + case_adj)) + d);
    ++n;
    val /= base;
  } while (val);
  return (int)n;
}

#if NANOPRINTF_USE_FLOAT_FORMAT_SPECIFIERS == 1
/* Generally, floating-point conversion implementations use
   grisu2 (https://bit.ly/2JgMggX) and ryu (https://bit.ly/2RLXSg0) algorithms,
   which are mathematically exact and fast, but require large lookup tables.

   This implementation was inspired by Wojciech Muła's (zdjęcia@garnek.pl)
   algorithm (http://0x80.pl/notesen/2015-12-29-float-to-string.html) and
   extended further by adding dynamic scaling and configurable integer width by
   Oskars Rubenis (https://github.com/Okarss). */

static NPF_FORCE_INLINE npf_double_bin_t npf_double_to_int_rep(double f) {
  // Union-cast is UB pre-C11 and in all C++; the compiler optimizes the code below.
  npf_double_bin_t bin;
  char const *src = (char const *)&f;
  char *dst = (char *)&bin;
  for (uint_fast8_t i = 0; i < sizeof(f); ++i) { dst[i] = src[i]; }
  return bin;
}

static int npf_ftoa_rev(char *buf, npf_format_spec_t const *spec, double f) {
  char const *ret = NULL;
  npf_double_bin_t bin = npf_double_to_int_rep(f);

  // Unsigned -> signed int casting is IB and can raise a signal but generally doesn't.
  npf_ftoa_exp_t exp =
    (npf_ftoa_exp_t)((npf_ftoa_exp_t)(bin >> NPF_DOUBLE_MAN_BITS) & NPF_DOUBLE_EXP_MASK);

  bin &= ((npf_double_bin_t)0x1 << NPF_DOUBLE_MAN_BITS) - 1;
  if (exp == (npf_ftoa_exp_t)NPF_DOUBLE_EXP_MASK) { // special value
    ret = (bin) ? "NAN" : "FNI";
    goto exit;
  }
  if (spec->prec > (NANOPRINTF_CONVERSION_BUFFER_SIZE - 2)) { goto exit; }
  if (exp) { // normal number
    bin |= (npf_double_bin_t)0x1 << NPF_DOUBLE_MAN_BITS;
  } else { // subnormal number
    ++exp;
  }
  exp = (npf_ftoa_exp_t)(exp - NPF_DOUBLE_EXP_BIAS);

  uint_fast8_t carry; carry = 0;
  npf_ftoa_dec_t end, dec; dec = (npf_ftoa_dec_t)spec->prec;
  if (dec || spec->alt_form) {
    buf[dec++] = '.';
  }

  { // Integer part
    npf_ftoa_man_t man_i;

    if (exp >= 0) {
      int_fast8_t shift_i =
        (int_fast8_t)((exp > NPF_FTOA_SHIFT_BITS) ? (int)NPF_FTOA_SHIFT_BITS : exp);
      npf_ftoa_exp_t exp_i = (npf_ftoa_exp_t)(exp - shift_i);
      shift_i = (int_fast8_t)(NPF_DOUBLE_MAN_BITS - shift_i);
      man_i = (npf_ftoa_man_t)(bin >> shift_i);

      if (exp_i) {
        if (shift_i) {
          carry = (bin >> (shift_i - 1)) & 0x1;
        }
        exp = NPF_DOUBLE_MAN_BITS; // invalidate the fraction part
      }

      // Scale the exponent from base-2 to base-10.
      for (; exp_i; --exp_i) {
        if (!(man_i & ((npf_ftoa_man_t)0x1 << (NPF_FTOA_MAN_BITS - 1)))) {
          man_i = (npf_ftoa_man_t)(man_i << 1);
          man_i = (npf_ftoa_man_t)(man_i | carry); carry = 0;
        } else {
          if (dec >= NANOPRINTF_CONVERSION_BUFFER_SIZE) { goto exit; }
          buf[dec++] = '0';
          carry = (((uint_fast8_t)(man_i % 5) + carry) > 2);
          man_i /= 5;
        }
      }
    } else {
      man_i = 0;
    }
    end = dec;

    do { // Print the integer
      if (end >= NANOPRINTF_CONVERSION_BUFFER_SIZE) { goto exit; }
      buf[end++] = (char)('0' + (char)(man_i % 10));
      man_i /= 10;
    } while (man_i);
  }

  { // Fraction part
    npf_ftoa_man_t man_f;
    npf_ftoa_dec_t dec_f = (npf_ftoa_dec_t)spec->prec;

    if (exp < NPF_DOUBLE_MAN_BITS) {
      int_fast8_t shift_f = (int_fast8_t)((exp < 0) ? -1 : exp);
      npf_ftoa_exp_t exp_f = (npf_ftoa_exp_t)(exp - shift_f);
      npf_double_bin_t bin_f =
        bin << ((NPF_DOUBLE_BIN_BITS - NPF_DOUBLE_MAN_BITS) + shift_f);

      // This if-else statement can be completely optimized at compile time.
      if (NPF_DOUBLE_BIN_BITS > NPF_FTOA_MAN_BITS) {
        man_f = (npf_ftoa_man_t)(bin_f >> ((unsigned)(NPF_DOUBLE_BIN_BITS -
                                                      NPF_FTOA_MAN_BITS) %
                                           NPF_DOUBLE_BIN_BITS));
        carry = (uint_fast8_t)((bin_f >> ((unsigned)(NPF_DOUBLE_BIN_BITS -
                                                     NPF_FTOA_MAN_BITS - 1) %
                                          NPF_DOUBLE_BIN_BITS)) & 0x1);
      } else {
        man_f = (npf_ftoa_man_t)((npf_ftoa_man_t)bin_f
                                 << ((unsigned)(NPF_FTOA_MAN_BITS -
                                                NPF_DOUBLE_BIN_BITS) % NPF_FTOA_MAN_BITS));
        carry = 0;
      }

      // Scale the exponent from base-2 to base-10 and prepare the first digit.
      for (uint_fast8_t digit = 0; dec_f && (exp_f < 4); ++exp_f) {
        if ((man_f > ((npf_ftoa_man_t)-4 / 5)) || digit) {
          carry = (uint_fast8_t)(man_f & 0x1);
          man_f = (npf_ftoa_man_t)(man_f >> 1);
        } else {
          man_f = (npf_ftoa_man_t)(man_f * 5);
          if (carry) { man_f = (npf_ftoa_man_t)(man_f + 3); carry = 0; }
          if (exp_f < 0) {
            buf[--dec_f] = '0';
          } else {
            ++digit;
          }
        }
      }
      man_f = (npf_ftoa_man_t)(man_f + carry);
      carry = (exp_f >= 0);
      dec = 0;
    } else {
      man_f = 0;
    }

    if (dec_f) {
      // Print the fraction
      for (;;) {
        buf[--dec_f] = (char)('0' + (char)(man_f >> (NPF_FTOA_MAN_BITS - 4)));
        man_f = (npf_ftoa_man_t)(man_f & ~((npf_ftoa_man_t)0xF << (NPF_FTOA_MAN_BITS - 4)));
        if (!dec_f) { break; }
        man_f = (npf_ftoa_man_t)(man_f * 10);
      }
      man_f = (npf_ftoa_man_t)(man_f << 4);
    }
    if (exp < NPF_DOUBLE_MAN_BITS) {
      carry &= (uint_fast8_t)(man_f >> (NPF_FTOA_MAN_BITS - 1));
    }
  }

  // Round the number
  for (; carry; ++dec) {
    if (dec >= NANOPRINTF_CONVERSION_BUFFER_SIZE) { goto exit; }
    if (dec >= end) { buf[end++] = '0'; }
    if (buf[dec] == '.') { continue; }
    carry = (buf[dec] == '9');
    buf[dec] = (char)(carry ? '0' : (buf[dec] + 1));
  }

  return (int)end;
exit:
  if (!ret) { ret = "RRE"; }
  uint_fast8_t i;
  for (i = 0; ret[i]; ++i) { buf[i] = (char)(ret[i] + spec->case_adjust); }
  return (int)i;
}

static int npf_atoa_rev(char *buf, npf_format_spec_t const *spec, double f) {
  // We compose the string backwards.
  // We determine how many digits we need (we drop unused bits, and round
  // properly). This may also affect the exponent we use.
  // We can determine the exponent before printing the digits, so we print
  // the exponent immediately (it will be at the start of the reversed buffer).
  // Then we print all the digits (from the units up, as an integer).
  // We add the decimal point (if needed) just before the most significant digit.

  char const *ret = NULL;
  npf_double_bin_t bin = npf_double_to_int_rep(f);

  // Quite silly test, but it documents that we need the mantissa bits, plus the
  // implicit 1, plus at most one bit that might be produced by rounding.
  //static_assert(NPF_DOUBLE_MAN_BITS + 1 + 1 <= NPF_DOUBLE_BIN_BITS);

  // exp is always the binary exponent to be printed (ie the one associated with the
  // most significant digit, not with the most significant bit -- ie the one associated
  // with the leading char of the output string, not with the bit in the "implicit 1" position)

  // Unsigned -> signed int casting is IB and can raise a signal but generally doesn't.
  npf_ftoa_exp_t exp =
    (npf_ftoa_exp_t)((npf_ftoa_exp_t)(bin >> NPF_DOUBLE_MAN_BITS) & NPF_DOUBLE_EXP_MASK);

  bin &= ((npf_double_bin_t)0x1 << NPF_DOUBLE_MAN_BITS) - 1;
  if (exp == (npf_ftoa_exp_t)NPF_DOUBLE_EXP_MASK) { // special value
    ret = (bin) ? "NAN" : "FNI";
    goto exit;
  }
  {
    // We need room for at least "p+0" and the leading digit (not part of precision)
    // TODO: isn't it better to omit this check? it's not the common case, we don't care if "err" takes a long time to be produced
    if (spec->prec > (NANOPRINTF_CONVERSION_BUFFER_SIZE - 4)) { goto exit; }

    if (exp) { // normal number
      bin |= (npf_double_bin_t)0x1 << NPF_DOUBLE_MAN_BITS;
    } else { // subnormal number
      if(bin != 0) {
        ++exp;
      } else {
        exp = NPF_DOUBLE_EXP_BIAS; // 0.0 must get an exponent of 0 (+bias - bias)
      }
    }
    exp = (npf_ftoa_exp_t)(exp - NPF_DOUBLE_EXP_BIAS);

    const int bin_n_dig = (NPF_DOUBLE_MAN_BITS + 1 + 3) / 4; // tot digits -- must count the implicit 1
    int n_dig = n_dig = spec->prec + 1;
    const int n_dig_to_remove = bin_n_dig - NPF_MIN(bin_n_dig, n_dig);
    if (n_dig_to_remove) {
      if(1) { // TODO: rounding is IB; choose whether to skip it and save some code (but the behavior becomes "surprising")
        // Remove the unwanted digits. Shift all of them out, but stop one bit
        // earlier so that we can get that bit as the carry. The missing shift-by-1
        // is done later
        bin = bin >> (4 * n_dig_to_remove - 1);
        const int carry = (bin & 0x1);
        // The carry can cause a new high bit to appear. We don't care, since
        // the high nibble was 0x1 (or 0x0 for subnormal numbers), so now it can't grow
        // larger than 0x2 (or 0x1), so it can be handled by the normal nibble-to-hexdigit logic.
        bin >>= 1;
        bin += carry;
      } else {
        // note: shifting by >= the left-hand variable bitsize is UB; but this
        // shift is well-defined -- we can never never remove the leading
        // significant digit, so we never shift out all the bits, so
        // the shift amount is always less than the bitsize of the variable.
        bin = bin >> (4 * n_dig_to_remove);
      }
    }

    // print the exponent
    int end = -1;
    int neg_exp = 0;
    if (exp < 0) {
      neg_exp = 1;
      exp = -exp;
    }
    do {
      buf[++end] = (char)(exp % 10) + (char)'0';
      exp /= 10;
    } while(exp);
    buf[++end] = neg_exp ? '-' : '+';
    buf[++end] = 'P' + spec->case_adjust;
    // Check if the string fits in the buffer.
    // We always count the decimal point: if it's removed, we have no fractional
    // digits, so the test cannot be wrong just because of this mis-accounting.
    // count: exponent (end +1) + fractional+integral (n_dig) + dot (1)
    if (end + 1 + n_dig + 1 > NANOPRINTF_CONVERSION_BUFFER_SIZE) {
      goto exit;
    }
    for(; n_dig > bin_n_dig; --n_dig) { // pad with trailing zeros (for precision)
      buf[++end] = '0';
    }
    for (int i = n_dig - 1; i > 0; --i) { // print fractional digits
      uint_fast8_t d = bin & 0xF;
      buf[++end] = (char)(((d < 10) ? '0' : ('A' - 10 + spec->case_adjust)) + d);
      bin >>= 4;
    }
    if (n_dig > 1 || spec->alt_form) {
      buf[++end] = '.';
    }
    buf[++end] = (char)(bin & 0xF) + (char)'0';
    // the sign and the '0x' prefix are handled outside

    return end + 1;
  }
exit:
  if (!ret) { ret = "RRE"; }
  uint_fast8_t i;
  for (i = 0; ret[i]; ++i) { buf[i] = (char)(ret[i] + spec->case_adjust); }
  return -(int)i;
}

#endif // NANOPRINTF_USE_FLOAT_FORMAT_SPECIFIERS

#if NANOPRINTF_USE_BINARY_FORMAT_SPECIFIERS == 1
static int npf_bin_len(npf_uint_t u) {
  // Return the length of the binary string format of 'u', preferring intrinsics.
  if (!u) { return 1; }

#ifdef _MSC_VER // Win64, use _BSR64 for everything. If x86, use _BSR when non-large.
  #ifdef _M_X64
    #define NPF_HAVE_BUILTIN_CLZ
    #define NPF_CLZ _BitScanReverse64
  #elif NANOPRINTF_USE_LARGE_FORMAT_SPECIFIERS == 0
    #define NPF_HAVE_BUILTIN_CLZ
    #define NPF_CLZ _BitScanReverse
  #endif
  #ifdef NPF_HAVE_BUILTIN_CLZ
    unsigned long idx;
    NPF_CLZ(&idx, u);
    return (int)(idx + 1);
  #endif
#elif NANOPRINTF_CLANG || NANOPRINTF_GCC_PAST_4_6
  #define NPF_HAVE_BUILTIN_CLZ
  #if NANOPRINTF_USE_LARGE_FORMAT_SPECIFIERS == 1
    #define NPF_CLZ(X) ((sizeof(long long) * CHAR_BIT) - (size_t)__builtin_clzll(X))
  #else
    #define NPF_CLZ(X) ((sizeof(long) * CHAR_BIT) - (size_t)__builtin_clzl(X))
  #endif
  return (int)NPF_CLZ(u);
#endif

#ifndef NPF_HAVE_BUILTIN_CLZ
  int n;
  for (n = 0; u; ++n, u >>= 1); // slow but small software fallback
  return n;
#else
  #undef NPF_HAVE_BUILTIN_CLZ
  #undef NPF_CLZ
#endif
}
#endif

static void npf_bufputc(int c, void *ctx) {
  npf_bufputc_ctx_t *bpc = (npf_bufputc_ctx_t *)ctx;
  if (bpc->cur < bpc->len) { bpc->dst[bpc->cur++] = (char)c; }
}

static void npf_bufputc_nop(int c, void *ctx) { (void)c; (void)ctx; }

typedef struct npf_cnt_putc_ctx {
  npf_putc pc;
  void *ctx;
  int n;
} npf_cnt_putc_ctx_t;

static void npf_putc_cnt(int c, void *ctx) {
  npf_cnt_putc_ctx_t *pc_cnt = (npf_cnt_putc_ctx_t *)ctx;
  ++pc_cnt->n;
  pc_cnt->pc(c, pc_cnt->ctx); // sibling-call optimization
}

#define NPF_PUTC(VAL) do { npf_putc_cnt((int)(VAL), &pc_cnt); } while (0)

#define NPF_EXTRACT(MOD, CAST_TO, EXTRACT_AS) \
  case NPF_FMT_SPEC_LEN_MOD_##MOD: val = (CAST_TO)va_arg(args, EXTRACT_AS); break

#define NPF_WRITEBACK(MOD, TYPE) \
  case NPF_FMT_SPEC_LEN_MOD_##MOD: *(va_arg(args, TYPE *)) = (TYPE)pc_cnt.n; break

int npf_vpprintf(npf_putc pc, void *pc_ctx, char const *format, va_list args) {
  npf_format_spec_t fs;
  char const *cur = format;
  npf_cnt_putc_ctx_t pc_cnt;
  pc_cnt.pc = pc;
  pc_cnt.ctx = pc_ctx;
  pc_cnt.n = 0;

  while (*cur) {
    int const fs_len = (*cur != '%') ? 0 : npf_parse_format_spec(cur, &fs);
    if (!fs_len) { NPF_PUTC(*cur++); continue; }
    cur += fs_len;

    // Extract star-args immediately
#if NANOPRINTF_USE_FIELD_WIDTH_FORMAT_SPECIFIERS == 1
    if (fs.field_width_opt == NPF_FMT_SPEC_OPT_STAR) {
      fs.field_width = va_arg(args, int);
      if (fs.field_width < 0) {
        fs.field_width = -fs.field_width;
        fs.left_justified = 1;
      }
    }
#endif
#if NANOPRINTF_USE_PRECISION_FORMAT_SPECIFIERS == 1
    if (fs.prec_opt == NPF_FMT_SPEC_OPT_STAR) {
      fs.prec = va_arg(args, int);
      if (fs.prec < 0) { fs.prec = 0; fs.prec_opt = NPF_FMT_SPEC_OPT_NONE; }
    }
#endif

    union { char cbuf_mem[NANOPRINTF_CONVERSION_BUFFER_SIZE]; npf_uint_t binval; } u;
    char *cbuf = u.cbuf_mem, sign_c = 0;
    int cbuf_len = 0, need_0x = 0;
#if NANOPRINTF_USE_FIELD_WIDTH_FORMAT_SPECIFIERS == 1
    int field_pad = 0;
    char pad_c = 0;
#endif
#if NANOPRINTF_USE_PRECISION_FORMAT_SPECIFIERS == 1
    int prec_pad = 0;
#if NANOPRINTF_USE_FIELD_WIDTH_FORMAT_SPECIFIERS == 1
    int zero = 0;
#endif
#endif

#if NANOPRINTF_USE_PRECISION_FORMAT_SPECIFIERS == 1
    // Set default precision (we can do that only now that we have extracted the
    // argument-provided precision (".*"), and know whether to ignore that or not
    if (fs.prec_opt == NPF_FMT_SPEC_OPT_NONE) {
      switch(fs.conv_spec) {
      #if NANOPRINTF_USE_FLOAT_DEC_FORMAT_SPECIFIER == 1
        case NPF_FMT_SPEC_CONV_FLOAT_DEC:
      #endif
      #if NANOPRINTF_USE_FLOAT_SCI_FORMAT_SPECIFIER == 1
        case NPF_FMT_SPEC_CONV_FLOAT_SCI:
      #endif
      #if NANOPRINTF_USE_FLOAT_SHORTEST_FORMAT_SPECIFIER == 1
        case NPF_FMT_SPEC_CONV_FLOAT_SHORTEST:
      #endif
          fs.prec = 6;
          break;
      #if NANOPRINTF_USE_FLOAT_HEX_FORMAT_SPECIFIER == 1
        case NPF_FMT_SPEC_CONV_FLOAT_HEX:
          // If the precision is missing, "then the precision is sufficient for an
          // exact representation of the value". We can just pick the max
          // precision ever needed for any value.
          // The format we use for %a is to have the implicit leading 1 in its
          // own hex digit, and mantissa_bits in the fractional digits.
          // The precision doesn't count the leading integral digit.
          // So: prec = ceil(fractional_mantissa_bits / 4.0)
          fs.prec = (NPF_DOUBLE_MAN_BITS + 3) / 4;
          break;
      #endif
        default: // keep 0 as the default
          break;
      }
    }
#endif

    // Extract and convert the argument to string, point cbuf at the text.
    switch (fs.conv_spec) {
      case NPF_FMT_SPEC_CONV_PERCENT:
        *cbuf = '%';
        cbuf_len = 1;
        break;

      case NPF_FMT_SPEC_CONV_CHAR:
        *cbuf = (char)va_arg(args, int);
        cbuf_len = 1;
        break;

      case NPF_FMT_SPEC_CONV_STRING: {
        cbuf = va_arg(args, char *);
#if NANOPRINTF_USE_PRECISION_FORMAT_SPECIFIERS == 1
        for (char const *s = cbuf;
             ((fs.prec_opt == NPF_FMT_SPEC_OPT_NONE) || (cbuf_len < fs.prec)) && cbuf && *s;
             ++s, ++cbuf_len);
#else
        for (char const *s = cbuf; cbuf && *s; ++s, ++cbuf_len); // strlen
#endif
      } break;

      case NPF_FMT_SPEC_CONV_SIGNED_INT: {
        npf_int_t val = 0;
        switch (fs.length_modifier) {
          NPF_EXTRACT(NONE, int, int);
          NPF_EXTRACT(SHORT, short, int);
          NPF_EXTRACT(LONG_DOUBLE, int, int);
          NPF_EXTRACT(CHAR, signed char, int);
          NPF_EXTRACT(LONG, long, long);
#if NANOPRINTF_USE_LARGE_FORMAT_SPECIFIERS == 1
          NPF_EXTRACT(LARGE_LONG_LONG, long long, long long);
          NPF_EXTRACT(LARGE_INTMAX, intmax_t, intmax_t);
          NPF_EXTRACT(LARGE_SIZET, npf_ssize_t, npf_ssize_t);
          NPF_EXTRACT(LARGE_PTRDIFFT, ptrdiff_t, ptrdiff_t);
#endif
          default: break;
        }

        sign_c = (val < 0) ? '-' : fs.prepend;

#if NANOPRINTF_USE_PRECISION_FORMAT_SPECIFIERS == 1
#if NANOPRINTF_USE_FIELD_WIDTH_FORMAT_SPECIFIERS == 1
        zero = !val;
#endif
        // special case, if prec and value are 0, skip
        if (!val && (fs.prec_opt != NPF_FMT_SPEC_OPT_NONE) && !fs.prec) {
          cbuf_len = 0;
        } else
#endif
        {
          npf_uint_t uval = (npf_uint_t)val;
          if (val < 0) { uval = 0 - uval; }
          cbuf_len = npf_utoa_rev(uval, cbuf, 10, fs.case_adjust);
        }
      } break;

#if NANOPRINTF_USE_BINARY_FORMAT_SPECIFIERS == 1
      case NPF_FMT_SPEC_CONV_BINARY:
#endif
      case NPF_FMT_SPEC_CONV_OCTAL:
      case NPF_FMT_SPEC_CONV_HEX_INT:
      case NPF_FMT_SPEC_CONV_UNSIGNED_INT: {
        npf_uint_t val = 0;

        switch (fs.length_modifier) {
          NPF_EXTRACT(NONE, unsigned, unsigned);
          NPF_EXTRACT(SHORT, unsigned short, unsigned);
          NPF_EXTRACT(LONG_DOUBLE, unsigned, unsigned);
          NPF_EXTRACT(CHAR, unsigned char, unsigned);
          NPF_EXTRACT(LONG, unsigned long, unsigned long);
#if NANOPRINTF_USE_LARGE_FORMAT_SPECIFIERS == 1
          NPF_EXTRACT(LARGE_LONG_LONG, unsigned long long, unsigned long long);
          NPF_EXTRACT(LARGE_INTMAX, uintmax_t, uintmax_t);
          NPF_EXTRACT(LARGE_SIZET, size_t, size_t);
          NPF_EXTRACT(LARGE_PTRDIFFT, size_t, size_t);
#endif
          default: break;
        }

#if NANOPRINTF_USE_PRECISION_FORMAT_SPECIFIERS == 1
#if NANOPRINTF_USE_FIELD_WIDTH_FORMAT_SPECIFIERS == 1
        zero = !val;
#endif
        if (!val && (fs.prec_opt != NPF_FMT_SPEC_OPT_NONE) && !fs.prec) {
          // Zero value and explicitly-requested zero precision means "print nothing".
          if ((fs.conv_spec == NPF_FMT_SPEC_CONV_OCTAL) && fs.alt_form) {
            fs.prec = 1; // octal special case, print a single '0'
          }
        } else
#endif
#if NANOPRINTF_USE_BINARY_FORMAT_SPECIFIERS == 1
        if (fs.conv_spec == NPF_FMT_SPEC_CONV_BINARY) {
          cbuf_len = npf_bin_len(val); u.binval = val;
        } else
#endif
        {
          uint_fast8_t const base = (fs.conv_spec == NPF_FMT_SPEC_CONV_OCTAL) ?
            8u : ((fs.conv_spec == NPF_FMT_SPEC_CONV_HEX_INT) ? 16u : 10u);
          cbuf_len = npf_utoa_rev(val, cbuf, base, fs.case_adjust);
        }

        if (val && fs.alt_form && (fs.conv_spec == NPF_FMT_SPEC_CONV_OCTAL)) {
          cbuf[cbuf_len++] = '0'; // OK to add leading octal '0' immediately.
        }

        if (val && fs.alt_form) { // 0x or 0b but can't write it yet.
          if (fs.conv_spec == NPF_FMT_SPEC_CONV_HEX_INT) { need_0x = 'X'; }
#if NANOPRINTF_USE_BINARY_FORMAT_SPECIFIERS == 1
          else if (fs.conv_spec == NPF_FMT_SPEC_CONV_BINARY) { need_0x = 'B'; }
#endif
          if (need_0x) { need_0x += fs.case_adjust; }
        }
      } break;

      case NPF_FMT_SPEC_CONV_POINTER: {
        cbuf_len =
          npf_utoa_rev((npf_uint_t)(uintptr_t)va_arg(args, void *), cbuf, 16, 'a' - 'A');
        need_0x = 'x';
      } break;

#if NANOPRINTF_USE_WRITEBACK_FORMAT_SPECIFIERS == 1
      case NPF_FMT_SPEC_CONV_WRITEBACK:
        switch (fs.length_modifier) {
          NPF_WRITEBACK(NONE, int);
          NPF_WRITEBACK(SHORT, short);
          NPF_WRITEBACK(LONG, long);
          NPF_WRITEBACK(LONG_DOUBLE, double);
          NPF_WRITEBACK(CHAR, signed char);
#if NANOPRINTF_USE_LARGE_FORMAT_SPECIFIERS == 1
          NPF_WRITEBACK(LARGE_LONG_LONG, long long);
          NPF_WRITEBACK(LARGE_INTMAX, intmax_t);
          NPF_WRITEBACK(LARGE_SIZET, size_t);
          NPF_WRITEBACK(LARGE_PTRDIFFT, ptrdiff_t);
#endif
          default: break;
        } break;
#endif

#if NANOPRINTF_USE_FLOAT_FORMAT_SPECIFIERS == 1
  #if NANOPRINTF_USE_FLOAT_DEC_FORMAT_SPECIFIER
      case NPF_FMT_SPEC_CONV_FLOAT_DEC:
  #endif
  #if NANOPRINTF_USE_FLOAT_SCI_FORMAT_SPECIFIER
      case NPF_FMT_SPEC_CONV_FLOAT_SCI:
  #endif
  #if NANOPRINTF_USE_FLOAT_SHORTEST_FORMAT_SPECIFIER
      case NPF_FMT_SPEC_CONV_FLOAT_SHORTEST:
  #endif
  #if NANOPRINTF_USE_FLOAT_HEX_FORMAT_SPECIFIER
      case NPF_FMT_SPEC_CONV_FLOAT_HEX:
  #endif
      {
        double val;
        if (fs.length_modifier == NPF_FMT_SPEC_LEN_MOD_LONG_DOUBLE) {
          val = (double)va_arg(args, long double);
        } else {
          val = va_arg(args, double);
        }

        sign_c = (npf_double_to_int_rep(val) >> NPF_DOUBLE_SIGN_POS) ? '-' : fs.prepend;
#if NANOPRINTF_USE_FIELD_WIDTH_FORMAT_SPECIFIERS == 1
        zero = (val == 0.);
#endif

#if NANOPRINTF_USE_FLOAT_HEX_FORMAT_SPECIFIER == 1
        if(fs.conv_spec == NPF_FMT_SPEC_CONV_FLOAT_HEX) {
          cbuf_len = npf_atoa_rev(cbuf, &fs, val);
          if(cbuf_len > 0) {
            need_0x = 'X' + fs.case_adjust;
          }
          goto end_float_toa;
        }
#endif
#if NANOPRINTF_USE_FLOAT_DEC_FORMAT_SPECIFIER == 1
        if(fs.conv_spec == NPF_FMT_SPEC_CONV_FLOAT_DEC) {
          cbuf_len = npf_ftoa_rev(cbuf, &fs, val);
          goto end_float_toa;
        }
#endif
      end_float_toa: ;
        if (cbuf_len < 0) { // negative means text (not number), so ignore the '0' flag
           cbuf_len = -cbuf_len;
#if NANOPRINTF_USE_FIELD_WIDTH_FORMAT_SPECIFIERS == 1
           fs.leading_zero_pad = 0;
#endif
        }
      } break;
#endif
      default: break;
    }

#if NANOPRINTF_USE_FIELD_WIDTH_FORMAT_SPECIFIERS == 1
    // Compute the field width pad character
    if (fs.field_width_opt != NPF_FMT_SPEC_OPT_NONE) {
      // '0' flag is only legal with numeric types
      // '-' flag overrides '0' flag
      if (fs.leading_zero_pad && !fs.left_justified) {
        if ((fs.conv_spec != NPF_FMT_SPEC_CONV_STRING) &&
            (fs.conv_spec != NPF_FMT_SPEC_CONV_CHAR) &&
            (fs.conv_spec != NPF_FMT_SPEC_CONV_PERCENT)) {
#if NANOPRINTF_USE_PRECISION_FORMAT_SPECIFIERS == 1
          if ((fs.prec_opt != NPF_FMT_SPEC_OPT_NONE) && !fs.prec && zero) {
            pad_c = ' ';
          } else
#endif
          { pad_c = '0'; }
        }
      } else { pad_c = ' '; }
    }
#endif

    // Compute the number of bytes to truncate or '0'-pad.
#if NANOPRINTF_USE_PRECISION_FORMAT_SPECIFIERS == 1
    if (fs.conv_spec != NPF_FMT_SPEC_CONV_STRING
#if NANOPRINTF_USE_FLOAT_DEC_FORMAT_SPECIFIER == 1
        && (fs.conv_spec != NPF_FMT_SPEC_CONV_FLOAT_DEC)
#endif
#if NANOPRINTF_USE_FLOAT_SCI_FORMAT_SPECIFIER == 1
        && (fs.conv_spec != NPF_FMT_SPEC_CONV_FLOAT_SCI)
#endif
#if NANOPRINTF_USE_FLOAT_SHORTEST_FORMAT_SPECIFIER == 1
        && (fs.conv_spec != NPF_FMT_SPEC_CONV_FLOAT_SHORTEST)
#endif
#if NANOPRINTF_USE_FLOAT_HEX_FORMAT_SPECIFIER == 1
        && (fs.conv_spec != NPF_FMT_SPEC_CONV_FLOAT_HEX)
#endif
    ) {
      prec_pad = npf_max(0, fs.prec - cbuf_len);
    }
#endif

#if NANOPRINTF_USE_FIELD_WIDTH_FORMAT_SPECIFIERS == 1
    // Given the full converted length, how many pad bytes?
    field_pad = fs.field_width - cbuf_len - !!sign_c;
    if (need_0x) { field_pad -= 2; }
#if NANOPRINTF_USE_PRECISION_FORMAT_SPECIFIERS == 1
    field_pad -= prec_pad;
#endif
    field_pad = npf_max(0, field_pad);

    // Apply right-justified field width if requested
    if (!fs.left_justified && pad_c) { // If leading zeros pad, sign goes first.
      if (pad_c == '0') {
        if (sign_c) { NPF_PUTC(sign_c); sign_c = 0; }
        // Pad byte is '0', write '0x' before '0' pad chars.
        if (need_0x) { NPF_PUTC('0'); NPF_PUTC(need_0x); }
      }
      while (field_pad-- > 0) { NPF_PUTC(pad_c); }
      // Pad byte is ' ', write '0x' after ' ' pad chars but before number.
      if ((pad_c != '0') && need_0x) {
#if NANOPRINTF_USE_FLOAT_FORMAT_SPECIFIERS == 1 && NANOPRINTF_USE_FLOAT_HEX_FORMAT_SPECIFIER == 1
        if (sign_c) { NPF_PUTC(sign_c); sign_c = 0; }
#endif
        NPF_PUTC('0'); NPF_PUTC(need_0x);
      }
    } else
#endif
    { // no pad, '0x' requested.
#if NANOPRINTF_USE_FLOAT_FORMAT_SPECIFIERS == 1 && NANOPRINTF_USE_FLOAT_HEX_FORMAT_SPECIFIER == 1
      if (sign_c) { NPF_PUTC(sign_c); sign_c = 0; }
#endif
      if (need_0x) { NPF_PUTC('0'); NPF_PUTC(need_0x); }
    }

    // Write the converted payload
    if (fs.conv_spec == NPF_FMT_SPEC_CONV_STRING) {
      for (int i = 0; cbuf && (i < cbuf_len); ++i) { NPF_PUTC(cbuf[i]); }
    } else {
      if (sign_c) { NPF_PUTC(sign_c); }
#if NANOPRINTF_USE_PRECISION_FORMAT_SPECIFIERS == 1
      while (prec_pad-- > 0) { NPF_PUTC('0'); } // int precision leads.
#endif
#if NANOPRINTF_USE_BINARY_FORMAT_SPECIFIERS == 1
      if (fs.conv_spec == NPF_FMT_SPEC_CONV_BINARY) {
        while (cbuf_len) { NPF_PUTC('0' + ((u.binval >> --cbuf_len) & 1)); }
      } else
#endif
      { while (cbuf_len-- > 0) { NPF_PUTC(cbuf[cbuf_len]); } } // payload is reversed
    }

#if NANOPRINTF_USE_FIELD_WIDTH_FORMAT_SPECIFIERS == 1
    if (fs.left_justified && pad_c) { // Apply left-justified field width
      while (field_pad-- > 0) { NPF_PUTC(pad_c); }
    }
#endif
  }

  return pc_cnt.n;
}

#undef NPF_PUTC
#undef NPF_EXTRACT
#undef NPF_WRITEBACK

int npf_pprintf(npf_putc pc, void *pc_ctx, char const *format, ...) {
  va_list val;
  va_start(val, format);
  int const rv = npf_vpprintf(pc, pc_ctx, format, val);
  va_end(val);
  return rv;
}

int npf_snprintf(char *buffer, size_t bufsz, const char *format, ...) {
  va_list val;
  va_start(val, format);
  int const rv = npf_vsnprintf(buffer, bufsz, format, val);
  va_end(val);
  return rv;
}

int npf_vsnprintf(char *buffer, size_t bufsz, char const *format, va_list vlist) {
  npf_bufputc_ctx_t bufputc_ctx;
  bufputc_ctx.dst = buffer;
  bufputc_ctx.len = bufsz;
  bufputc_ctx.cur = 0;

  npf_putc const pc = buffer ? npf_bufputc : npf_bufputc_nop;
  int const n = npf_vpprintf(pc, &bufputc_ctx, format, vlist);
  pc('\0', &bufputc_ctx);

  if (buffer && bufsz) {
#ifdef NANOPRINTF_SNPRINTF_SAFE_EMPTY_STRING_ON_OVERFLOW
    if (n >= (int)bufsz) { buffer[0] = '\0'; }
#else
    buffer[bufsz - 1] = '\0';
#endif
  }

  return n;
}

#if NANOPRINTF_HAVE_GCC_WARNING_PRAGMAS
  #pragma GCC diagnostic pop
#endif

#ifdef _MSC_VER
  #pragma warning(pop)
#endif

#endif // NANOPRINTF_IMPLEMENTATION_INCLUDED
#endif // NANOPRINTF_IMPLEMENTATION

/*
  nanoprintf is dual-licensed under both the "Unlicense" and the
  "Zero-Clause BSD" (0BSD) licenses. The intent of this dual-licensing
  structure is to make nanoprintf as consumable as possible in as many
  environments / countries / companies as possible without any
  encumberances.

  The text of the two licenses follows below:

  ============================== UNLICENSE ==============================

  This is free and unencumbered software released into the public domain.

  Anyone is free to copy, modify, publish, use, compile, sell, or
  distribute this software, either in source code form or as a compiled
  binary, for any purpose, commercial or non-commercial, and by any
  means.

  In jurisdictions that recognize copyright laws, the author or authors
  of this software dedicate any and all copyright interest in the
  software to the public domain. We make this dedication for the benefit
  of the public at large and to the detriment of our heirs and
  successors. We intend this dedication to be an overt act of
  relinquishment in perpetuity of all present and future rights to this
  software under copyright law.

  THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
  EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
  MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
  IN NO EVENT SHALL THE AUTHORS BE LIABLE FOR ANY CLAIM, DAMAGES OR
  OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE,
  ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR
  OTHER DEALINGS IN THE SOFTWARE.

  For more information, please refer to <http://unlicense.org>

  ================================ 0BSD =================================

  Copyright (C) 2019- by Charles Nicholson <charles.nicholson+nanoprintf@gmail.com>

  Permission to use, copy, modify, and/or distribute this software for
  any purpose with or without fee is hereby granted.

  THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
  WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
  MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
  ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
  WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
  ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
  OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
*/
