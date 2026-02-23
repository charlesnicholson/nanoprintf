/* nanoprintf v0.5.5: a tiny embeddable printf replacement written in C.
   https://github.com/charlesnicholson/nanoprintf
   charles.nicholson+nanoprintf@gmail.com
   dual-licensed under 0bsd and unlicense, take your pick. see eof for details. */

#ifndef NPF_H_INCLUDED
#define NPF_H_INCLUDED

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
#define NPF_RESTRICT
extern "C" {
#else
#if defined(__STDC_VERSION__) && (__STDC_VERSION__ >= 199901L)
#define NPF_RESTRICT restrict
#else
#define NPF_RESTRICT
#endif
#endif

// The npf_ functions all return the number of bytes required to express the
// fully-formatted string, not including the null terminator character.
// The npf_ functions do not return negative values, since the lack of 'l' length
// modifier support makes encoding errors impossible.

typedef void (*npf_putc)(int c, void *ctx);

#if defined(__clang__)
_Pragma("clang diagnostic push")
_Pragma("clang diagnostic ignored \"-Wgnu-zero-variadic-macro-arguments\"")
#endif

#if defined(NANOPRINTF_FLOAT_SINGLE_PRECISION) && NANOPRINTF_FLOAT_SINGLE_PRECISION == 1
#define NPF_MAP_ARGS(...) NPF__MAP(NPF__WRAP, __VA_ARGS__)
#else
#define NPF_MAP_ARGS(...) __VA_ARGS__
#endif

#if defined(NANOPRINTF_FLOAT_SINGLE_PRECISION) && NANOPRINTF_FLOAT_SINGLE_PRECISION == 1
#define NPF_PRINTF_SP_ATTR NPF_PRINTF_ATTR(3, 0)
#else
#define NPF_PRINTF_SP_ATTR NPF_PRINTF_ATTR(3, 4)
#endif

// ---- public api

#define npf_snprintf(buf, sz, format, ...) \
  npf_snprintf_impl((buf), (sz), NPF_MAP_ARGS((format), ##__VA_ARGS__))

#define npf_pprintf(pc, ctx, format, ...) \
  npf_pprintf_impl((pc), (ctx), NPF_MAP_ARGS((format), ##__VA_ARGS__))

NPF_VISIBILITY int npf_vsnprintf(char * NPF_RESTRICT buffer,
                                 size_t bufsz,
                                 char const * NPF_RESTRICT format,
                                 va_list vlist) NPF_PRINTF_ATTR(3, 0);

NPF_VISIBILITY int npf_vpprintf(npf_putc pc,
                                void * NPF_RESTRICT pc_ctx,
                                char const * NPF_RESTRICT format,
                                va_list vlist) NPF_PRINTF_ATTR(3, 0);

// ---- publicly visible helper functions (called by macro entry points)

NPF_VISIBILITY int npf_snprintf_impl(char * NPF_RESTRICT buffer,
                                      size_t bufsz,
                                      const char * NPF_RESTRICT format, ...)
                                      NPF_PRINTF_SP_ATTR;

NPF_VISIBILITY int npf_pprintf_impl(npf_putc pc,
                                    void * NPF_RESTRICT pc_ctx,
                                    char const * NPF_RESTRICT format, ...)
                                    NPF_PRINTF_SP_ATTR;

#if defined(__clang__)
_Pragma("clang diagnostic pop")
#endif

#ifdef __cplusplus
}
#endif

#endif // NPF_H_INCLUDED

#if defined(NANOPRINTF_FLOAT_SINGLE_PRECISION) && NANOPRINTF_FLOAT_SINGLE_PRECISION == 1
#ifndef NPF_FLOAT_T_INCLUDED
#define NPF_FLOAT_T_INCLUDED
typedef struct { float val; } npf_float_t;
#endif
#endif

/* The implementation of nanoprintf begins here, to be compiled only if
   NANOPRINTF_IMPLEMENTATION is defined. In a multi-file library what follows would
   be nanoprintf.c. */

#ifdef NANOPRINTF_IMPLEMENTATION

#ifndef NPF_IMPLEMENTATION_INCLUDED
#define NPF_IMPLEMENTATION_INCLUDED

#include <limits.h>
#include <stdint.h>

// The conversion buffer must fit at least UINT64_MAX in octal format with the leading '0'.
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
    !defined(NANOPRINTF_USE_SMALL_FORMAT_SPECIFIERS) && \
    !defined(NANOPRINTF_USE_BINARY_FORMAT_SPECIFIERS) && \
    !defined(NANOPRINTF_USE_WRITEBACK_FORMAT_SPECIFIERS) && \
    !defined(NANOPRINTF_USE_ALT_FORM_FLAG) && \
    !defined(NANOPRINTF_FLOAT_SINGLE_PRECISION)
  #define NANOPRINTF_USE_FIELD_WIDTH_FORMAT_SPECIFIERS 1
  #define NANOPRINTF_USE_PRECISION_FORMAT_SPECIFIERS 1
  #define NANOPRINTF_USE_FLOAT_FORMAT_SPECIFIERS 1
  #define NANOPRINTF_USE_LARGE_FORMAT_SPECIFIERS 0
  #define NANOPRINTF_USE_SMALL_FORMAT_SPECIFIERS 1
  #define NANOPRINTF_USE_BINARY_FORMAT_SPECIFIERS 0
  #define NANOPRINTF_USE_WRITEBACK_FORMAT_SPECIFIERS 0
  #define NANOPRINTF_USE_ALT_FORM_FLAG 1
  #define NANOPRINTF_FLOAT_SINGLE_PRECISION 0
#endif

// Single-precision mode defaults to off unless explicitly enabled.
#ifndef NANOPRINTF_FLOAT_SINGLE_PRECISION
  #define NANOPRINTF_FLOAT_SINGLE_PRECISION 0
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
#ifndef NANOPRINTF_USE_SMALL_FORMAT_SPECIFIERS
  #error NANOPRINTF_USE_SMALL_FORMAT_SPECIFIERS must be #defined to 0 or 1
#endif
#ifndef NANOPRINTF_USE_BINARY_FORMAT_SPECIFIERS
  #error NANOPRINTF_USE_BINARY_FORMAT_SPECIFIERS must be #defined to 0 or 1
#endif
#ifndef NANOPRINTF_USE_WRITEBACK_FORMAT_SPECIFIERS
  #error NANOPRINTF_USE_WRITEBACK_FORMAT_SPECIFIERS must be #defined to 0 or 1
#endif

// Ensure flags are compatible.
#if (NANOPRINTF_USE_FLOAT_FORMAT_SPECIFIERS == 1) && \
    (NANOPRINTF_USE_PRECISION_FORMAT_SPECIFIERS == 0)
  #error Precision format specifiers must be enabled if float support is enabled.
#endif

#if (NANOPRINTF_FLOAT_SINGLE_PRECISION == 1) && \
    (NANOPRINTF_USE_FLOAT_FORMAT_SPECIFIERS == 0)
  #error Single precision requires float format specifiers to be enabled.
#endif

#if NANOPRINTF_FLOAT_SINGLE_PRECISION == 1
  #ifdef __cplusplus
    #if __cplusplus < 201103L
      #error Single-precision float wrapping requires C++11 or later.
    #endif
  #else
    #if !(defined(__GNUC__) || defined(__clang__))
      #if !defined(__STDC_VERSION__) || (__STDC_VERSION__ < 201112L)
        #error Single-precision float wrapping requires C11 or later (or GCC/Clang).
      #endif
    #endif
  #endif
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
  #define NPF_CLANG 1
  #define NPF_GCC_PAST_4_6 0
#else
  #define NPF_CLANG 0
  #if defined(__GNUC__) && ((__GNUC__ > 4) || ((__GNUC__ == 4) && (__GNUC_MINOR__ > 6)))
    #define NPF_GCC_PAST_4_6 1
  #else
    #define NPF_GCC_PAST_4_6 0
  #endif
#endif

#if NPF_CLANG || NPF_GCC_PAST_4_6
  #define NPF_HAVE_GCC_WARNING_PRAGMAS 1
#else
  #define NPF_HAVE_GCC_WARNING_PRAGMAS 0
#endif

#if NPF_HAVE_GCC_WARNING_PRAGMAS
  #pragma GCC diagnostic push
  #pragma GCC diagnostic ignored "-Wpragmas"
  #pragma GCC diagnostic ignored "-Wfloat-equal"
  #pragma GCC diagnostic ignored "-Wgnu-statement-expression-from-macro-expansion"
  #pragma GCC diagnostic ignored "-Wimplicit-fallthrough"
  #pragma GCC diagnostic ignored "-Wpadded"
  #pragma GCC diagnostic ignored "-Wpedantic"
  #pragma GCC diagnostic ignored "-Wunused-function"

  #ifdef __cplusplus
    #pragma GCC diagnostic ignored "-Wold-style-cast"
  #endif

  #if NPF_CLANG
    #pragma GCC diagnostic ignored "-Wc++98-compat-pedantic"
    #pragma GCC diagnostic ignored "-Wcovered-switch-default"
    #pragma GCC diagnostic ignored "-Wdeclaration-after-statement"
    #pragma GCC diagnostic ignored "-Wzero-as-null-pointer-constant"
    #ifndef __APPLE__
      #pragma GCC diagnostic ignored "-Wunsafe-buffer-usage"
    #endif
  #elif NPF_GCC_PAST_4_6
    #pragma GCC diagnostic ignored "-Wmaybe-uninitialized"
  #endif
#endif

#ifdef _MSC_VER
  #pragma warning(push)
  #pragma warning(disable:4619) // there is no warning number 'number'
  // C4619 has to be disabled first!
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

  #define NPF_MIN(X, Y) ({ \
    __typeof__(X) const x = (X); __typeof__(Y) const y = (Y); x <= y ? x : y; })
  #define NPF_MAX(X, Y) ({ \
    __typeof__(X) const x = (X); __typeof__(Y) const y = (Y); x >= y ? x : y; })
