/*
    nanoprintf: a tiny embeddable printf replacement written in C.
    https://github.com/charlesnicholson/nanoprintf
    charles.nicholson+nanoprintf@gmail.com

    LICENSE:
    --------
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
*/

#ifndef NANOPRINTF_IMPLEMENTATION

#ifndef NANOPRINTF_H_INCLUDED
#define NANOPRINTF_H_INCLUDED

#ifndef NANOPRINTF_USE_LARGE_FORMAT_SPECIFIERS
#error NANOPRINTF_USE_LARGE_FORMAT_SPECIFIERS must be #defined to 0 or 1
#endif

#ifndef NANOPRINTF_USE_FLOAT_FORMAT_SPECIFIERS
#error NANOPRINTF_USE_FLOAT_FORMAT_SPECIFIERS must be #defined to 0 or 1
#endif

/* %n is an attack vector, allow it to be disabled. */
#ifndef NANOPRINTF_USE_WRITEBACK_FORMAT_SPECIFIERS
#error NANOPRINTF_USE_WRITEBACK_FORMAT_SPECIFIERS must be #defined to 0 or 1
#endif

#ifdef NANOPRINTF_VISIBILITY_STATIC
#define NPF_VISIBILITY static
#else
#define NPF_VISIBILITY extern
#endif

#if NANOPRINTF_USE_FLOAT_FORMAT_SPECIFIERS == 1
#ifdef __cplusplus
#if __cplusplus < 201103L
#error nanoprintf float support requires fixed-width types from c++11 or later.
#endif
#else
#if __STDC_VERSION__ < 199409L
#error nanoprintf float support requires fixed-width types from c99 or later.
#endif
#endif
#endif

#include <stdarg.h>
#include <stddef.h>

#if NANOPRINTF_USE_FLOAT_FORMAT_SPECIFIERS == 1
#include <stdint.h>
#endif

#if NANOPRINTF_USE_LARGE_FORMAT_SPECIFIERS == 1
#include <inttypes.h>
#endif

#if defined(__clang__)
#define NPF_PRINTF_ATTR(FORMAT_INDEX, VARGS_INDEX) \
    __attribute__((__format__(__printf__, FORMAT_INDEX, VARGS_INDEX)))
#elif defined(__GNUC__) || defined(__GNUG__)
#define NPF_PRINTF_ATTR(FORMAT_INDEX, VARGS_INDEX) \
    __attribute__((format(printf, FORMAT_INDEX, VARGS_INDEX)))
#else
#define NPF_PRINTF_ATTR(FORMAT_INDEX, VARGS_INDEX)
#endif

