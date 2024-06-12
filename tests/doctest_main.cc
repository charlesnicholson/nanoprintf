#ifdef _MSC_VER
  #pragma warning(disable:4619) // there is no warning number 'number'
  // C4619 has to be disabled first!
  #pragma warning(disable:5246) // initialization of a subobject should be wrapped in braces
  #pragma warning(disable:5262) // implicit switch fall-through
  #pragma warning(disable:5264) // const variable is not used
#endif

#if defined(__clang__) && !defined(__APPLE__)
  #pragma GCC diagnostic ignored "-Wunsafe-buffer-usage"
#endif

#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include "doctest.h"
