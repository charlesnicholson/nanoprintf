// unit_ftoa_rev.cc against the fused %f conversion. The shared one is covered
// by that file directly; both implementations run the same cases.
#define NANOPRINTF_USE_FLOAT_SCI_FORMAT_SPECIFIER 0
#define NANOPRINTF_USE_FLOAT_SHORTEST_FORMAT_SPECIFIER 0

#include "unit_ftoa_rev.cc"
