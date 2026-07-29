#pragma once

#define NANOPRINTF_USE_SMALL_FORMAT_SPECIFIERS 1
#define NANOPRINTF_USE_FIELD_WIDTH_FORMAT_SPECIFIERS 1
#define NANOPRINTF_USE_PRECISION_FORMAT_SPECIFIERS 1
#define NANOPRINTF_USE_FLOAT_FORMAT_SPECIFIERS 1
#define NANOPRINTF_USE_FLOAT_HEX_FORMAT_SPECIFIER 1
// Overridable: npf_ftoa_rev only exists when the sci conversions are compiled out,
// so unit_ftoa_rev.cc turns them off to reach it.
#ifndef NANOPRINTF_USE_FLOAT_SCI_FORMAT_SPECIFIER
  #define NANOPRINTF_USE_FLOAT_SCI_FORMAT_SPECIFIER 1
#endif
#ifndef NANOPRINTF_USE_FLOAT_SHORTEST_FORMAT_SPECIFIER
  #define NANOPRINTF_USE_FLOAT_SHORTEST_FORMAT_SPECIFIER 1
#endif
#define NANOPRINTF_USE_BINARY_FORMAT_SPECIFIERS 1
#define NANOPRINTF_USE_WRITEBACK_FORMAT_SPECIFIERS 1
#define NANOPRINTF_USE_ALT_FORM_FLAG 1
#define NANOPRINTF_USE_FIXED_WIDTH_FORMAT_SPECIFIERS 1

/* Which %f implementation this TU got. Tests that go through npf_snprintf rather
   than calling a conversion directly are compiled under both, so their doctest
   names carry the tag to stay distinct. */
#if (NANOPRINTF_USE_FLOAT_SCI_FORMAT_SPECIFIER == 1) || \
    (NANOPRINTF_USE_FLOAT_SHORTEST_FORMAT_SPECIFIER == 1)
  #define NPF_FLOAT_PATH " [shared %f]"
#else
  #define NPF_FLOAT_PATH " [fused %f]"
#endif

// Each unit test file compiles nanoprintf privately for access to helper functions.
#define NANOPRINTF_VISIBILITY_STATIC
#define NANOPRINTF_IMPLEMENTATION

#include "../nanoprintf.h"

#include "npf_doctest.h"