#else
  #if defined(_MSC_VER)
    #define NPF_NOINLINE __declspec(noinline)
    #define NPF_FORCE_INLINE inline __forceinline
  #else
    #define NPF_NOINLINE
    #define NPF_FORCE_INLINE
  #endif

  #define NPF_MIN(X, Y) ((X) <= (Y) ? (X) : (Y))
  #define NPF_MAX(X, Y) ((X) >= (Y) ? (X) : (Y))
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
#if NANOPRINTF_USE_SMALL_FORMAT_SPECIFIERS == 1
  NPF_FMT_SPEC_LEN_MOD_SHORT,       // 'h'
  NPF_FMT_SPEC_LEN_MOD_CHAR,        // 'hh'
#endif
  NPF_FMT_SPEC_LEN_MOD_LONG,        // 'l'
  NPF_FMT_SPEC_LEN_MOD_LONG_DOUBLE, // 'L'
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
  NPF_FMT_SPEC_CONV_FLOAT_DEC,      // 'f', 'F'
  NPF_FMT_SPEC_CONV_FLOAT_SCI,      // 'e', 'E'
  NPF_FMT_SPEC_CONV_FLOAT_SHORTEST, // 'g', 'G'
  NPF_FMT_SPEC_CONV_FLOAT_HEX,      // 'a', 'A'
#endif
};

typedef struct npf_format_spec {
#if NANOPRINTF_USE_FIELD_WIDTH_FORMAT_SPECIFIERS == 1
  int field_width;
#endif
#if NANOPRINTF_USE_PRECISION_FORMAT_SPECIFIERS == 1
  int prec;
  uint8_t prec_opt;
#endif
#if NANOPRINTF_USE_FIELD_WIDTH_FORMAT_SPECIFIERS == 1
  uint8_t field_width_opt;
  char left_justified;   // '-'
  char leading_zero_pad; // '0'
#endif
  char prepend;          // ' ' or '+'
#if NANOPRINTF_USE_ALT_FORM_FLAG == 1
  char alt_form;         // '#'
#endif
  char case_adjust;      // 'a' - 'A' , or 0 (must be non-negative to work)
  uint8_t length_modifier;
  uint8_t conv_spec;
} npf_format_spec_t;

