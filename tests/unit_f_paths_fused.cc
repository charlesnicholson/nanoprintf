// The fused %f path: sci conversions off, so %f uses npf_ftoa_rev.
#define NANOPRINTF_USE_FLOAT_SCI_FORMAT_SPECIFIER 0
#define NANOPRINTF_USE_FLOAT_SHORTEST_FORMAT_SPECIFIER 0

#include "unit_nanoprintf.h"

#include "npf_f_paths.h"

int npf_f_fused(char *buf, unsigned len, char const *fmt, double v) {
  return npf_snprintf(buf, (size_t)len, fmt, v);
}
