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

// Each unit test file compiles nanoprintf privately for access to helper functions.
#define NANOPRINTF_VISIBILITY_STATIC
#define NANOPRINTF_IMPLEMENTATION

#include "../nanoprintf.h"

#include "npf_doctest.h"
