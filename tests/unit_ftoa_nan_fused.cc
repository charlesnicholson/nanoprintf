// Same tests as unit_ftoa_nan.cc, against the fused %f conversion. That file goes
// through npf_snprintf rather than calling a conversion directly, so it is valid
// under either implementation and both are worth covering.
#define NANOPRINTF_USE_FLOAT_SCI_FORMAT_SPECIFIER 0
#define NANOPRINTF_USE_FLOAT_SHORTEST_FORMAT_SPECIFIER 0

#include "unit_ftoa_nan.cc"
