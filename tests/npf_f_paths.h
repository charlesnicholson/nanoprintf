#pragma once

/* There are two %f implementations, selected by whether the sci conversions are
   compiled in, and nanoprintf.h can only be included with one configuration per
   translation unit. So each path gets its own TU exposing a wrapper, and
   unit_f_paths.cc compares them against each other.

   npf_ftoa_rev fuses digit generation with placement, which is the smallest way to
   do %f alone. npf_etoa_rev generates digits plus a decimal exponent and lays out
   afterwards, which is what %e and %g need; when either of those is enabled, %f
   shares it rather than paying for a second generator. The two must agree. */

int npf_f_fused(char *buf, unsigned len, char const *fmt, double v);
int npf_f_unified(char *buf, unsigned len, char const *fmt, double v);
