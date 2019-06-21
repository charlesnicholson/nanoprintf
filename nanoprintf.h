// https://github.com/nothings/stb/blob/master/docs/stb_howto.txt
#ifndef NANOPRINTF_IMPLEMENTATION

#ifndef NANOPRINTF_H_INCLUDED
#define NANOPRINTF_H_INCLUDED

#ifndef NANOPRINTF_USE_C99_FORMAT_SPECIFIERS
#error NANOPRINTF_USE_C99_FORMAT_SPECIFIERS must be #defined to 0 or 1
#endif

#ifndef NANOPRINTF_USE_FLOAT_FORMAT_SPECIFIERS
#error NANOPRINTF_USE_FLOAT_FORMAT_SPECIFIERS must be #defined to 0 or 1
#endif

#include <stdarg.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/* Public API */

int npf_snprintf(char *buffer, size_t bufsz, const char *format, ...);
int npf_vsnprintf(char *buffer, size_t bufsz, char const *format,
                  va_list vlist);

/* Internal */

typedef enum {
    NPF_FMT_SPEC_FIELD_WIDTH_NONE,
    NPF_FMT_SPEC_FIELD_WIDTH_LITERAL,
    NPF_FMT_SPEC_FIELD_WIDTH_STAR
} npf__format_spec_field_width_t;

typedef enum {
    NPF_FMT_SPEC_PRECISION_NONE,
    NPF_FMT_SPEC_PRECISION_LITERAL,
    NPF_FMT_SPEC_PRECISION_STAR
} npf__format_spec_precision_t;

typedef enum {
    NPF_FMT_SPEC_LENGTH_MOD_NONE,
    NPF_FMT_SPEC_LENGTH_MOD_SHORT,      /* 'h' */
    NPF_FMT_SPEC_LENGTH_MOD_LONG,       /* 'l' */
    NPF_FMT_SPEC_LENGTH_MOD_LONG_DOUBLE /* 'L' */
#if NANOPRINTF_USE_C99_FORMAT_SPECIFIERS
    ,
    NPF_FMT_SPEC_LENGTH_MOD_C99_CHAR,      /* 'hh' */
    NPF_FMT_SPEC_LENGTH_MOD_C99_LONG_LONG, /* 'll' */
    NPF_FMT_SPEC_LENGTH_MOD_C99_INTMAX,    /* 'j' */
    NPF_FMT_SPEC_LENGTH_MOD_C99_SIZET,     /* 'z' */
    NPF_FMT_SPEC_LENGTH_MOD_C99_PTRDIFFT   /* 't' */
#endif
} npf__format_spec_length_modifier_t;

typedef enum {
    NPF_FMT_SPEC_CONV_PERCENT,       /* '%' */
    NPF_FMT_SPEC_CONV_CHAR,          /* 'c' */
    NPF_FMT_SPEC_CONV_STRING,        /* 's' */
    NPF_FMT_SPEC_CONV_SIGNED_INT,    /* 'i', 'd' */
    NPF_FMT_SPEC_CONV_OCTAL,         /* 'o' */
    NPF_FMT_SPEC_CONV_HEX_INT,       /* 'x', 'X' */
    NPF_FMT_SPEC_CONV_UNSIGNED_INT,  /* 'u' */
    NPF_FMT_SPEC_CONV_CHARS_WRITTEN, /* 'n' */
    NPF_FMT_SPEC_CONV_POINTER        /* 'p' */
#if NANOPRINTF_USE_FLOAT_FORMAT_SPECIFIERS
    ,
    NPF_FMT_SPEC_CONV_FLOAT_DECIMAL,  /* 'f', 'F' */
    NPF_FMT_SPEC_CONV_FLOAT_EXPONENT, /* 'e', 'E' */
    NPF_FMT_SPEC_CONV_FLOAT_DYNAMIC   /* 'g', 'G' */
#if NANOPRINTF_USE_C99_FORMAT_SPECIFIERS
    ,
    NPF_FMT_SPEC_CONV_C99_FLOAT_HEX /* 'a', 'A' */
#endif
#endif
} npf__format_spec_conversion_t;

typedef enum {
    NPF_FMT_SPEC_CONV_CASE_NONE,
    NPF_FMT_SPEC_CONV_CASE_LOWER,
    NPF_FMT_SPEC_CONV_CASE_UPPER
} npf__format_spec_conversion_case_t;

typedef struct {
    /* optional flags */
    char left_justified : 1;   /* '-' */
    char prepend_sign : 1;     /* '+' */
    char prepend_space : 1;    /* ' ' */
    char alternative_form : 1; /* '#' */
    char zero_pad : 1;         /* '0' */

    /* field width */
    npf__format_spec_field_width_t field_width_type;
    int field_width;

    /* precision */
    npf__format_spec_precision_t precision_type;
    int precision;

    /* length modifier for specifying argument size */
    npf__format_spec_length_modifier_t length_modifier;

    /* conversion specifiers */
    npf__format_spec_conversion_t conversion_specifier;
    npf__format_spec_conversion_case_t conversion_specifier_case;
} npf__format_spec_t;