#ifdef __cplusplus
extern "C" {
#endif

/* Public API */

NPF_VISIBILITY int npf_snprintf(char *buffer, size_t bufsz, const char *format,
                                ...) NPF_PRINTF_ATTR(3, 4);

NPF_VISIBILITY int npf_vsnprintf(char *buffer, size_t bufsz, char const *format,
                                 va_list vlist) NPF_PRINTF_ATTR(3, 0);

typedef void (*npf_putc)(int c, void *ctx);
NPF_VISIBILITY int npf_pprintf(npf_putc pc, void *pc_ctx, char const *format,
                               ...) NPF_PRINTF_ATTR(3, 4);

NPF_VISIBILITY int npf_vpprintf(npf_putc pc, void *pc_ctx, char const *format,
                                va_list vlist) NPF_PRINTF_ATTR(3, 0);

/* Internal */

typedef enum {
    NPF_FMT_SPEC_FIELD_WIDTH_NONE,
    NPF_FMT_SPEC_FIELD_WIDTH_LITERAL
} npf__format_spec_field_width_t;

typedef enum {
    NPF_FMT_SPEC_PRECISION_NONE,
    NPF_FMT_SPEC_PRECISION_LITERAL
} npf__format_spec_precision_t;

typedef enum {
    NPF_FMT_SPEC_LEN_MOD_NONE,
    NPF_FMT_SPEC_LEN_MOD_SHORT,       /* 'h' */
    NPF_FMT_SPEC_LEN_MOD_LONG,        /* 'l' */
    NPF_FMT_SPEC_LEN_MOD_LONG_DOUBLE, /* 'L' */
    NPF_FMT_SPEC_LEN_MOD_CHAR         /* 'hh' */
#if NANOPRINTF_USE_LARGE_FORMAT_SPECIFIERS == 1
    ,
    NPF_FMT_SPEC_LEN_MOD_LARGE_LONG_LONG, /* 'll' */
    NPF_FMT_SPEC_LEN_MOD_LARGE_INTMAX,    /* 'j' */
    NPF_FMT_SPEC_LEN_MOD_LARGE_SIZET,     /* 'z' */
    NPF_FMT_SPEC_LEN_MOD_LARGE_PTRDIFFT   /* 't' */
#endif
} npf__format_spec_length_modifier_t;

typedef enum {
    NPF_FMT_SPEC_CONV_PERCENT,      /* '%' */
    NPF_FMT_SPEC_CONV_CHAR,         /* 'c' */
    NPF_FMT_SPEC_CONV_STRING,       /* 's' */
    NPF_FMT_SPEC_CONV_SIGNED_INT,   /* 'i', 'd' */
    NPF_FMT_SPEC_CONV_OCTAL,        /* 'o' */
    NPF_FMT_SPEC_CONV_HEX_INT,      /* 'x', 'X' */
    NPF_FMT_SPEC_CONV_UNSIGNED_INT, /* 'u' */
    NPF_FMT_SPEC_CONV_POINTER       /* 'p' */
#if NANOPRINTF_USE_WRITEBACK_FORMAT_SPECIFIERS == 1
    ,
    NPF_FMT_SPEC_CONV_WRITEBACK /* 'n' */
#endif
#if NANOPRINTF_USE_FLOAT_FORMAT_SPECIFIERS == 1
    ,
    NPF_FMT_SPEC_CONV_FLOAT_DECIMAL,  /* 'f', 'F' */
    NPF_FMT_SPEC_CONV_FLOAT_EXPONENT, /* 'e', 'E' */
    NPF_FMT_SPEC_CONV_FLOAT_DYNAMIC,  /* 'g', 'G' */
    NPF_FMT_SPEC_CONV_FLOAT_HEXPONENT /* 'a', 'A' */
#endif
} npf__format_spec_conversion_t;

typedef enum {
    NPF_FMT_SPEC_CONV_CASE_NONE,
    NPF_FMT_SPEC_CONV_CASE_LOWER,
    NPF_FMT_SPEC_CONV_CASE_UPPER
} npf__format_spec_conversion_case_t;

typedef struct {
    /* optional flags */
    unsigned left_justified : 1;   /* '-' */
    unsigned prepend_sign : 1;     /* '+' */
    unsigned prepend_space : 1;    /* ' ' */
    unsigned alternative_form : 1; /* '#' */
    unsigned leading_zero_pad : 1; /* '0' */

    /* field width */
    npf__format_spec_field_width_t field_width_type;
    int field_width;

    /* precision */
    npf__format_spec_precision_t precision_type;
    int precision;

    /* length modifier for specifying argument size */
    npf__format_spec_length_modifier_t length_modifier;

    /* conversion specifiers */
    npf__format_spec_conversion_t conv_spec;
    npf__format_spec_conversion_case_t conv_spec_case;
} npf__format_spec_t;

#if NANOPRINTF_USE_LARGE_FORMAT_SPECIFIERS == 0
typedef long npf__int_t;
typedef unsigned long npf__uint_t;
#else
typedef intmax_t npf__int_t;
typedef uintmax_t npf__uint_t;
#endif

NPF_VISIBILITY int npf__parse_format_spec(char const *format, va_list vlist,
                                          npf__format_spec_t *out_spec);

typedef struct {
    char *dst;
    size_t len;
    size_t cur;
} npf__bufputc_ctx_t;

NPF_VISIBILITY void npf__bufputc(int c, void *ctx);
NPF_VISIBILITY void npf__bufputc_nop(int c, void *ctx);
NPF_VISIBILITY int npf__itoa_rev(char *buf, npf__int_t i);
NPF_VISIBILITY int npf__utoa_rev(char *buf, npf__uint_t i, unsigned base,
                                 npf__format_spec_conversion_case_t cc);
NPF_VISIBILITY int npf__ptoa_rev(char *buf, void const *p);
#if NANOPRINTF_USE_FLOAT_FORMAT_SPECIFIERS == 1
NPF_VISIBILITY int npf__fsplit_abs(float f, uint64_t *out_int_part,
                                   uint64_t *out_frac_part,
                                   int *out_frac_base10_neg_e);
NPF_VISIBILITY int npf__ftoa_rev(char *buf, float f, unsigned base,
                                 npf__format_spec_conversion_case_t cc,
                                 int *out_frac_chars);
#endif

#ifdef __cplusplus
}
#endif

#endif /* NANOPRINTF_H_INCLUDED */

#else /* NANOPRINTF_IMPLEMENTATION */

#undef NANOPRINTF_IMPLEMENTATION
#include "nanoprintf.h"
#define NANOPRINTF_IMPLEMENTATION

#if NANOPRINTF_USE_FLOAT_FORMAT_SPECIFIERS == 1
#include <math.h>
#endif

#define NPF_MIN(x, y) ((x) < (y) ? (x) : (y))
#define NPF_MAX(x, y) ((x) > (y) ? (x) : (y))

