"""Regenerate the %e/%g test data, verifying it against the host system printf.

Writes two files next to this script:

  unit_eg.inc         the exhaustive value/precision table used by unit_etoa_rev.cc
  conformance_eg.inc  the curated, flag-guarded block pasted into conformance.c

Only unit_eg.inc is consumed directly. conformance_eg.inc is a scratch output:
paste it into the "float scientific and shortest" section of conformance.c when
the curated case list here changes.

Every expected string is produced by nanoprintf and then checked against the
system printf in both double and single-precision modes. A case that nanoprintf
and the system disagree on is dropped and reported, unless the value's exact
decimal expansion is a half-way tie, which nanoprintf deliberately rounds away
from zero exactly as its %f does. Rows that need a double-precision build go into
a separate table so single-precision mode can skip them.

Run from anywhere: python3 tests/gen_eg_tests.py
"""

from __future__ import annotations

import decimal
import pathlib
import subprocess
import sys
import tempfile

NPF = pathlib.Path(__file__).resolve().parent.parent
HERE = pathlib.Path(__file__).resolve().parent

Case = tuple[str, str, str]  # (group, format, C value expression)

# ------------------------------------------------------------- curated (matrix)

CONV = ["e", "E", "g", "G"]


def curated() -> list[Case]:
    out: list[Case] = []

    def add(group: str, fmt: str, val: str) -> None:
        out.append((group, fmt, val))

    # zero, negative zero, one, and the sign/space flags on each spelling
    for v in ["0.0", "-0.0", "1.0", "-1.0", "1.5", "-1.5"]:
        for c in CONV:
            add("basic", f"%{c}", v)
            add("basic", f"%+{c}", v)
            add("basic", f"% {c}", v)
            add("basic", f"%.0{c}", v)
            add("basic", f"%.1{c}", v)
            add("basic", f"%.3{c}", v)

    # alt form: keeps the point and, for %g, the trailing zeros
    for v in ["0.0", "1.0", "100.0", "1.5", "1e5", "1e-5", "0.0001"]:
        for c in CONV:
            add("alt-form", f"%#{c}", v)
            add("alt-form", f"%#.0{c}", v)
            add("alt-form", f"%#.1{c}", v)
            add("alt-form", f"%#.4{c}", v)

    # the %g style boundary, both sides, and trailing-zero removal
    for v in ["1e-5", "0.0001", "0.001", "0.1", "1.0", "10.0", "100.0", "1000.0",
              "100000.0", "1000000.0", "1e7", "999999.0"]:
        for p in (1, 2, 4, 6):
            add("g-style", f"%.{p}g", v)
    for v in ["100.0", "1.5", "1.25", "1e5", "120.0", "0.5"]:
        for p in (1, 3, 6):
            add("g-strip", f"%.{p}g", v)
            add("g-strip", f"%#.{p}g", v)

    # exponent field: two digits minimum, three when the exponent needs them
    for v in ["1.0", "10.0", "0.1", "1e9", "1e-9", "1e10", "1e-10", "1e99", "1e-99",
              "1e100", "1e-100"]:
        add("exponent", "%.3e", v)
        add("exponent", "%.3E", v)
        add("exponent", "%.2g", v)

    # rounding carries that move the exponent
    for v in ["9.5", "0.95", "9.9999", "0.99999", "0.09999"]:
        for f in ["%.0e", "%.1e", "%.3e", "%.0g", "%.1g", "%.3g"]:
            add("rounding", f, v)

    # field width, justification, zero padding
    for v in ["0.0", "-0.0", "1.5", "-1.5", "1e-5", "-1e100"]:
        for f in ["%20e", "%20g", "%-20e", "%-20g", "%020e", "%020g",
                  "%+20.3e", "%+-20.3g", "%020.3e", "%020.3g", "% 20.2e",
                  "%1e", "%1g", "%13.3e", "%14.4g", "%020.0e", "%020.0g",
                  "%08.0e", "%08.0g"]:
            add("width", f, v)

    return out


# --------------------------------------------------------- exhaustive (unit)

