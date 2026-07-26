// The unified %f path: sci conversions on, so %f shares npf_etoa_rev.
#define NANOPRINTF_USE_FLOAT_SCI_FORMAT_SPECIFIER 1
#define NANOPRINTF_USE_FLOAT_SHORTEST_FORMAT_SPECIFIER 1

#include "unit_nanoprintf.h"

#include "npf_f_paths.h"

int npf_f_unified(char *buf, unsigned len, char const *fmt, double v) {
  return npf_snprintf(buf, (size_t)len, fmt, v);
}
