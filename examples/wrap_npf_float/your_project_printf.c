#include "your_project_printf.h"

// Compile nanoprintf in this translation unit.
#define NANOPRINTF_IMPLEMENTATION
#include "../../nanoprintf.h"

int your_project_snprintf_impl(char *buffer, size_t bufsz, char const *fmt, ...) {
    va_list val;
    va_start(val, fmt);
    // npf_vsnprintf works here because float args in the va_list are already
    // wrapped in npf_float_t by the NPF_MAP_ARGS macro at the call site.
    int const rv = npf_vsnprintf(buffer, bufsz, fmt, val);
    va_end(val);
    return rv;
}