#if NANOPRINTF_USE_LARGE_FORMAT_SPECIFIERS == 1
  typedef intmax_t npf_int_t;
  typedef uintmax_t npf_uint_t;
#elif ULONG_MAX > UINTPTR_MAX
  typedef long npf_int_t;
  typedef unsigned long npf_uint_t;
#else
  typedef intptr_t npf_int_t;
  typedef uintptr_t npf_uint_t;
#endif

typedef struct npf_bufputc_ctx {
  char *dst;
  size_t len;
  size_t cur;
} npf_bufputc_ctx_t;

#if NANOPRINTF_USE_LARGE_FORMAT_SPECIFIERS == 1
  typedef char npf_size_is_ptrdiff[(sizeof(size_t) == sizeof(ptrdiff_t)) ? 1 : -1];
  typedef ptrdiff_t npf_ssize_t;
  typedef size_t npf_uptrdiff_t;
#endif

#ifdef _MSC_VER
  #include <intrin.h>
#endif

static int npf_parse_format_spec(char const *format, npf_format_spec_t *out_spec) {
  char const *cur = format;

#if NANOPRINTF_USE_FIELD_WIDTH_FORMAT_SPECIFIERS == 1
  out_spec->left_justified = 0;
  out_spec->leading_zero_pad = 0;
#endif
  out_spec->case_adjust = 'a' - 'A'; // lowercase
  out_spec->prepend = 0;
#if NANOPRINTF_USE_ALT_FORM_FLAG == 1
  out_spec->alt_form = 0;
#endif

  while (*++cur) { // cur points at the leading '%' character
    switch (*cur) { // Optional flags
#if NANOPRINTF_USE_FIELD_WIDTH_FORMAT_SPECIFIERS == 1
      case '-': out_spec->left_justified = '-'; continue;
      case '0': out_spec->leading_zero_pad = 1; continue;
#endif
      case '+': out_spec->prepend = '+'; continue;
      case ' ': if (out_spec->prepend == 0) { out_spec->prepend = ' '; } continue;
#if NANOPRINTF_USE_ALT_FORM_FLAG == 1
      case '#': out_spec->alt_form = '#'; continue;
#endif
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
#if NANOPRINTF_USE_SMALL_FORMAT_SPECIFIERS == 1
    case 'h':
      out_spec->length_modifier = NPF_FMT_SPEC_LEN_MOD_SHORT;
      if (*cur == 'h') {
        out_spec->length_modifier = NPF_FMT_SPEC_LEN_MOD_CHAR;
        ++cur;
      }
      break;
#endif
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
      break;

    case 'c': out_spec->conv_spec = NPF_FMT_SPEC_CONV_CHAR;
      break;

    case 's': out_spec->conv_spec = NPF_FMT_SPEC_CONV_STRING;
      break;

    case 'i':
    case 'd': tmp_conv = NPF_FMT_SPEC_CONV_SIGNED_INT; goto finish;
    case 'o': tmp_conv = NPF_FMT_SPEC_CONV_OCTAL; goto finish;
    case 'u': tmp_conv = NPF_FMT_SPEC_CONV_UNSIGNED_INT; goto finish;
    case 'X': out_spec->case_adjust = 0;
    case 'x': tmp_conv = NPF_FMT_SPEC_CONV_HEX_INT; goto finish;
    finish:
      out_spec->conv_spec = (uint8_t)tmp_conv;
      break;

#if NANOPRINTF_USE_FLOAT_FORMAT_SPECIFIERS == 1
    case 'F': out_spec->case_adjust = 0;
    case 'f':
      out_spec->conv_spec = NPF_FMT_SPEC_CONV_FLOAT_DEC;
      break;

    case 'E': out_spec->case_adjust = 0;
    case 'e':
      out_spec->conv_spec = NPF_FMT_SPEC_CONV_FLOAT_SCI;
      break;

    case 'G': out_spec->case_adjust = 0;
    case 'g':
      out_spec->conv_spec = NPF_FMT_SPEC_CONV_FLOAT_SHORTEST;
      break;

    case 'A': out_spec->case_adjust = 0;
    case 'a':
      out_spec->conv_spec = NPF_FMT_SPEC_CONV_FLOAT_HEX;
      break;
#endif

#if NANOPRINTF_USE_WRITEBACK_FORMAT_SPECIFIERS == 1
    case 'n':
      // todo: reject string if flags or width or precision exist
      out_spec->conv_spec = NPF_FMT_SPEC_CONV_WRITEBACK;
      break;
#endif

    case 'p':
      out_spec->conv_spec = NPF_FMT_SPEC_CONV_POINTER;
      break;

#if NANOPRINTF_USE_BINARY_FORMAT_SPECIFIERS == 1
    case 'B':
      out_spec->case_adjust = 0;
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

#include <float.h>

#if NANOPRINTF_FLOAT_SINGLE_PRECISION == 1
  typedef float npf_real_t;
  #define NPF_REAL_MANT_DIG FLT_MANT_DIG
  #define NPF_REAL_MAX_EXP  FLT_MAX_EXP
#else
  typedef double npf_real_t;
  #define NPF_REAL_MANT_DIG DBL_MANT_DIG
  #define NPF_REAL_MAX_EXP  DBL_MAX_EXP
#endif

#if (NPF_REAL_MANT_DIG <= 11) && (NPF_REAL_MAX_EXP <= 16)
  typedef uint_fast16_t npf_real_bin_t;
  typedef int_fast8_t npf_ftoa_exp_t;
#elif (NPF_REAL_MANT_DIG <= 24) && (NPF_REAL_MAX_EXP <= 128)
  typedef uint_fast32_t npf_real_bin_t;
  typedef int_fast8_t npf_ftoa_exp_t;
#elif (NPF_REAL_MANT_DIG <= 53) && (NPF_REAL_MAX_EXP <= 1024)
  typedef uint_fast64_t npf_real_bin_t;
  typedef int_fast16_t npf_ftoa_exp_t;
#else
  #error Unsupported width of the real type.
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
  NPF_REAL_EXP_MASK = NPF_REAL_MAX_EXP * 2 - 1,
  NPF_REAL_EXP_BIAS = NPF_REAL_MAX_EXP - 1,
  NPF_REAL_MAN_BITS = NPF_REAL_MANT_DIG - 1,
  NPF_REAL_BIN_BITS = sizeof(npf_real_bin_t) * CHAR_BIT,
  NPF_REAL_SIGN_POS = sizeof(npf_real_t) * CHAR_BIT - 1,
  NPF_FTOA_MAN_BITS   = sizeof(npf_ftoa_man_t) * CHAR_BIT,
  NPF_FTOA_SHIFT_BITS =
    ((NPF_FTOA_MAN_BITS < NPF_REAL_MANT_DIG) ? NPF_FTOA_MAN_BITS : NPF_REAL_MANT_DIG) - 1
};

/* Generally, floating-point conversion implementations use
   grisu2 (https://bit.ly/2JgMggX) and ryu (https://bit.ly/2RLXSg0) algorithms,
   which are mathematically exact and fast, but require large lookup tables.

   This implementation was inspired by Wojciech Muła's (zdjęcia@garnek.pl)
   algorithm (http://0x80.pl/notesen/2015-12-29-float-to-string.html) and
   extended further by adding dynamic scaling and configurable integer width by
   Oskars Rubenis (https://github.com/Okarss). */

static NPF_FORCE_INLINE npf_real_bin_t npf_real_to_int_rep(npf_real_t f) {
  // Union-cast is UB pre-C11 and in all C++; the compiler optimizes the code below.
  npf_real_bin_t bin;
  char const *src = (char const *)&f;
  char *dst = (char *)&bin;
  for (uint_fast8_t i = 0; i < sizeof(f); ++i) { dst[i] = src[i]; }
  return bin;
}

static int npf_ftoa_rev(char *buf, npf_format_spec_t const *spec, npf_real_t f) {
  char const *ret = NULL;
  npf_real_bin_t bin = npf_real_to_int_rep(f);

  // Unsigned -> signed int casting is IB and can raise a signal but generally doesn't.
  npf_ftoa_exp_t exp =
    (npf_ftoa_exp_t)((npf_ftoa_exp_t)(bin >> NPF_REAL_MAN_BITS) & NPF_REAL_EXP_MASK);

  bin &= ((npf_real_bin_t)0x1 << NPF_REAL_MAN_BITS) - 1;
  if (exp == (npf_ftoa_exp_t)NPF_REAL_EXP_MASK) { // special value
    ret = (bin) ? "NAN" : "FNI";
    goto exit;
  }
  if (spec->prec > (NANOPRINTF_CONVERSION_BUFFER_SIZE - 2)) { goto exit; }
  if (exp) { // normal number
    bin |= (npf_real_bin_t)0x1 << NPF_REAL_MAN_BITS;
  } else { // subnormal number
    ++exp;
  }
  exp = (npf_ftoa_exp_t)(exp - NPF_REAL_EXP_BIAS);

  uint_fast8_t carry; carry = 0;
  npf_ftoa_dec_t end, dec; dec = (npf_ftoa_dec_t)spec->prec;
  if (dec
#if NANOPRINTF_USE_ALT_FORM_FLAG == 1
      || spec->alt_form
#endif
  ) {
    buf[dec++] = '.';
  }

  { // Integer part
    npf_ftoa_man_t man_i;

    if (exp >= 0) {
      int_fast8_t shift_i =
        (int_fast8_t)((exp > NPF_FTOA_SHIFT_BITS) ? (int)NPF_FTOA_SHIFT_BITS : exp);
      npf_ftoa_exp_t exp_i = (npf_ftoa_exp_t)(exp - shift_i);
      shift_i = (int_fast8_t)(NPF_REAL_MAN_BITS - shift_i);
      man_i = (npf_ftoa_man_t)(bin >> shift_i);

      if (exp_i) {
        if (shift_i) {
          carry = (bin >> (shift_i - 1)) & 0x1;
        }
        exp = NPF_REAL_MAN_BITS; // invalidate the fraction part
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

    if (exp < NPF_REAL_MAN_BITS) {
      int_fast8_t shift_f = (int_fast8_t)((exp < 0) ? -1 : exp);
      npf_ftoa_exp_t exp_f = (npf_ftoa_exp_t)(exp - shift_f);
      npf_real_bin_t bin_f =
        bin << ((NPF_REAL_BIN_BITS - NPF_REAL_MAN_BITS) + shift_f);

      // This if-else statement can be completely optimized at compile time.
      if (NPF_REAL_BIN_BITS > NPF_FTOA_MAN_BITS) {
        man_f = (npf_ftoa_man_t)(bin_f >> ((unsigned)(NPF_REAL_BIN_BITS -
                                                      NPF_FTOA_MAN_BITS) %
                                           NPF_REAL_BIN_BITS));
        carry = (uint_fast8_t)((bin_f >> ((unsigned)(NPF_REAL_BIN_BITS -
                                                     NPF_FTOA_MAN_BITS - 1) %
                                          NPF_REAL_BIN_BITS)) & 0x1);
      } else {
        man_f = (npf_ftoa_man_t)((npf_ftoa_man_t)bin_f
                                 << ((unsigned)(NPF_FTOA_MAN_BITS -
                                                NPF_REAL_BIN_BITS) % NPF_FTOA_MAN_BITS));
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
    if (exp < NPF_REAL_MAN_BITS) {
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
#elif NPF_CLANG || NPF_GCC_PAST_4_6
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

#if NANOPRINTF_USE_PRECISION_FORMAT_SPECIFIERS == 1
  // Set default precision (we can do that only now that we have extracted the
  // argument-provided precision (".*"), and know whether to ignore that or not
  #if NANOPRINTF_USE_FLOAT_FORMAT_SPECIFIERS == 1
    if (fs.prec_opt == NPF_FMT_SPEC_OPT_NONE) {
      switch (fs.conv_spec) {
        case NPF_FMT_SPEC_CONV_FLOAT_DEC:
        case NPF_FMT_SPEC_CONV_FLOAT_SCI:
        case NPF_FMT_SPEC_CONV_FLOAT_SHORTEST:
        case NPF_FMT_SPEC_CONV_FLOAT_HEX:
          fs.prec = 6;
          break;
        default:
          break;
      }
    }
  #endif

  if ((fs.prec_opt == NPF_FMT_SPEC_OPT_NONE) &&
      (fs.conv_spec == NPF_FMT_SPEC_CONV_POINTER)) {
        fs.prec = (sizeof(void *) * CHAR_BIT + 3) / 4;
  }
#endif

#if (NANOPRINTF_USE_PRECISION_FORMAT_SPECIFIERS == 1) && \
    (NANOPRINTF_USE_FIELD_WIDTH_FORMAT_SPECIFIERS == 1)
    // For d i o u x X, the '0' flag must be ignored if a precision is provided
    if (fs.prec_opt != NPF_FMT_SPEC_OPT_NONE) {
      switch (fs.conv_spec) {
        case NPF_FMT_SPEC_CONV_SIGNED_INT:
        case NPF_FMT_SPEC_CONV_OCTAL:
        case NPF_FMT_SPEC_CONV_HEX_INT:
        case NPF_FMT_SPEC_CONV_UNSIGNED_INT:
          fs.leading_zero_pad = 0;
          break;
        default:
          break;
      }
    }
#endif

    union { char cbuf_mem[NANOPRINTF_CONVERSION_BUFFER_SIZE]; npf_uint_t binval; } u;
    char *cbuf = u.cbuf_mem, sign_c = 0;
    int cbuf_len = 0;
    char need_0x = 0;
#if NANOPRINTF_USE_FIELD_WIDTH_FORMAT_SPECIFIERS == 1
    int field_pad = 0;
    char pad_c = 0;
#endif
#if NANOPRINTF_USE_PRECISION_FORMAT_SPECIFIERS == 1
    int prec_pad = 0;
#if NANOPRINTF_USE_FIELD_WIDTH_FORMAT_SPECIFIERS == 1
    uint_fast8_t zero = 0;
#endif
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
#if NANOPRINTF_USE_SMALL_FORMAT_SPECIFIERS == 1
          NPF_EXTRACT(SHORT, short, int);
          NPF_EXTRACT(CHAR, signed char, int);
#endif
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
      case NPF_FMT_SPEC_CONV_UNSIGNED_INT:
      case NPF_FMT_SPEC_CONV_POINTER: {
        npf_uint_t val = 0;

        if (fs.conv_spec == NPF_FMT_SPEC_CONV_POINTER) {
          val = (npf_uint_t)(uintptr_t)va_arg(args, void *);
        } else {
          switch (fs.length_modifier) {
            NPF_EXTRACT(NONE, unsigned, unsigned);
#if NANOPRINTF_USE_SMALL_FORMAT_SPECIFIERS == 1
            NPF_EXTRACT(SHORT, unsigned short, unsigned);
            NPF_EXTRACT(CHAR, unsigned char, unsigned);
#endif
            NPF_EXTRACT(LONG, unsigned long, unsigned long);
#if NANOPRINTF_USE_LARGE_FORMAT_SPECIFIERS == 1
            NPF_EXTRACT(LARGE_LONG_LONG, unsigned long long, unsigned long long);
            NPF_EXTRACT(LARGE_INTMAX, uintmax_t, uintmax_t);
            NPF_EXTRACT(LARGE_SIZET, size_t, size_t);
            NPF_EXTRACT(LARGE_PTRDIFFT, npf_uptrdiff_t, npf_uptrdiff_t);
#endif
            default: break;
          }
        }

#if NANOPRINTF_USE_PRECISION_FORMAT_SPECIFIERS == 1
#if NANOPRINTF_USE_FIELD_WIDTH_FORMAT_SPECIFIERS == 1
        zero = !val;
#endif
        if (!val && (fs.prec_opt != NPF_FMT_SPEC_OPT_NONE) && !fs.prec) {
          // Zero value and explicitly-requested zero precision means "print nothing".
#if NANOPRINTF_USE_ALT_FORM_FLAG == 1
          if ((fs.conv_spec == NPF_FMT_SPEC_CONV_OCTAL) && fs.alt_form) {
            fs.prec = 1; // octal special case, print a single '0'
          }
#endif
        } else
#endif
#if NANOPRINTF_USE_BINARY_FORMAT_SPECIFIERS == 1
        if (fs.conv_spec == NPF_FMT_SPEC_CONV_BINARY) {
          cbuf_len = npf_bin_len(val); u.binval = val;
        } else
#endif
        {
          uint_fast8_t const base = (fs.conv_spec == NPF_FMT_SPEC_CONV_OCTAL) ?
            8u : ((fs.conv_spec == NPF_FMT_SPEC_CONV_UNSIGNED_INT) ? 10u : 16u);
          cbuf_len = npf_utoa_rev(val, cbuf, base, fs.case_adjust);
        }

#if NANOPRINTF_USE_ALT_FORM_FLAG == 1
        if (val && fs.alt_form && (fs.conv_spec == NPF_FMT_SPEC_CONV_OCTAL)) {
          cbuf[cbuf_len++] = '0'; // OK to add leading octal '0' immediately.
        }

        if (val && fs.alt_form) { // 0x or 0b but can't write it yet.
          if ((fs.conv_spec == NPF_FMT_SPEC_CONV_HEX_INT) ||
              (fs.conv_spec == NPF_FMT_SPEC_CONV_POINTER)) { need_0x = 'X'; }
#if NANOPRINTF_USE_BINARY_FORMAT_SPECIFIERS == 1
          else if (fs.conv_spec == NPF_FMT_SPEC_CONV_BINARY) { need_0x = 'B'; }
#endif
          if (need_0x) { need_0x = (char)(need_0x + fs.case_adjust); }
        }
#endif
      } break;

#if NANOPRINTF_USE_WRITEBACK_FORMAT_SPECIFIERS == 1
      case NPF_FMT_SPEC_CONV_WRITEBACK:
        switch (fs.length_modifier) {
          NPF_WRITEBACK(NONE, int);
#if NANOPRINTF_USE_SMALL_FORMAT_SPECIFIERS == 1
          NPF_WRITEBACK(SHORT, short);
          NPF_WRITEBACK(CHAR, signed char);
#endif
          NPF_WRITEBACK(LONG, long);
#if NANOPRINTF_USE_LARGE_FORMAT_SPECIFIERS == 1
          NPF_WRITEBACK(LARGE_LONG_LONG, long long);
          NPF_WRITEBACK(LARGE_INTMAX, intmax_t);
          NPF_WRITEBACK(LARGE_SIZET, npf_ssize_t);
          NPF_WRITEBACK(LARGE_PTRDIFFT, ptrdiff_t);
#endif
          default: break;
        } break;
#endif

#if NANOPRINTF_USE_FLOAT_FORMAT_SPECIFIERS == 1
      case NPF_FMT_SPEC_CONV_FLOAT_DEC:
      case NPF_FMT_SPEC_CONV_FLOAT_SCI:
      case NPF_FMT_SPEC_CONV_FLOAT_SHORTEST:
      case NPF_FMT_SPEC_CONV_FLOAT_HEX: {
        npf_real_t val;
#if NANOPRINTF_FLOAT_SINGLE_PRECISION == 1
        val = va_arg(args, npf_float_t).val;
#else
        if (fs.length_modifier == NPF_FMT_SPEC_LEN_MOD_LONG_DOUBLE) {
          val = (npf_real_t)va_arg(args, long double);
        } else {
          val = va_arg(args, double);
        }
#endif

        sign_c = (npf_real_to_int_rep(val) >> NPF_REAL_SIGN_POS) ? '-' : fs.prepend;
#if NANOPRINTF_USE_FIELD_WIDTH_FORMAT_SPECIFIERS == 1
        zero = (val == 0);
#endif
        cbuf_len = npf_ftoa_rev(cbuf, &fs, val);
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
      // The '0' flag is only legal with numeric types, and '-' overrides '0'.
      if (fs.leading_zero_pad && !fs.left_justified) {
#if NANOPRINTF_USE_PRECISION_FORMAT_SPECIFIERS == 1
        if ((fs.prec_opt != NPF_FMT_SPEC_OPT_NONE) && !fs.prec && zero) {
          pad_c = ' ';
        } else
#endif
        { pad_c = '0'; }
      } else { pad_c = ' '; }
    }
#endif

    // Compute the number of bytes to truncate or '0'-pad.
#if NANOPRINTF_USE_PRECISION_FORMAT_SPECIFIERS == 1
    if (fs.conv_spec != NPF_FMT_SPEC_CONV_STRING) {
#if NANOPRINTF_USE_FLOAT_FORMAT_SPECIFIERS == 1
      // float precision is after the decimal point
      if ((fs.conv_spec != NPF_FMT_SPEC_CONV_FLOAT_DEC) &&
          (fs.conv_spec != NPF_FMT_SPEC_CONV_FLOAT_SCI) &&
          (fs.conv_spec != NPF_FMT_SPEC_CONV_FLOAT_SHORTEST) &&
          (fs.conv_spec != NPF_FMT_SPEC_CONV_FLOAT_HEX))
#endif
      { prec_pad = NPF_MAX(0, fs.prec - cbuf_len); }
    }
#endif

#if NANOPRINTF_USE_FIELD_WIDTH_FORMAT_SPECIFIERS == 1
    // Given the full converted length, how many pad bytes?
    field_pad = fs.field_width - cbuf_len - !!sign_c;
    if (need_0x) { field_pad -= 2; }
#if NANOPRINTF_USE_PRECISION_FORMAT_SPECIFIERS == 1
    field_pad -= prec_pad;
#endif
    field_pad = NPF_MAX(0, field_pad);

    // Apply right-justified field width if requested
    if (!fs.left_justified && pad_c) { // If leading zeros pad, sign goes first.
      if (pad_c == '0') {
        if (sign_c) { NPF_PUTC(sign_c); sign_c = 0; }
        // Pad byte is '0', write '0x' before '0' pad chars.
        if (need_0x) { NPF_PUTC('0'); NPF_PUTC(need_0x); }
      }
      while (field_pad-- > 0) { NPF_PUTC(pad_c); }
      // Pad byte is ' ', write '0x' after ' ' pad chars but before number.
      if ((pad_c != '0') && need_0x) { NPF_PUTC('0'); NPF_PUTC(need_0x); }
    } else
#endif
    { if (need_0x) { NPF_PUTC('0'); NPF_PUTC(need_0x); } } // no pad, '0x' requested.

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

int npf_vsnprintf(char * NPF_RESTRICT buffer,
                  size_t bufsz,
                  char const * NPF_RESTRICT format,
                  va_list vlist) {
  npf_bufputc_ctx_t bufputc_ctx;
  bufputc_ctx.dst = buffer;
  bufputc_ctx.len = bufsz;
  bufputc_ctx.cur = 0;

  npf_putc const pc = buffer ? npf_bufputc : npf_bufputc_nop;
  int const n = npf_vpprintf(pc, &bufputc_ctx, format, vlist);

  if (buffer && bufsz) {
#ifdef NANOPRINTF_SNPRINTF_SAFE_EMPTY_STRING_ON_OVERFLOW
    buffer[(n < 0 || (unsigned)n >= bufsz) ? 0 : n] = '\0';
#else
    buffer[n < 0 ? 0 : NPF_MIN((unsigned)n, bufsz - 1)] = '\0';
#endif
  }

  return n;
}

int npf_pprintf_impl(npf_putc pc,
                     void * NPF_RESTRICT pc_ctx,
                     char const * NPF_RESTRICT format,
                     ...) {
  va_list val;
  va_start(val, format);
  int const rv = npf_vpprintf(pc, pc_ctx, format, val);
  va_end(val);
  return rv;
}

int npf_snprintf_impl(char * NPF_RESTRICT buffer,
                      size_t bufsz,
                      const char * NPF_RESTRICT format,
                      ...) {
  va_list val;
  va_start(val, format);
  int const rv = npf_vsnprintf(buffer, bufsz, format, val);
  va_end(val);
  return rv;
}

#if NPF_HAVE_GCC_WARNING_PRAGMAS
  #pragma GCC diagnostic pop
#endif

#ifdef _MSC_VER
  #pragma warning(pop)
#endif

#endif // NPF_IMPLEMENTATION_INCLUDED
#endif // NANOPRINTF_IMPLEMENTATION

// Single-precision argument wrapping and MAP macro expansion machinery.
// The npf_snprintf / npf_pprintf / NPF_MAP_ARGS macros defined above reference
// these, but that's fine: macro bodies are only expanded at the point of
// invocation, not at the point of definition.

#ifndef NPF_MAP_INCLUDED
#define NPF_MAP_INCLUDED

#if defined(NANOPRINTF_FLOAT_SINGLE_PRECISION) && NANOPRINTF_FLOAT_SINGLE_PRECISION == 1

// NPF__WRAP: wrap float/double args into npf_float_t, pass other types through.
#if defined(__cplusplus)
  static inline npf_float_t npf__wrap_impl(float f) { npf_float_t r; r.val = f; return r; }
  static inline npf_float_t npf__wrap_impl(double d) { npf_float_t r; r.val = (float)d; return r; }
  template<typename T> static inline T npf__wrap_impl(T v) { return v; }
  #define NPF__WRAP(x) npf__wrap_impl(x)
#elif defined(__GNUC__) || defined(__clang__)
  // The inner __builtin_choose_expr uses 0 as a safe fallback so (float)(x) is
  // never instantiated for non-numeric types (avoiding pointer-to-float cast errors).
  #define NPF__IS_REAL(x) (__builtin_types_compatible_p(__typeof__(x), float) || \
                            __builtin_types_compatible_p(__typeof__(x), double))
  #define NPF__WRAP(x) __builtin_choose_expr(NPF__IS_REAL(x), \
    ({ npf_float_t _npf_r; \
       _npf_r.val = (float)__builtin_choose_expr(NPF__IS_REAL(x), (x), 0); \
       _npf_r; }), \
    (x))
#else
  #error Single-precision float wrapping requires GCC, Clang, or C++.
#endif

// Argument counting (up to 64 variadic args)
#define NPF__NARG(...)  NPF__NARG_(__VA_ARGS__, NPF__RSEQ())
#define NPF__NARG_(...) NPF__ARG_N(__VA_ARGS__)
#define NPF__ARG_N( \
   _1, _2, _3, _4, _5, _6, _7, _8, _9,_10, _11,_12,_13,_14,_15,_16,_17,_18,_19,_20, \
  _21,_22,_23,_24,_25,_26,_27,_28,_29,_30, _31,_32,_33,_34,_35,_36,_37,_38,_39,_40, \
  _41,_42,_43,_44,_45,_46,_47,_48,_49,_50, _51,_52,_53,_54,_55,_56,_57,_58,_59,_60, \
  _61,_62,_63,_64,N,...) N
#define NPF__RSEQ() \
  64,63,62,61,60,59,58,57,56,55,54,53,52,51,50,49,48,47,46,45,44,43,42,41,40,39,38, \
  37,36,35,34,33,32,31,30,29,28,27,26,25,24,23,22,21,20,19,18,17,16,15,14,13,12,11, \
  10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0

// Token pasting
#define NPF__CAT(a,b)  NPF__CAT_(a,b)
#define NPF__CAT_(a,b) a##b

// MAP: apply f to each argument
#define NPF__MAP(f,...) NPF__CAT(NPF__MAP_,NPF__NARG(__VA_ARGS__))(f,__VA_ARGS__)
#define NPF__MAP_1(f,a)      f(a)
#define NPF__MAP_2(f,a,...)  f(a), NPF__MAP_1(f,__VA_ARGS__)
#define NPF__MAP_3(f,a,...)  f(a), NPF__MAP_2(f,__VA_ARGS__)
#define NPF__MAP_4(f,a,...)  f(a), NPF__MAP_3(f,__VA_ARGS__)
#define NPF__MAP_5(f,a,...)  f(a), NPF__MAP_4(f,__VA_ARGS__)
#define NPF__MAP_6(f,a,...)  f(a), NPF__MAP_5(f,__VA_ARGS__)
#define NPF__MAP_7(f,a,...)  f(a), NPF__MAP_6(f,__VA_ARGS__)
#define NPF__MAP_8(f,a,...)  f(a), NPF__MAP_7(f,__VA_ARGS__)
#define NPF__MAP_9(f,a,...)  f(a), NPF__MAP_8(f,__VA_ARGS__)
#define NPF__MAP_10(f,a,...) f(a), NPF__MAP_9(f,__VA_ARGS__)
#define NPF__MAP_11(f,a,...) f(a), NPF__MAP_10(f,__VA_ARGS__)
#define NPF__MAP_12(f,a,...) f(a), NPF__MAP_11(f,__VA_ARGS__)
#define NPF__MAP_13(f,a,...) f(a), NPF__MAP_12(f,__VA_ARGS__)
#define NPF__MAP_14(f,a,...) f(a), NPF__MAP_13(f,__VA_ARGS__)
#define NPF__MAP_15(f,a,...) f(a), NPF__MAP_14(f,__VA_ARGS__)
#define NPF__MAP_16(f,a,...) f(a), NPF__MAP_15(f,__VA_ARGS__)
#define NPF__MAP_17(f,a,...) f(a), NPF__MAP_16(f,__VA_ARGS__)
#define NPF__MAP_18(f,a,...) f(a), NPF__MAP_17(f,__VA_ARGS__)
#define NPF__MAP_19(f,a,...) f(a), NPF__MAP_18(f,__VA_ARGS__)
#define NPF__MAP_20(f,a,...) f(a), NPF__MAP_19(f,__VA_ARGS__)
#define NPF__MAP_21(f,a,...) f(a), NPF__MAP_20(f,__VA_ARGS__)
#define NPF__MAP_22(f,a,...) f(a), NPF__MAP_21(f,__VA_ARGS__)
#define NPF__MAP_23(f,a,...) f(a), NPF__MAP_22(f,__VA_ARGS__)
#define NPF__MAP_24(f,a,...) f(a), NPF__MAP_23(f,__VA_ARGS__)
#define NPF__MAP_25(f,a,...) f(a), NPF__MAP_24(f,__VA_ARGS__)
#define NPF__MAP_26(f,a,...) f(a), NPF__MAP_25(f,__VA_ARGS__)
#define NPF__MAP_27(f,a,...) f(a), NPF__MAP_26(f,__VA_ARGS__)
#define NPF__MAP_28(f,a,...) f(a), NPF__MAP_27(f,__VA_ARGS__)
#define NPF__MAP_29(f,a,...) f(a), NPF__MAP_28(f,__VA_ARGS__)
#define NPF__MAP_30(f,a,...) f(a), NPF__MAP_29(f,__VA_ARGS__)
#define NPF__MAP_31(f,a,...) f(a), NPF__MAP_30(f,__VA_ARGS__)
#define NPF__MAP_32(f,a,...) f(a), NPF__MAP_31(f,__VA_ARGS__)
#define NPF__MAP_33(f,a,...) f(a), NPF__MAP_32(f,__VA_ARGS__)
#define NPF__MAP_34(f,a,...) f(a), NPF__MAP_33(f,__VA_ARGS__)
#define NPF__MAP_35(f,a,...) f(a), NPF__MAP_34(f,__VA_ARGS__)
#define NPF__MAP_36(f,a,...) f(a), NPF__MAP_35(f,__VA_ARGS__)
#define NPF__MAP_37(f,a,...) f(a), NPF__MAP_36(f,__VA_ARGS__)
#define NPF__MAP_38(f,a,...) f(a), NPF__MAP_37(f,__VA_ARGS__)
#define NPF__MAP_39(f,a,...) f(a), NPF__MAP_38(f,__VA_ARGS__)
#define NPF__MAP_40(f,a,...) f(a), NPF__MAP_39(f,__VA_ARGS__)
#define NPF__MAP_41(f,a,...) f(a), NPF__MAP_40(f,__VA_ARGS__)
#define NPF__MAP_42(f,a,...) f(a), NPF__MAP_41(f,__VA_ARGS__)
#define NPF__MAP_43(f,a,...) f(a), NPF__MAP_42(f,__VA_ARGS__)
#define NPF__MAP_44(f,a,...) f(a), NPF__MAP_43(f,__VA_ARGS__)
#define NPF__MAP_45(f,a,...) f(a), NPF__MAP_44(f,__VA_ARGS__)
#define NPF__MAP_46(f,a,...) f(a), NPF__MAP_45(f,__VA_ARGS__)
#define NPF__MAP_47(f,a,...) f(a), NPF__MAP_46(f,__VA_ARGS__)
#define NPF__MAP_48(f,a,...) f(a), NPF__MAP_47(f,__VA_ARGS__)
#define NPF__MAP_49(f,a,...) f(a), NPF__MAP_48(f,__VA_ARGS__)
#define NPF__MAP_50(f,a,...) f(a), NPF__MAP_49(f,__VA_ARGS__)
#define NPF__MAP_51(f,a,...) f(a), NPF__MAP_50(f,__VA_ARGS__)
#define NPF__MAP_52(f,a,...) f(a), NPF__MAP_51(f,__VA_ARGS__)
#define NPF__MAP_53(f,a,...) f(a), NPF__MAP_52(f,__VA_ARGS__)
#define NPF__MAP_54(f,a,...) f(a), NPF__MAP_53(f,__VA_ARGS__)
#define NPF__MAP_55(f,a,...) f(a), NPF__MAP_54(f,__VA_ARGS__)
#define NPF__MAP_56(f,a,...) f(a), NPF__MAP_55(f,__VA_ARGS__)
#define NPF__MAP_57(f,a,...) f(a), NPF__MAP_56(f,__VA_ARGS__)
#define NPF__MAP_58(f,a,...) f(a), NPF__MAP_57(f,__VA_ARGS__)
#define NPF__MAP_59(f,a,...) f(a), NPF__MAP_58(f,__VA_ARGS__)
#define NPF__MAP_60(f,a,...) f(a), NPF__MAP_59(f,__VA_ARGS__)
#define NPF__MAP_61(f,a,...) f(a), NPF__MAP_60(f,__VA_ARGS__)
#define NPF__MAP_62(f,a,...) f(a), NPF__MAP_61(f,__VA_ARGS__)
#define NPF__MAP_63(f,a,...) f(a), NPF__MAP_62(f,__VA_ARGS__)
#define NPF__MAP_64(f,a,...) f(a), NPF__MAP_63(f,__VA_ARGS__)

#endif // NANOPRINTF_FLOAT_SINGLE_PRECISION

#endif // NPF_MAP_INCLUDED

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
