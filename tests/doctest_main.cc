#ifdef _MSC_VER
  #pragma warning(disable:5246) // initialization of subobject needs braces
  #pragma warning(disable:5262) // implicit switch fall-through
  #pragma warning(disable:5264) // unused const variable
#endif

#if defined(__clang__) && !defined(__APPLE__)
  #pragma GCC diagnostic ignored "-Wunsafe-buffer-usage"
#endif

#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include "doctest.h"
