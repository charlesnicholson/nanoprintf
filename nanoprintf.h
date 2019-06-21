#ifndef NANOPRINTF_IMPLEMENTATION

#ifndef NANOPRINTF_H
#define NANOPRINTF_H

#include <stddef.h>
#include <stdarg.h>

#ifdef __cplusplus
extern "C" {
#endif

int npf_snprintf(char *buffer, size_t bufsz, const char *format, ...);
int npf_vsnprintf(char const *buffer, size_t bufsz, char const *format, va_list vlist);

#ifdef __cplusplus
}
#endif

#endif /* NANOPRINTF_H */

#else  /* NANOPRINTF_IMPLEMENTATION */

#undef NANOPRINTF_IMPLEMENTATION
#include "nanoprintf.h"
#define NANOPRINTF_IMPLEMENTATION

#include <stdio.h>

#ifdef __cplusplus
extern "C" {
#endif

int npf_snprintf(char *buffer, size_t bufsz, const char *format, ...) {
    (void)buffer;
    (void)bufsz;
    (void)format;
    printf("npf_snprintf\n");
    return 0;
}

#ifdef __cplusplus
}
#endif

#endif /* NANOPRINTF_IMPLEMENTATION */
