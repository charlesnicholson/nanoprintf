#pragma once

// Each unit test file compiles nanoprintf privately for access to helper functions.
#define NANOPRINTF_USE_FIELD_WIDTH_FORMAT_SPECIFIERS 1
#define NANOPRINTF_USE_PRECISION_FORMAT_SPECIFIERS 1
#define NANOPRINTF_USE_FLOAT_FORMAT_SPECIFIERS 1
#define NANOPRINTF_USE_BINARY_FORMAT_SPECIFIERS 1
#define NANOPRINTF_USE_WRITEBACK_FORMAT_SPECIFIERS 1
#define NANOPRINTF_VISIBILITY_STATIC
#define NANOPRINTF_IMPLEMENTATION

#ifdef _MSC_VER
  #pragma warning(push)
  #pragma warning(disable:4464) // relative include uses ..
#endif

#include "../nanoprintf.h"

#ifdef _MSC_VER
  #pragma warning(pop)
#endif

#if NANOPRINTF_HAVE_GCC_WARNING_PRAGMAS
  #pragma GCC diagnostic push
  #if NANOPRINTF_CLANG
    #pragma GCC diagnostic ignored "-Wc++98-compat-pedantic"
    #ifndef __APPLE__
      #pragma GCC diagnostic ignored "-Wreserved-identifier"
    #endif
  #endif
#endif

#ifdef _MSC_VER
  #pragma warning(disable:4514) // unreferenced inline function removed
#endif

#include "doctest.h"