def exhaustive() -> list[Case]:
    out: list[Case] = []

    def add(fmt: str, val: str) -> None:
        out.append(("sweep", fmt, val))

    values = [
        "0.0", "-0.0", "1.0", "-1.0", "0.5", "-0.5", "1.5", "-1.5", "2.0", "-2.0",
        "0.25", "0.125", "0.0625", "0.03125", "0.015625", "0.00390625", "0.75",
        "3.5", "7.25", "10.0", "100.0", "1000.0", "10000.0", "100000.0",
        "1000000.0", "10000000.0", "42.0", "42.5", "123.0", "255.0", "256.0",
        "1024.0", "65536.0", "1048576.0", "16777216.0", "0.001", "0.01", "0.1",
        "1e5", "1e6", "1e7", "1e8", "1e-1", "1e-2", "1e-3", "1e-4",
        "9.5", "0.95", "9.9999", "0.99999", "0.09999", "1.0000001", "0.49999",
        "0.50001", "1.4999", "2.5", "0.05", "0.005", "9999.5", "12345.0",
        "999999.0", "1000001.0", "8.0", "0.8", "80.0", "1.0009765625",
        "3.140625", "2.71875", "6.5", "0.375", "0.6875", "1.9375", "31.25",
        "62.5", "781.25", "0.0009765625", "4096.5", "0.0517578125",
    ]
    convs = ["e", "E", "g", "G"]
    for v in values:
        for c in convs:
            add(f"%{c}", v)
            add(f"%#{c}", v)
            add(f"%+{c}", v)
            add(f"% {c}", v)
            for p in range(13):
                add(f"%.{p}{c}", v)
                add(f"%#.{p}{c}", v)
    # magnitude ladder at the default and a few explicit precisions
    for e in list(range(-30, 31, 1)):
        v = f"1.5e{e}"
        for f in ["%e", "%g", "%.0e", "%.1e", "%.3e", "%.0g", "%.2g", "%.4g",
                  "%#.3e", "%#.3g", "%E", "%G"]:
            add(f, v)
    # widths crossed with signs on a handful of magnitudes
    for v in ["0.0", "-0.0", "1.5", "-1.5", "1e-5", "-1e-5", "1e10", "-1e10"]:
        for f in ["%20e", "%20g", "%-20e", "%-20g", "%020e", "%020g", "%+20.3e",
                  "%+-20.3g", "%020.3e", "%020.3g", "% 20.2e", "% -20.2g",
                  "%1e", "%1g", "%13.3e", "%14.4g", "%0.3e", "%0.3g",
                  "%020.0e", "%020.0g", "%08.0e", "%08.0g", "%5.0e", "%5.0g",
                  "%30.10e", "%30.10g", "%-30.10e", "%030.10g"]:
            add(f, v)
    return out


# ------------------------------------------------------------------- evaluation

PROBE = r"""
#define NANOPRINTF_USE_FIELD_WIDTH_FORMAT_SPECIFIERS 1
#define NANOPRINTF_USE_PRECISION_FORMAT_SPECIFIERS 1
#define NANOPRINTF_USE_FLOAT_FORMAT_SPECIFIERS 1
#define NANOPRINTF_USE_LARGE_FORMAT_SPECIFIERS 1
#define NANOPRINTF_USE_SMALL_FORMAT_SPECIFIERS 1
#define NANOPRINTF_USE_BINARY_FORMAT_SPECIFIERS 1
#define NANOPRINTF_USE_WRITEBACK_FORMAT_SPECIFIERS 1
#define NANOPRINTF_USE_ALT_FORM_FLAG 1
#define NANOPRINTF_USE_FLOAT_SCI_FORMAT_SPECIFIER 1
#define NANOPRINTF_USE_FLOAT_SHORTEST_FORMAT_SPECIFIER 1
#define NANOPRINTF_IMPLEMENTATION
#include "nanoprintf.h"
#include <stdio.h>
#include <string.h>
#include <float.h>

#define C(fmt, val) do { \
    char a[600]; \
    npf_snprintf(a, sizeof a, fmt, val); \
    printf("%s\t", a); \
    snprintf(a, sizeof a, fmt, (double)(val)); \
    printf("%s\n", a); \
  } while (0)
int main(void) {
CASES
  return 0;
}
"""


def evaluate(cases: list[Case], single: bool) -> list[tuple[str, str]]:
    body = "\n".join(f'  C("{fmt}", {val});' for _g, fmt, val in cases)
    src = PROBE.replace("CASES", body)
    if single:
        src = src.replace("#define NANOPRINTF_IMPLEMENTATION",
                          "#define NANOPRINTF_USE_FLOAT_SINGLE_PRECISION 1\n"
                          "#define NANOPRINTF_IMPLEMENTATION")
    with tempfile.TemporaryDirectory() as d:
        p = pathlib.Path(d)
        (p / "t.c").write_text(src)
        subprocess.run(["cc", "-w", f"-I{NPF}", "-o", str(p / "t"), str(p / "t.c")],
                       check=True)
        r = subprocess.run([str(p / "t")], check=True, stdout=subprocess.PIPE)
    lines = r.stdout.decode().split("\n")[:-1]
    assert len(lines) == len(cases), f"{len(lines)} results for {len(cases)} cases"
    return [tuple(ln.split("\t")) for ln in lines]  # type: ignore[misc]


