#pragma once

// Suppress warnings from doctest.h and test code for the rest of the translation unit.

#ifdef _MSC_VER
  #pragma warning(disable:4619)  // there is no warning number 'number'
  #pragma warning(disable:4365)  // signed/unsigned mismatch
  #pragma warning(disable:4505)  // unreferenced local function removed
  #pragma warning(disable:5039)  // potentially throwing function passed to extern C
  #pragma warning(disable:5246)  // subobject initialization should be wrapped in braces
  #pragma warning(disable:5262)  // implicit switch fall-through
  #pragma warning(disable:5264)  // const variable is not used
  #pragma warning(disable:26451) // casting operator result to wider type
#endif

#ifdef __clang__
  #pragma GCC diagnostic ignored "-Wc++98-compat-pedantic"
  #pragma GCC diagnostic ignored "-Wold-style-cast"
  #ifndef __APPLE__
    #pragma GCC diagnostic ignored "-Wreserved-identifier"
    #pragma GCC diagnostic ignored "-Wunsafe-buffer-usage"
    #pragma GCC diagnostic ignored "-W#warnings"
  #endif
#endif

#ifdef __clang__
  #pragma clang diagnostic push
  #pragma clang diagnostic ignored "-Wunknown-warning-option"
  #pragma clang diagnostic ignored "-Wnrvo"
  #pragma clang diagnostic ignored "-Wthread-safety-negative"
#endif

#include "doctest.h"

#ifdef __clang__
  #pragma clang diagnostic pop
#endif