#ifdef __cplusplus
extern "C" {
#endif

int npf__parse_format_spec(char const *format, va_list vlist,
                           npf__format_spec_t *out_spec) {
    char const *cur = format;

    out_spec->left_justified = 0;
    out_spec->prepend_sign = 0;
    out_spec->prepend_space = 0;
    out_spec->alternative_form = 0;
    out_spec->leading_zero_pad = 0;
    out_spec->field_width_type = NPF_FMT_SPEC_FIELD_WIDTH_NONE;
    out_spec->length_modifier = NPF_FMT_SPEC_LEN_MOD_NONE;
    out_spec->conv_spec_case = NPF_FMT_SPEC_CONV_CASE_NONE;

    /* Format specifiers start with % */
    if (*cur++ != '%') {
        return 0;
    }

    /* Optional flags */
    while (*cur) {
        switch (*cur++) {
            case '-':
                out_spec->left_justified = 1;
                out_spec->leading_zero_pad = 0;
                continue;
            case '+':
                out_spec->prepend_sign = 1;
                out_spec->prepend_space = 0;
                continue;
            case ' ':
                out_spec->prepend_space = !out_spec->prepend_sign;
                continue;
            case '#':
                out_spec->alternative_form = 1;
                continue;
            case '0':
                out_spec->leading_zero_pad = !out_spec->left_justified;
                continue;
            default:
                break;
        }
        --cur;
        break;
    }

    /* Minimum field width */
    if (*cur == '*') {
        /* '*' modifiers require more varargs */
        int const field_width_star = va_arg(vlist, int);
        out_spec->field_width_type = NPF_FMT_SPEC_FIELD_WIDTH_LITERAL;
        if (field_width_star >= 0) {
            out_spec->field_width = field_width_star;
        } else {
            out_spec->field_width = -field_width_star;
            out_spec->left_justified = 1;
        }
        ++cur;
    } else {
        out_spec->field_width = 0;
        if ((*cur >= '0') && (*cur <= '9')) {
            out_spec->field_width_type = NPF_FMT_SPEC_FIELD_WIDTH_LITERAL;
        }
        while ((*cur >= '0') && (*cur <= '9')) {
            out_spec->field_width =
                (out_spec->field_width * 10) + (*cur++ - '0');
        }
    }

    /* Precision */
    out_spec->precision_type = NPF_FMT_SPEC_PRECISION_NONE;
    if (*cur == '.') {
        ++cur;
        if (*cur == '*') {
            int const star_precision = va_arg(vlist, int);
            if (star_precision >= 0) {
                out_spec->precision_type = NPF_FMT_SPEC_PRECISION_LITERAL;
                out_spec->precision = star_precision;
            }
            ++cur;
        } else if (*cur == '-') {
            /* ignore negative precision */
            out_spec->precision_type = NPF_FMT_SPEC_PRECISION_NONE;
            ++cur;
            while ((*cur >= '0') && (*cur <= '9')) {
                ++cur;
            }
        } else {
            out_spec->precision = 0;
            out_spec->precision_type = NPF_FMT_SPEC_PRECISION_LITERAL;
            while ((*cur >= '0') && (*cur <= '9')) {
                out_spec->precision =
                    (out_spec->precision * 10) + (*cur++ - '0');
            }
        }
    }

    /* Length modifier */
    switch (*cur++) {
        case 'h':
            if (*cur == 'h') {
                out_spec->length_modifier = NPF_FMT_SPEC_LEN_MOD_CHAR;
                ++cur;
            } else {
                out_spec->length_modifier = NPF_FMT_SPEC_LEN_MOD_SHORT;
            }
            break;
        case 'l':
#if NANOPRINTF_USE_LARGE_FORMAT_SPECIFIERS == 1
            if (*cur == 'l') {
                out_spec->length_modifier =
                    NPF_FMT_SPEC_LEN_MOD_LARGE_LONG_LONG;
                ++cur;
            } else
#endif
                out_spec->length_modifier = NPF_FMT_SPEC_LEN_MOD_LONG;
            break;
#if NANOPRINTF_USE_FLOAT_FORMAT_SPECIFIERS == 1
        case 'L':
            out_spec->length_modifier = NPF_FMT_SPEC_LEN_MOD_LONG_DOUBLE;
            break;
#endif
#if NANOPRINTF_USE_LARGE_FORMAT_SPECIFIERS == 1
        case 'j':
            out_spec->length_modifier = NPF_FMT_SPEC_LEN_MOD_LARGE_INTMAX;
            break;
        case 'z':
            out_spec->length_modifier = NPF_FMT_SPEC_LEN_MOD_LARGE_SIZET;
            break;
        case 't':
            out_spec->length_modifier = NPF_FMT_SPEC_LEN_MOD_LARGE_PTRDIFFT;
            break;
#endif
        default:
            --cur;
            break;
    }

    /* Conversion specifier */
    switch (*cur++) {
        case '%':
            out_spec->conv_spec = NPF_FMT_SPEC_CONV_PERCENT;
            out_spec->precision_type = NPF_FMT_SPEC_PRECISION_NONE;
            break;
        case 'c':
            out_spec->conv_spec = NPF_FMT_SPEC_CONV_CHAR;
            out_spec->precision_type = NPF_FMT_SPEC_PRECISION_NONE;
            break;
        case 's':
            out_spec->conv_spec = NPF_FMT_SPEC_CONV_STRING;
            out_spec->leading_zero_pad = 0;
            break;
        case 'i':
            out_spec->conv_spec = NPF_FMT_SPEC_CONV_SIGNED_INT;
            break;
        case 'd':
            out_spec->conv_spec = NPF_FMT_SPEC_CONV_SIGNED_INT;
            break;
        case 'o':
            out_spec->conv_spec = NPF_FMT_SPEC_CONV_OCTAL;
            break;
        case 'x':
            out_spec->conv_spec = NPF_FMT_SPEC_CONV_HEX_INT;
            out_spec->conv_spec_case = NPF_FMT_SPEC_CONV_CASE_LOWER;
            break;
        case 'X':
            out_spec->conv_spec = NPF_FMT_SPEC_CONV_HEX_INT;
            out_spec->conv_spec_case = NPF_FMT_SPEC_CONV_CASE_UPPER;
            break;
        case 'u':
            out_spec->conv_spec = NPF_FMT_SPEC_CONV_UNSIGNED_INT;
            break;
#if NANOPRINTF_USE_FLOAT_FORMAT_SPECIFIERS == 1
        case 'f':
            out_spec->conv_spec = NPF_FMT_SPEC_CONV_FLOAT_DECIMAL;
            out_spec->conv_spec_case = NPF_FMT_SPEC_CONV_CASE_LOWER;
            break;
        case 'F':
            out_spec->conv_spec = NPF_FMT_SPEC_CONV_FLOAT_DECIMAL;
            out_spec->conv_spec_case = NPF_FMT_SPEC_CONV_CASE_UPPER;
            break;
        case 'e':
            out_spec->conv_spec = NPF_FMT_SPEC_CONV_FLOAT_EXPONENT;
            out_spec->conv_spec_case = NPF_FMT_SPEC_CONV_CASE_LOWER;
            break;
        case 'E':
            out_spec->conv_spec = NPF_FMT_SPEC_CONV_FLOAT_EXPONENT;
            out_spec->conv_spec_case = NPF_FMT_SPEC_CONV_CASE_UPPER;
            break;
        case 'a':
            out_spec->conv_spec = NPF_FMT_SPEC_CONV_FLOAT_HEXPONENT;
            out_spec->conv_spec_case = NPF_FMT_SPEC_CONV_CASE_LOWER;
            break;
        case 'A':
            out_spec->conv_spec = NPF_FMT_SPEC_CONV_FLOAT_HEXPONENT;
            out_spec->conv_spec_case = NPF_FMT_SPEC_CONV_CASE_UPPER;
            break;
        case 'g':
            out_spec->conv_spec = NPF_FMT_SPEC_CONV_FLOAT_DYNAMIC;
            out_spec->conv_spec_case = NPF_FMT_SPEC_CONV_CASE_LOWER;
            break;
        case 'G':
            out_spec->conv_spec = NPF_FMT_SPEC_CONV_FLOAT_DYNAMIC;
            out_spec->conv_spec_case = NPF_FMT_SPEC_CONV_CASE_UPPER;
            break;
#endif
#if NANOPRINTF_USE_WRITEBACK_FORMAT_SPECIFIERS == 1
        case 'n':
            /* todo: reject string if flags or width or precision exist */
            out_spec->conv_spec = NPF_FMT_SPEC_CONV_WRITEBACK;
            out_spec->precision_type = NPF_FMT_SPEC_PRECISION_NONE;
            break;
#endif
        case 'p':
            out_spec->conv_spec = NPF_FMT_SPEC_CONV_POINTER;
            out_spec->precision_type = NPF_FMT_SPEC_PRECISION_NONE;
            break;
        default:
            return 0;
    }

    if (out_spec->precision_type == NPF_FMT_SPEC_PRECISION_NONE) {
        switch (out_spec->conv_spec) {
            case NPF_FMT_SPEC_CONV_PERCENT:
            case NPF_FMT_SPEC_CONV_CHAR:
            case NPF_FMT_SPEC_CONV_STRING:
            case NPF_FMT_SPEC_CONV_POINTER:
#if NANOPRINTF_USE_WRITEBACK_FORMAT_SPECIFIERS == 1
            case NPF_FMT_SPEC_CONV_WRITEBACK:
#endif
                out_spec->precision = 0;
                break;
            case NPF_FMT_SPEC_CONV_SIGNED_INT:
            case NPF_FMT_SPEC_CONV_OCTAL:
            case NPF_FMT_SPEC_CONV_HEX_INT:
            case NPF_FMT_SPEC_CONV_UNSIGNED_INT:
                out_spec->precision = 1;
                break;
#if NANOPRINTF_USE_FLOAT_FORMAT_SPECIFIERS == 1
            case NPF_FMT_SPEC_CONV_FLOAT_DECIMAL:
            case NPF_FMT_SPEC_CONV_FLOAT_EXPONENT:
            case NPF_FMT_SPEC_CONV_FLOAT_DYNAMIC:
            case NPF_FMT_SPEC_CONV_FLOAT_HEXPONENT:
#endif
                out_spec->precision = 6;
                break;
        }
    }

    return (int)(cur - format);
}

void npf__bufputc(int c, void *ctx) {
    npf__bufputc_ctx_t *bpc = (npf__bufputc_ctx_t *)ctx;
    if (bpc->cur < bpc->len - 1) {
        bpc->dst[bpc->cur++] = (char)c;
    }
}

void npf__bufputc_nop(int c, void *ctx) {
    (void)c;
    (void)ctx;
}

int npf__itoa_rev(char *buf, npf__int_t i) {
    char *dst = buf;
    if (i == 0) {
        *dst++ = '0';
    } else {
        int const neg = (i < 0) ? -1 : 1;
        while (i) {
            *dst++ = (char)('0' + (neg * (i % 10)));
            i /= 10;
        }
    }
    return (int)(dst - buf);
}

int npf__utoa_rev(char *buf, npf__uint_t i, unsigned base,
                  npf__format_spec_conversion_case_t cc) {
    char *dst = buf;
    if (i == 0) {
        *dst++ = '0';
    } else {
        unsigned const base_c =
            (cc == NPF_FMT_SPEC_CONV_CASE_LOWER) ? 'a' : 'A';
        while (i) {
            unsigned const d = i % base;
            i /= base;
            *dst++ = (d < 10) ? (char)('0' + d) : (char)(base_c + (d - 10));
        }
    }
    return (int)(dst - buf);
}

int npf__ptoa_rev(char *buf, void const *p) {
    if (!p) {
        *buf++ = ')';
        *buf++ = 'l';
        *buf++ = 'l';
        *buf++ = 'u';
        *buf++ = 'n';
        *buf++ = '(';
        return 6;
    } else {
        /* c89 requires configuration to learn what uint a void* fits in.
        Instead, just alias to char* and print nibble-by-nibble. */
        unsigned i;
        char const *pb = (char const *)&p;
        char *dst = buf;
        for (i = 0; i < sizeof(void *); ++i) {
            unsigned const d1 = pb[i] & 0xF;
            unsigned const d2 = (pb[i] >> 4) & 0xF;
            *dst++ = (d1 < 10) ? (char)('0' + d1) : (char)('a' + (d1 - 10));
            *dst++ = (d2 < 10) ? (char)('0' + d2) : (char)('a' + (d2 - 10));
        }
        while (*--dst == '0')
            ;
        ++dst;
        *dst++ = 'x';
        *dst++ = '0';
        return (int)(dst - buf);
    }
}

#if NANOPRINTF_USE_FLOAT_FORMAT_SPECIFIERS == 1
enum {
    NPF_MANTISSA_BITS = 23,
    NPF_EXPONENT_BITS = 8,
    NPF_EXPONENT_BIAS = 127,
    NPF_FRACTION_BIN_DIGITS = 64,
    NPF_MAX_FRACTION_DEC_DIGITS = 8
};

int npf__fsplit_abs(float f, uint64_t *out_int_part, uint64_t *out_frac_part,
                    int *out_frac_base10_neg_exp) {
    /* conversion algorithm by Wojciech Muła (zdjęcia@garnek.pl)
       http://0x80.pl/notesen/2015-12-29-float-to-string.html
       grisu2 (https://bit.ly/2JgMggX) and ryu (https://bit.ly/2RLXSg0)
       are fast + precise + round, but require large lookup tables.
    */

    /* union-cast is UB, so copy through char*, compiler can optimize. */
    uint32_t f_bits;
    {
        char const *src = (char const *)&f;
        char *dst = (char *)&f_bits;
        *dst++ = *src++;
        *dst++ = *src++;
        *dst++ = *src++;
        *dst++ = *src++;
    }

    int const exponent = ((int)((f_bits >> NPF_MANTISSA_BITS) &
                                ((1u << NPF_EXPONENT_BITS) - 1u)) -
                          NPF_EXPONENT_BIAS) -
                         NPF_MANTISSA_BITS;

    /* value is out of range */
    if (exponent >= (64 - NPF_MANTISSA_BITS)) {
        return 0;
    }

    uint32_t const implicit_one = 1u << NPF_MANTISSA_BITS;
    uint32_t const mantissa = f_bits & (implicit_one - 1);
    uint32_t const mantissa_norm = mantissa | implicit_one;

    if (exponent > 0) {
        *out_int_part = (uint64_t)mantissa_norm << exponent;
    } else if (exponent < 0) {
        if (-exponent > NPF_MANTISSA_BITS) {
            *out_int_part = 0;
        } else {
            *out_int_part = mantissa_norm >> -exponent;
        }
    } else {
        *out_int_part = mantissa_norm;
    }

    uint64_t frac;
    {
        int const shift = NPF_FRACTION_BIN_DIGITS + exponent - 4;
        if ((shift >= (NPF_FRACTION_BIN_DIGITS - 4)) || (shift < 0)) {
            frac = 0;
        } else {
            frac = ((uint64_t)mantissa_norm) << shift;
        }
        /* multiply off the leading one's digit */
        frac &= 0x0fffffffffffffffllu;
        frac *= 10;
    }

    {
        /* Count the number of 0s at the beginning of the fractional part. */
        int frac_base10_neg_exp = 0;
        while (frac && ((frac >> (NPF_FRACTION_BIN_DIGITS - 4))) == 0) {
            ++frac_base10_neg_exp;
            frac &= 0x0fffffffffffffffllu;
            frac *= 10;
        }
        *out_frac_base10_neg_exp = frac_base10_neg_exp;
    }

    {
        /* Convert the fractional part to base 10. */
        unsigned frac_part = 0;
        for (int i = 0; frac && (i < NPF_MAX_FRACTION_DEC_DIGITS); ++i) {
            frac_part *= 10;
            frac_part += frac >> (NPF_FRACTION_BIN_DIGITS - 4);
            frac &= 0x0fffffffffffffffllu;
            frac *= 10;
        }
        *out_frac_part = frac_part;
    }
    return 1;
}

int npf__ftoa_rev(char *buf, float f, unsigned base,
                  npf__format_spec_conversion_case_t cc, int *out_frac_chars) {
    char const case_c = (cc == NPF_FMT_SPEC_CONV_CASE_LOWER) ? 'a' - 'A' : 0;

    if (f != f) {
        *buf++ = 'N' + case_c;
        *buf++ = 'A' + case_c;
        *buf++ = 'N' + case_c;
        return -3;
    }
    if (f == INFINITY) {
        *buf++ = 'F' + case_c;
        *buf++ = 'N' + case_c;
        *buf++ = 'I' + case_c;
        return -3;
    }

    uint64_t int_part, frac_part;
    int frac_base10_neg_exp;
    if (npf__fsplit_abs(f, &int_part, &frac_part, &frac_base10_neg_exp) == 0) {
        *buf++ = 'R' + case_c;
        *buf++ = 'O' + case_c;
        *buf++ = 'O' + case_c;
        return -3;
    }

    unsigned const base_c = (cc == NPF_FMT_SPEC_CONV_CASE_LOWER) ? 'a' : 'A';
    char *dst = buf;

    // write the fractional digits
    while (frac_part) {
        unsigned const d = frac_part % base;
        frac_part /= base;
        *dst++ = (d < 10) ? (char)('0' + d) : (char)(base_c + (d - 10));
    }
    // write the 0 digits between the . and the first fractional digit
    while (frac_base10_neg_exp-- > 0) {
        *dst++ = '0';
    }
    *out_frac_chars = (int)(dst - buf);
    // write the decimal point
    *dst++ = '.';
    // write the integer digits
    if (int_part == 0) {
        *dst++ = '0';
    } else {
        while (int_part) {
            unsigned const d = int_part % base;
            int_part /= base;
            *dst++ = (d < 10) ? (char)('0' + d) : (char)(base_c + (d - 10));
        }
    }
    return (int)(dst - buf);
}
#endif

#define NPF_PUTC(VAL)      \
    do {                   \
        pc((VAL), pc_ctx); \
        ++n;               \
    } while (0)

#define NPF_EXTRACT(MOD, CAST_TO, EXTRACT_AS)     \
    case NPF_FMT_SPEC_LEN_MOD_##MOD:              \
        val = (CAST_TO)va_arg(vlist, EXTRACT_AS); \
        break

#define NPF_WRITEBACK(MOD, TYPE)            \
    case NPF_FMT_SPEC_LEN_MOD_##MOD:        \
        *(va_arg(vlist, TYPE *)) = (TYPE)n; \
        break

int npf_vpprintf(npf_putc pc, void *pc_ctx, char const *format, va_list vlist) {
    npf__format_spec_t fs;
    char const *cur = format;
    int n = 0, sign = 0, i;

    while (*cur) {
        if (*cur != '%') {
            /* Non-format character, write directly */
            NPF_PUTC(*cur++);
        } else {
            /* Might be a format run, try to parse */
            int const fs_len = npf__parse_format_spec(cur, vlist, &fs);
            if (fs_len == 0) {
                /* Invalid format specifier, write and continue */
                NPF_PUTC(*cur++);
            } else {
                /* Format specifier, convert and write argument */
                char cbuf_mem[32], *cbuf = cbuf_mem, sign_c, pad_c;
                int cbuf_len = 0, field_pad = 0, prec_pad = 0;
#if NANOPRINTF_USE_FLOAT_FORMAT_SPECIFIERS
                int frac_chars = 0, inf_or_nan = 0;
#endif
                /* Convert the argument to string and point cbuf at it */
                switch (fs.conv_spec) {
                    case NPF_FMT_SPEC_CONV_PERCENT:
                        *cbuf = '%';
                        cbuf_len = 1;
                        break;

                    case NPF_FMT_SPEC_CONV_CHAR: /* 'c' */
                        *cbuf = (char)va_arg(vlist, int);
                        cbuf_len = 1;
                        break;

                    case NPF_FMT_SPEC_CONV_STRING: { /* 's' */
                        char *s = va_arg(vlist, char *);
                        /* don't bother loading cbuf, just point to s */
                        cbuf = s;
                        while (*s) ++s;
                        cbuf_len = (int)(s - cbuf);
                        if (fs.precision_type ==
                            NPF_FMT_SPEC_PRECISION_LITERAL) {
                            /* precision modifier truncates strings */
                            cbuf_len = NPF_MIN(fs.precision, cbuf_len);
                        }
                    } break;

                    case NPF_FMT_SPEC_CONV_SIGNED_INT: { /* 'i', 'd' */
                        npf__int_t val = 0;
                        switch (fs.length_modifier) {
                            NPF_EXTRACT(NONE, int, int);
                            NPF_EXTRACT(SHORT, short, int);
                            NPF_EXTRACT(LONG, long, long);
                            NPF_EXTRACT(LONG_DOUBLE, int, int);
                            NPF_EXTRACT(CHAR, char, int);
#if NANOPRINTF_USE_LARGE_FORMAT_SPECIFIERS == 1
                            NPF_EXTRACT(LARGE_LONG_LONG, long long, long long);
                            NPF_EXTRACT(LARGE_INTMAX, intmax_t, intmax_t);
                            NPF_EXTRACT(LARGE_SIZET, intmax_t, intmax_t);
                            NPF_EXTRACT(LARGE_PTRDIFFT, ptrdiff_t, ptrdiff_t);
#endif
                        }

                        sign = (val < 0) ? -1 : 1;
                        /* special case, if prec and value are 0, skip */
                        if (!val && !fs.precision &&
                            (fs.precision_type ==
                             NPF_FMT_SPEC_PRECISION_LITERAL)) {
                            cbuf_len = 0;
                        } else {
                            /* print the number into cbuf */
                            cbuf_len = npf__itoa_rev(cbuf, val);
                        }
                    } break;

                    case NPF_FMT_SPEC_CONV_OCTAL:          /* 'o' */
                    case NPF_FMT_SPEC_CONV_HEX_INT:        /* 'x', 'X' */
                    case NPF_FMT_SPEC_CONV_UNSIGNED_INT: { /* 'u' */
                        unsigned const base =
                            (fs.conv_spec == NPF_FMT_SPEC_CONV_OCTAL)
                                ? 8
                                : ((fs.conv_spec == NPF_FMT_SPEC_CONV_HEX_INT)
                                       ? 16
                                       : 10);
                        npf__uint_t val = 0;
                        switch (fs.length_modifier) {
                            NPF_EXTRACT(NONE, unsigned, unsigned);
                            NPF_EXTRACT(SHORT, unsigned short, unsigned);
                            NPF_EXTRACT(LONG, unsigned long, unsigned long);
                            NPF_EXTRACT(LONG_DOUBLE, unsigned, unsigned);
                            NPF_EXTRACT(CHAR, unsigned char, unsigned);
#if NANOPRINTF_USE_LARGE_FORMAT_SPECIFIERS == 1
                            NPF_EXTRACT(LARGE_LONG_LONG, unsigned long long,
                                        unsigned long long);
                            NPF_EXTRACT(LARGE_INTMAX, uintmax_t, uintmax_t);
                            NPF_EXTRACT(LARGE_SIZET, size_t, size_t);
                            NPF_EXTRACT(LARGE_PTRDIFFT, size_t, size_t);
#endif
                        }

                        /* octal special case, print a single '0' */
                        if ((fs.conv_spec == NPF_FMT_SPEC_CONV_OCTAL) && !val &&
                            !fs.precision && fs.alternative_form) {
                            fs.precision = 1;
                        }
                        /* special case, if prec and value are 0, skip */
                        if (!val && !fs.precision &&
                            (fs.precision_type ==
                             NPF_FMT_SPEC_PRECISION_LITERAL)) {
                            cbuf_len = 0;
                        } else {
                            /* print the number info cbuf */
                            cbuf_len = npf__utoa_rev(cbuf, val, base,
                                                     fs.conv_spec_case);
                        }
                        /* alt form adds '0' octal or '0x' hex prefix */
                        if (val && fs.alternative_form) {
                            if (fs.conv_spec == NPF_FMT_SPEC_CONV_OCTAL) {
                                cbuf[cbuf_len++] = '0';
                            } else if (fs.conv_spec ==
                                       NPF_FMT_SPEC_CONV_HEX_INT) {
                                cbuf[cbuf_len++] =
                                    (fs.conv_spec_case ==
                                     NPF_FMT_SPEC_CONV_CASE_LOWER)
                                        ? 'x'
                                        : 'X';
                                cbuf[cbuf_len++] = '0';
                            }
                        }
                    } break;

                    case NPF_FMT_SPEC_CONV_POINTER: /* 'p' */
                        cbuf_len = npf__ptoa_rev(cbuf, va_arg(vlist, void *));
                        break;

#if NANOPRINTF_USE_WRITEBACK_FORMAT_SPECIFIERS == 1
                    case NPF_FMT_SPEC_CONV_WRITEBACK: /* 'n' */
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
                        }
                        break;
#endif

#if NANOPRINTF_USE_FLOAT_FORMAT_SPECIFIERS == 1
                    case NPF_FMT_SPEC_CONV_FLOAT_DECIMAL:     /* 'f', 'F' */
                    case NPF_FMT_SPEC_CONV_FLOAT_EXPONENT:    /* 'e', 'E' */
                    case NPF_FMT_SPEC_CONV_FLOAT_DYNAMIC:     /* 'g', 'G' */
                    case NPF_FMT_SPEC_CONV_FLOAT_HEXPONENT: { /* 'a', 'A' */
                        float val;
                        if (fs.length_modifier ==
                            NPF_FMT_SPEC_LEN_MOD_LONG_DOUBLE) {
                            val = (float)va_arg(vlist, long double);
                        } else {
                            val = (float)va_arg(vlist, double);
                        }
                        sign = (val < 0) ? -1 : 1;
                        cbuf_len = npf__ftoa_rev(
                            cbuf, val, 10, fs.conv_spec_case, &frac_chars);
                        if (cbuf_len < 0) {
                            cbuf_len = -cbuf_len;
                            inf_or_nan = 1;
                        } else {
                            /* truncate lowest frac digits for precision */
                            if (frac_chars > fs.precision) {
                                cbuf += (frac_chars - fs.precision);
                                cbuf_len -= (frac_chars - fs.precision);
                            }
                        }
                    } break;
#endif
                }

                /* Compute the leading symbol (+, -, ' ') */
                sign_c = 0;
                if (sign == -1) {
                    sign_c = '-';
                } else if (sign == 1) {
                    if (fs.prepend_sign) {
                        sign_c = '+';
                    } else if (fs.prepend_space) {
                        sign_c = ' ';
                    }
                }

                /* Compute the field width pad character */
                pad_c = 0;
                if (fs.field_width_type == NPF_FMT_SPEC_FIELD_WIDTH_LITERAL) {
                    if (fs.leading_zero_pad) {
                        /* '0' flag is only legal with numeric types */
                        if ((fs.conv_spec != NPF_FMT_SPEC_CONV_STRING) &&
                            (fs.conv_spec != NPF_FMT_SPEC_CONV_CHAR) &&
                            (fs.conv_spec != NPF_FMT_SPEC_CONV_PERCENT)) {
                            pad_c = '0';
                        }
                    } else {
                        pad_c = ' ';
                    }
                }

                /* Compute the number of bytes to truncate or '0'-pad. */
                if (fs.conv_spec != NPF_FMT_SPEC_CONV_STRING) {
#if NANOPRINTF_USE_FLOAT_FORMAT_SPECIFIERS == 1
                    if (!inf_or_nan) {
                        /* float precision is after the decimal point */
                        int const precision_start =
                            (fs.conv_spec == NPF_FMT_SPEC_CONV_FLOAT_DECIMAL)
                                ? frac_chars
                                : cbuf_len;
                        prec_pad = NPF_MAX(0, fs.precision - precision_start);
                    }
#else
                    prec_pad = NPF_MAX(0, fs.precision - cbuf_len);
#endif
                }

                /* Given the full converted length, how many pad bytes? */
                field_pad =
                    NPF_MAX(0, fs.field_width - cbuf_len - !!sign_c - prec_pad);

                /* Apply right-justified field width if requested */
                if (!fs.left_justified && pad_c) {
                    /* If leading zeros pad, sign goes first. */
                    if ((sign_c == '-' || sign_c == '+') && pad_c == '0') {
                        NPF_PUTC(sign_c);
                        sign_c = 0;
                    }
                    while (field_pad-- > 0) {
                        NPF_PUTC(pad_c);
                    }
                }

                /* Write the converted payload */
                if (fs.conv_spec == NPF_FMT_SPEC_CONV_STRING) {
                    /* Strings are not reversed, put directly */
                    for (i = 0; i < cbuf_len; ++i) {
                        NPF_PUTC(cbuf[i]);
                    }
                } else {
                    if (sign_c) {
                        NPF_PUTC(sign_c);
                    }
#if NANOPRINTF_USE_FLOAT_FORMAT_SPECIFIERS == 1
                    if (fs.conv_spec != NPF_FMT_SPEC_CONV_FLOAT_DECIMAL) {
#endif
                        /* integral precision comes before the number. */
                        while (prec_pad-- > 0) {
                            NPF_PUTC('0');
                        }
#if NANOPRINTF_USE_FLOAT_FORMAT_SPECIFIERS == 1
                    } else {
                        /* if 0 precision, skip the fractional part and '.'
                           if 0 prec + alternative form, keep the '.' */
                        if (fs.precision == 0) {
                            cbuf += frac_chars + !fs.alternative_form;
                            cbuf_len -= frac_chars + !fs.alternative_form;
                        }
                    }
#endif
                    /* *toa_rev leaves payloads reversed */
                    while (cbuf_len-- > 0) {
                        NPF_PUTC(cbuf[cbuf_len]);
                    }

#if NANOPRINTF_USE_FLOAT_FORMAT_SPECIFIERS == 1
                    /* real precision comes after the number. */
                    if ((fs.conv_spec == NPF_FMT_SPEC_CONV_FLOAT_DECIMAL) &&
                        !inf_or_nan) {
                        while (prec_pad-- > 0) {
                            NPF_PUTC('0');
                        }
                    }
#endif
                }

                /* Apply left-justified field width if requested */
                if (fs.left_justified && pad_c) {
                    while (field_pad-- > 0) {
                        NPF_PUTC(pad_c);
                    }
                }

                cur += fs_len;
            }
        }
    }
    NPF_PUTC('\0');
    return n - 1;
}

#undef NPF_PUTC
#undef NPF_EXTRACT
#undef NPF_WRITEBACK

int npf_pprintf(npf_putc pc, void *pc_ctx, char const *format, ...) {
    va_list val;
    int rv;
    va_start(val, format);
    rv = npf_vpprintf(pc, pc_ctx, format, val);
    va_end(val);
    return rv;
}

int npf_snprintf(char *buffer, size_t bufsz, const char *format, ...) {
    va_list val;
    int rv;
    va_start(val, format);
    rv = npf_vsnprintf(buffer, bufsz, format, val);
    va_end(val);
    return rv;
}

int npf_vsnprintf(char *buffer, size_t bufsz, char const *format,
                  va_list vlist) {
    npf__bufputc_ctx_t bufputc_ctx;
    bufputc_ctx.dst = buffer;
    bufputc_ctx.len = bufsz;
    bufputc_ctx.cur = 0;
    if (buffer && bufsz) {
        buffer[bufsz - 1] = 0;
    }
    return npf_vpprintf(buffer ? npf__bufputc : npf__bufputc_nop, &bufputc_ctx,
                        format, vlist);
}

#ifdef __cplusplus
}
#endif

#endif /* NANOPRINTF_IMPLEMENTATION */
