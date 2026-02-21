#pragma once

// Wrapper around the third-party doctest.h that suppresses warnings it triggers.

#ifdef _MSC_VER
  #pragma warning(push)
  #pragma warning(disable:4619)  // there is no warning number 'number'
  #pragma warning(disable:4365)  // signed/unsigned mismatch
  #pragma warning(disable:4505)  // unreferenced local function removed
  #pragma warning(disable:5039)  // potentially throwing function passed to extern C
  #pragma warning(disable:5246)  // subobject initialization should be wrapped in braces
  #pragma warning(disable:5262)  // implicit switch fall-through
  #pragma warning(disable:5264)  // const variable is not used
  #pragma warning(disable:26451) // casting operator result to wider type
#endif

#if defined(__clang__) || (defined(__GNUC__) && (__GNUC__ > 4 || (__GNUC__ == 4 && __GNUC_MINOR__ > 6)))
  #pragma GCC diagnostic push
  #ifdef __clang__
    #pragma GCC diagnostic ignored "-Wc++98-compat-pedantic"
    #pragma GCC diagnostic ignored "-Wold-style-cast"
    #ifndef __APPLE__
      #pragma GCC diagnostic ignored "-Wreserved-identifier"
      #pragma GCC diagnostic ignored "-Wunsafe-buffer-usage"
      #pragma GCC diagnostic ignored "-W#warnings"
    #endif
  #endif
#endif

#include "doctest.h"

#if defined(__clang__) || (defined(__GNUC__) && (__GNUC__ > 4 || (__GNUC__ == 4 && __GNUC_MINOR__ > 6)))
  #pragma GCC diagnostic pop
#endif

#ifdef _MSC_VER
  #pragma warning(pop)
#endif
