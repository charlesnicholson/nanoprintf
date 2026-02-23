/*
  Wrapping nanoprintf with single-precision float mode.

  In normal (double-precision) mode, you can write a variadic wrapper function
  that forwards to npf_vsnprintf via va_list (see ../wrap_npf). This works
  because C's variadic calling convention promotes float to double, and
  npf_vsnprintf extracts doubles from the va_list.

  In single-precision mode, float/double arguments must be wrapped in a struct
  at the call site to prevent float-to-double promotion. Once arguments cross a
  variadic function boundary (va_start), they've already been promoted and can't
  be un-promoted. The solution: a thin macro wraps the args with NPF_MAP_ARGS,
  then calls a real variadic function that receives already-wrapped values.
*/

#pragma once

#define NANOPRINTF_USE_FIELD_WIDTH_FORMAT_SPECIFIERS 0
#define NANOPRINTF_USE_PRECISION_FORMAT_SPECIFIERS 1
#define NANOPRINTF_USE_FLOAT_FORMAT_SPECIFIERS 1
#define NANOPRINTF_FLOAT_SINGLE_PRECISION 1
#define NANOPRINTF_USE_SMALL_FORMAT_SPECIFIERS 0
#define NANOPRINTF_USE_LARGE_FORMAT_SPECIFIERS 0
#define NANOPRINTF_USE_BINARY_FORMAT_SPECIFIERS 0
#define NANOPRINTF_USE_WRITEBACK_FORMAT_SPECIFIERS 0

#include "../../nanoprintf.h"

/* The macro wraps float/double args before they cross the variadic boundary.
   fmt is captured inside __VA_ARGS__ so no ##__VA_ARGS__ GNU extension is needed. */
#define your_project_snprintf(buf, sz, ...) \
  your_project_snprintf_((buf), (sz), NPF_MAP_ARGS(__VA_ARGS__))

/* The real function receives already-wrapped args and forwards via va_list. */
int your_project_snprintf_(char *buffer, size_t bufsz, const char *fmt, ...);
