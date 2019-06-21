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

#include <stddef.h>
#include <stdarg.h>

#ifdef __cplusplus
extern "C" {
#endif

int npf_snprintf(char *buffer, size_t bufsz, const char *format, ...);
int npf_vsnprintf(char *buffer, size_t bufsz, char const *format, va_list vlist);

typedef struct npf_format_spec_t {
    /* optional flags */
    char left_justified   : 1; /* '-' */
    char prepend_sign     : 1; /* '+' */
    char prepend_space    : 1; /* ' ' */
    char alternative_form : 1; /* '#' */
    char zero_pad         : 1; /* '0' */

    /* field width */
    enum { NPF_FMT_SPEC_FIELD_WIDTH_NONE,
           NPF_FMT_SPEC_FIELD_WIDTH_LITERAL,
           NPF_FMT_SPEC_FIELD_WIDTH_STAR } field_width_type;
    int field_width;

    /* precision */
    enum { NPF_FMT_SPEC_PRECISION_NONE,
           NPF_FMT_SPEC_PRECISION_LITERAL,
           NPF_FMT_SPEC_PRECISION_STAR } precision_type;
    int precision;

    /* length modifier for specifying argument size */
    enum { NPF_FMT_SPEC_LENGTH_MOD_NONE,
           NPF_FMT_SPEC_LENGTH_MOD_SHORT,         /* 'h' */
           NPF_FMT_SPEC_LENGTH_MOD_LONG,          /* 'l' */
           NPF_FMT_SPEC_LENGTH_MOD_LONG_DOUBLE    /* 'L' */
#if NANOPRINTF_USE_C99_FORMAT_SPECIFIERS
           , NPF_FMT_SPEC_LENGTH_MOD_C99_CHAR,    /* 'hh' */
           NPF_FMT_SPEC_LENGTH_MOD_C99_LONG_LONG, /* 'll' */
           NPF_FMT_SPEC_LENGTH_MOD_C99_INTMAX,    /* 'j' */
           NPF_FMT_SPEC_LENGTH_MOD_C99_SIZET,     /* 'z' */
           NPF_FMT_SPEC_LENGTH_MOD_C99_PTRDIFFT   /* 't' */
#endif
    } length_modifier;

    /* conversion specifiers */
    enum { NPF_FMT_SPEC_CONV_PERCENT,         /* '%' */
           NPF_FMT_SPEC_CONV_CHAR,            /* 'c' */
           NPF_FMT_SPEC_CONV_STRING,          /* 's' */
           NPF_FMT_SPEC_CONV_SIGNED_INT,      /* 'i', 'd' */
           NPF_FMT_SPEC_CONV_OCTAL,           /* 'o' */
           NPF_FMT_SPEC_CONV_HEX_INT,         /* 'x', 'X' */
           NPF_FMT_SPEC_CONV_UNSIGNED_INT,    /* 'u' */
           NPF_FMT_SPEC_CONV_CHARS_WRITTEN,   /* 'n' */
           NPF_FMT_SPEC_CONV_POINTER          /* 'p' */
#if NANOPRINTF_USE_FLOAT_FORMAT_SPECIFIERS
           , NPF_FMT_SPEC_CONV_FLOAT_DECIMAL, /* 'f', 'F' */
           NPF_FMT_SPEC_CONV_FLOAT_EXPONENT,  /* 'e', 'E' */
           NPF_FMT_SPEC_CONV_FLOAT_DYNAMIC    /* 'g', 'G' */
#if NANOPRINTF_USE_C99_FORMAT_SPECIFIERS
           , NPF_FMT_SPEC_CONV_C99_FLOAT_HEX  /* 'a', 'A' */
#endif
#endif
    } conversion_specifier;
    enum { NPF_FMT_SPEC_CONV_CASE_NONE,
           NPF_FMT_SPEC_CONV_CASE_LOWER,
           NPF_FMT_SPEC_CONV_CASE_UPPER } conversion_specifier_case;
} npf_conv_spec_t;

#ifdef __cplusplus
}
#endif

#endif /* NANOPRINTF_H_INCLUDED */

#else  /* NANOPRINTF_IMPLEMENTATION */

#undef NANOPRINTF_IMPLEMENTATION
#include "nanoprintf.h"
#define NANOPRINTF_IMPLEMENTATION

#ifdef __cplusplus
extern "C" {
#endif

#ifdef __cplusplus
}
#endif

#endif /* NANOPRINTF_IMPLEMENTATION */
