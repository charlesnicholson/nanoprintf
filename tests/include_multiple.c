// Confirm that nanoprintf can be included and then implemented.

#ifdef _MSC_VER
  #pragma warning(disable:4464) // relative include uses ..
#endif

// Include the interface multiple times to make sure it guards against itself.
#include "../nanoprintf.h"
#include "../nanoprintf.h"
#include "../nanoprintf.h"
#include "../nanoprintf.h"
#include "../nanoprintf.h"

#define NANOPRINTF_USE_FIELD_WIDTH_FORMAT_SPECIFIERS 1
#define NANOPRINTF_USE_PRECISION_FORMAT_SPECIFIERS 1
#define NANOPRINTF_USE_FLOAT_FORMAT_SPECIFIERS 1
#define NANOPRINTF_USE_LARGE_FORMAT_SPECIFIERS 1
#define NANOPRINTF_USE_BINARY_FORMAT_SPECIFIERS 1
#define NANOPRINTF_USE_WRITEBACK_FORMAT_SPECIFIERS 1
#define NANOPRINTF_IMPLEMENTATION

// Include the implementation multiple times to make sure it guards against itself.
#include "../nanoprintf.h"
#include "../nanoprintf.h"
#include "../nanoprintf.h"
#include "../nanoprintf.h"
#include "../nanoprintf.h"

#include <stdio.h>

int main(int c, char const *v[]) {
  (void)c;
  (void)v;
  char buf[64];
  npf_snprintf(buf, sizeof(buf), "%s", "hello");
  printf("%s\n", buf);
}
