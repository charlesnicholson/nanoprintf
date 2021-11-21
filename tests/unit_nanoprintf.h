#pragma once

#define NANOPRINTF_EXPOSE_INTERNALS
#define NANOPRINTF_USE_FIELD_WIDTH_FORMAT_SPECIFIERS 1
#define NANOPRINTF_USE_PRECISION_FORMAT_SPECIFIERS 1
#define NANOPRINTF_USE_FLOAT_FORMAT_SPECIFIERS 1
#define NANOPRINTF_USE_WRITEBACK_FORMAT_SPECIFIERS 1

/*
    NANOPRINTF_USE_LARGE_FORMAT_SPECIFIERS comes from CMake. The
    definitions of npf_int_t and npf_uint_t are defined differently based
    on the value of that flag, and tests should pass against both flavors.

    The separate conformance tests confirm that nanoprintf behaves well with
    different configuration flags; this is mostly to test [iu]toa_rev with
    different sized fields.
*/

#include "../nanoprintf.h"
