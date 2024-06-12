#pragma once

#define NANOPRINTF_USE_FIELD_WIDTH_FORMAT_SPECIFIERS 1
#define NANOPRINTF_USE_PRECISION_FORMAT_SPECIFIERS 1
#define NANOPRINTF_USE_FLOAT_FORMAT_SPECIFIERS 1
#define NANOPRINTF_USE_BINARY_FORMAT_SPECIFIERS 1
#define NANOPRINTF_USE_WRITEBACK_FORMAT_SPECIFIERS 1

// Each unit test file compiles nanoprintf privately for access to helper functions.
#define NANOPRINTF_VISIBILITY_STATIC
#define NANOPRINTF_IMPLEMENTATION

#ifdef _MSC_VER
  #pragma warning(disable:4619) // there is no warning number 'number'
  // C4619 has to be disabled first!
  #pragma warning(disable:4464) // relative include path contains '..'
#endif

#include "../nanoprintf.h"

#if NANOPRINTF_HAVE_GCC_WARNING_PRAGMAS
  #pragma GCC diagnostic push
  #if NANOPRINTF_CLANG
    #pragma GCC diagnostic ignored "-Wc++98-compat-pedantic"
    #ifndef __APPLE__
      #pragma GCC diagnostic ignored "-Wreserved-identifier"
      #pragma GCC diagnostic ignored "-Wunsafe-buffer-usage"
    #endif
  #endif
#endif

#ifdef _MSC_VER
  #pragma warning(disable:4365) // type conversion, signed/unsigned mismatch
  #pragma warning(disable:4505) // unreferenced local function has been removed
  #pragma warning(disable:4514) // unreferenced inline function has been removed
  #pragma warning(disable:4710) // function not inlined
  #pragma warning(disable:4711) // function selected for inline expansion
  #pragma warning(disable:5039) // potentially throwing function passed to extern C function
  #pragma warning(disable:5264) // const variable is not used
  #pragma warning(disable:26451) // casting the operator result value to a wider type
#endif

#include "doctest.h"
