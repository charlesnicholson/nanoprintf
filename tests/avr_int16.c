/*
  Conformance probe for a 16-bit int, run on an emulated AVR by avr_int16.py.

  Every other test target in this repo has a 32-bit int, so nothing else here
  exercises the width/precision arithmetic against an INT_MAX of 32767. The
  expectations are the ones that hold at any int width: a length is never
  negative, a width that fits is honored exactly, and one that does not is
  pinned to NPF_FMT_NUM_MAX. Floats are off because avr-libc has no 64-bit
  double, and the arithmetic under test is in the format-spec parser.
*/

#define NANOPRINTF_IMPLEMENTATION
#include "nanoprintf.h"

#include <avr/io.h>
#include <limits.h>
#include <string.h>

static void up(char c) { while (!(UCSR0A & (1 << UDRE0))) {} UDR0 = (unsigned char)c; }
static void us(char const *s) { while (*s) { up(*s++); } }

static void ud(int v) {
  char b[8];
  int i = 0;
  unsigned u = (v < 0) ? (0u - (unsigned)v) : (unsigned)v;
  if (v < 0) { up('-'); }
  do { b[i++] = (char)('0' + (u % 10)); u /= 10; } while (u);
  while (i) { up(b[--i]); }
}

static int pass, fail;
static char buf[64];

static void chk(char const *tag, int got, int want) {
  if (got == want) { ++pass; return; }
  ++fail;
  us("FAIL "); us(tag); us(" got="); ud(got); us(" want="); ud(want); us("\r\n");
}

static void chks(char const *tag, char const *want) {
  if (!strcmp(buf, want)) { ++pass; return; }
  ++fail;
  us("FAIL "); us(tag); us(" got=\""); us(buf); us("\" want=\""); us(want); us("\"\r\n");
}

int main(void) {
  UBRR0H = 0; UBRR0L = 8;
  UCSR0B = (1 << TXEN0);
  UCSR0C = (1 << UCSZ01) | (1 << UCSZ00);

  /* If these two fail, nothing below is testing what it claims to. */
  chk("int_is_16", (int)sizeof(int), 2);
  chk("INT_MAX_is_32767", INT_MAX, 32767);

  /* conversions at the promoted-argument width */
  npf_snprintf(buf, sizeof buf, "%d", INT_MIN); chks("d_INT_MIN", "-32768");
  npf_snprintf(buf, sizeof buf, "%d", INT_MAX); chks("d_INT_MAX", "32767");
  npf_snprintf(buf, sizeof buf, "%u", UINT_MAX); chks("u_UINT_MAX", "65535");
  npf_snprintf(buf, sizeof buf, "%x", UINT_MAX); chks("x_UINT_MAX", "ffff");
  npf_snprintf(buf, sizeof buf, "%ld", -2147483647L - 1); chks("ld_LONG_MIN", "-2147483648");
  npf_snprintf(buf, sizeof buf, "%hhd", 300); chks("hhd_300", "44");

  /* Unbounded digit runs in the format string. The accumulator must not wrap
     into a negative width, and the ceiling must survive the trip. */
  chk("w_INT_MAX_digits", npf_snprintf(buf, sizeof buf, "%2147483647d", 7) > 0, 1);
  chk("w_INT_MAX_digits_cap",
      npf_snprintf(buf, sizeof buf, "%2147483647d", 7) <= NPF_FMT_NUM_MAX, 1);
  chk("w_11_digits", npf_snprintf(buf, sizeof buf, "%99999999999d", 7) > 0, 1);
  chk("w_11_digits_cap",
      npf_snprintf(buf, sizeof buf, "%99999999999d", 7) <= NPF_FMT_NUM_MAX, 1);
  chk("p_11_digits", npf_snprintf(buf, sizeof buf, "%.99999999999d", 7) > 0, 1);
  chk("p_11_digits_cap",
      npf_snprintf(buf, sizeof buf, "%.99999999999d", 7) <= NPF_FMT_NUM_MAX, 1);

  /* A width that fits is honored exactly, not pinned to the ceiling. */
  chk("w_100", npf_snprintf(buf, sizeof buf, "%100d", 7), 100);
  chk("w_3000", npf_snprintf(buf, sizeof buf, "%3000d", 7), 3000);
  chk("star_3000", npf_snprintf(buf, sizeof buf, "%*d", 3000, 7), 3000);
  chk("star_neg_3000", npf_snprintf(buf, sizeof buf, "%*d", -3000, 7), 3000);
  chk("star_8000", npf_snprintf(buf, sizeof buf, "%*d", 8000, 7), 8000);
  chk("star_0", npf_snprintf(buf, sizeof buf, "%*d", 0, 7), 1);

  /* Star arguments at the edges. INT_MIN has no positive int counterpart. */
  chk("star_INT_MIN", npf_snprintf(buf, sizeof buf, "%*d", INT_MIN, 7), NPF_FMT_NUM_MAX);
  chk("star_INT_MAX", npf_snprintf(buf, sizeof buf, "%*d", INT_MAX, 7), NPF_FMT_NUM_MAX);
  chk("star_prec_INT_MIN", npf_snprintf(buf, sizeof buf, "%.*d", INT_MIN, 7), 1);
  chk("star_prec_INT_MAX", npf_snprintf(buf, sizeof buf, "%.*d", INT_MAX, 7), NPF_FMT_NUM_MAX);

  /* A width and a precision both land in one length, which must not wrap. */
  chk("star_w_and_prec", npf_snprintf(buf, sizeof buf, "%*.*d", INT_MAX, INT_MAX, 7),
      NPF_FMT_NUM_MAX);

  /* The 4095 that C11 5.2.4.1 requires an implementation to accept. */
  chk("w_4095", npf_snprintf(buf, sizeof buf, "%4095d", 7), 4095);
  chk("p_4095", npf_snprintf(buf, sizeof buf, "%.4095d", 7), 4095);

  us(fail ? "RESULT FAIL " : "RESULT PASS ");
  ud(pass); us("/"); ud(pass + fail); us("\r\nDONE\r\n");
  for (;;) {}
}