int npf__parse_format_spec(char const *format, npf__format_spec_t *out_spec);

#ifdef __cplusplus
}
#endif

#endif /* NANOPRINTF_H_INCLUDED */

#else /* NANOPRINTF_IMPLEMENTATION */

#undef NANOPRINTF_IMPLEMENTATION
#include "nanoprintf.h"
#define NANOPRINTF_IMPLEMENTATION

#ifdef __cplusplus
extern "C" {
#endif

int npf__parse_format_spec(char const *format, npf__format_spec_t *out_spec) {
    char const *cur = format;

    if (*cur++ != '%') {
        return 0;
    }

    switch (*cur++) {
        case '%':
            out_spec->conversion_specifier = NPF_FMT_SPEC_CONV_PERCENT;
            break;
        case 'c':
            out_spec->conversion_specifier = NPF_FMT_SPEC_CONV_CHAR;
            break;
        case 's':
            out_spec->conversion_specifier = NPF_FMT_SPEC_CONV_STRING;
            break;
        case 'i':
            out_spec->conversion_specifier = NPF_FMT_SPEC_CONV_SIGNED_INT;
            break;
        case 'd':
            out_spec->conversion_specifier = NPF_FMT_SPEC_CONV_SIGNED_INT;
            break;
        case 'o':
            out_spec->conversion_specifier = NPF_FMT_SPEC_CONV_OCTAL;
            break;
        case 'x':
            out_spec->conversion_specifier = NPF_FMT_SPEC_CONV_HEX_INT;
            out_spec->conversion_specifier_case = NPF_FMT_SPEC_CONV_CASE_LOWER;
            break;
        case 'X':
            out_spec->conversion_specifier = NPF_FMT_SPEC_CONV_HEX_INT;
            out_spec->conversion_specifier_case = NPF_FMT_SPEC_CONV_CASE_UPPER;
            break;
        case 'u':
            out_spec->conversion_specifier = NPF_FMT_SPEC_CONV_UNSIGNED_INT;
            break;
#if NANOPRINTF_USE_FLOAT_FORMAT_SPECIFIERS
        case 'f':
            out_spec->conversion_specifier = NPF_FMT_SPEC_CONV_FLOAT_DECIMAL;
            out_spec->conversion_specifier_case = NPF_FMT_SPEC_CONV_CASE_LOWER;
            break;
        case 'F':
            out_spec->conversion_specifier = NPF_FMT_SPEC_CONV_FLOAT_DECIMAL;
            out_spec->conversion_specifier_case = NPF_FMT_SPEC_CONV_CASE_UPPER;
            break;
        case 'e':
            out_spec->conversion_specifier = NPF_FMT_SPEC_CONV_FLOAT_EXPONENT;
            out_spec->conversion_specifier_case = NPF_FMT_SPEC_CONV_CASE_LOWER;
            break;
        case 'E':
            out_spec->conversion_specifier = NPF_FMT_SPEC_CONV_FLOAT_EXPONENT;
            out_spec->conversion_specifier_case = NPF_FMT_SPEC_CONV_CASE_UPPER;
            break;
#if NANOPRINTF_USE_C99_FORMAT_SPECIFIERS
        case 'a':
            out_spec->conversion_specifier = NPF_FMT_SPEC_CONV_C99_FLOAT_HEX;
            out_spec->conversion_specifier_case = NPF_FMT_SPEC_CONV_CASE_LOWER;
            break;
        case 'A':
            out_spec->conversion_specifier = NPF_FMT_SPEC_CONV_C99_FLOAT_HEX;
            out_spec->conversion_specifier_case = NPF_FMT_SPEC_CONV_CASE_UPPER;
            break;
#endif
        case 'g':
            out_spec->conversion_specifier = NPF_FMT_SPEC_CONV_FLOAT_DYNAMIC;
            out_spec->conversion_specifier_case = NPF_FMT_SPEC_CONV_CASE_LOWER;
            break;
        case 'G':
            out_spec->conversion_specifier = NPF_FMT_SPEC_CONV_FLOAT_DYNAMIC;
            out_spec->conversion_specifier_case = NPF_FMT_SPEC_CONV_CASE_UPPER;
            break;
#endif
        case 'n':
            out_spec->conversion_specifier = NPF_FMT_SPEC_CONV_CHARS_WRITTEN;
            break;
        case 'p':
            out_spec->conversion_specifier = NPF_FMT_SPEC_CONV_POINTER;
            break;
        default:
            return 0;
    }
    return (int)(cur - format);
}

#ifdef __cplusplus
}
#endif

#endif /* NANOPRINTF_IMPLEMENTATION */