def requested_sig(fmt: str) -> int:
    """Significant digits the conversion asks for."""
    prec = 6
    if "." in fmt:
        tail = fmt.split(".", 1)[1][:-1]
        prec = int(tail) if tail.isdigit() else 0
    return prec + 1 if fmt[-1] in "eE" else max(prec, 1)


def is_exact_tie(val: str, sig: int) -> bool:
    """True when the exact expansion is <sig digits> then a 5 then nothing."""
    d = decimal.Decimal(float(val))
    digits = "".join(c for c in f"{d:.60e}".split("e")[0] if c.isdigit()).rstrip("0")
    return len(digits) == sig + 1 and digits[sig] == "5"


def verify(cases: list[Case], label: str) -> list[tuple[str, str, str, str, bool]]:
    """Return (group, fmt, val, expected, needs_single_precision_guard)."""
    dbl = evaluate(cases, single=False)
    sgl = evaluate(cases, single=True)
    keep = []
    ties = 0
    dropped: dict[str, int] = {}
    seen: set[tuple[str, str]] = set()
    for (group, fmt, val), (nd, sd), (ns, _) in zip(cases, dbl, sgl, strict=True):
        if (fmt, val) in seen:
            continue
        seen.add((fmt, val))
        if nd != sd:
            if is_exact_tie(val, requested_sig(fmt)):
                ties += 1
                keep.append(("tie", fmt, val, nd, ns != nd))
            else:
                dropped[val] = dropped.get(val, 0) + 1
            continue
        keep.append((group, fmt, val, nd, ns != nd))
    print(f"{label}: {len(keep)} assertions ({ties} deliberate ties), "
          f"{sum(dropped.values())} dropped past the accuracy limit "
          f"across {len(dropped)} values: {sorted(dropped)}", file=sys.stderr)
    return keep


def cesc(s: str) -> str:
    return s.replace("\\", "\\\\").replace('"', '\\"')


def main() -> int:
    # --- curated block: NPF_TEST calls with #if guards, for conformance.c
    keep = verify(curated(), "conformance")

    def guards(fmt: str, needs_sp_guard: bool) -> tuple[str, ...]:
        g = []
        g.append("NANOPRINTF_USE_FLOAT_SCI_FORMAT_SPECIFIER == 1" if fmt[-1] in "eE"
                 else "NANOPRINTF_USE_FLOAT_SHORTEST_FORMAT_SPECIFIER == 1")
        if "#" in fmt:
            g.append("NANOPRINTF_USE_ALT_FORM_FLAG == 1")
        if any(c.isdigit() for c in fmt.split(".")[0]) or "-" in fmt:
            g.append("NANOPRINTF_USE_FIELD_WIDTH_FORMAT_SPECIFIERS == 1")
        if needs_sp_guard:
            g.append("NANOPRINTF_USE_FLOAT_SINGLE_PRECISION != 1")
        return tuple(g)

    buckets: dict[tuple[str, ...], list[str]] = {}
    for g, fmt, val, exp, sp in keep:
        key = guards(fmt, sp)
        buckets.setdefault(key, []).append(
            f'    NPF_TEST("{cesc(exp)}", "{cesc(fmt)}", {val}); /* {g} */')
    lines = []
    for key in sorted(buckets, key=lambda k: (len(k), k)):
        lines.extend(f"#if {cond}" for cond in key)
        lines.extend(buckets[key])
        lines.extend(["#endif"] * len(key))
        lines.append("")
    (HERE / "conformance_eg.inc").write_text("\n".join(lines))

    # --- exhaustive table: {format, value, expected} rows, for unit_etoa_rev.cc
    keep = verify(exhaustive(), "unit")
    rows_all: list[str] = []
    rows_dbl: list[str] = []
    for _g, fmt, val, exp, needs_guard in keep:
        row = f'    {{ "{cesc(fmt)}", {val}, "{cesc(exp)}" }},'
        (rows_dbl if needs_guard else rows_all).append(row)
    (HERE / "unit_eg.inc").write_text(
        "// Generated by .context/gen_eg_tests.py; every row verified against the\n"
        "// system printf. Rows in the second table are double-precision only.\n"
        "static npf_eg_case const npf_eg_cases[] = {\n"
        + "\n".join(rows_all) + "\n};\n\n"
        "static npf_eg_case const npf_eg_cases_double_only[] = {\n"
        + "\n".join(rows_dbl) + "\n};\n")
    print("wrote conformance_eg.inc and unit_eg.inc", file=sys.stderr)
    return 0


if __name__ == "__main__":
    sys.exit(main())
