"""Compile and analyze nanoprintf for different architectures."""

import argparse
import pathlib
import re
import subprocess
import sys
import tempfile


def _parse_args() -> argparse.Namespace:
    """Parse and validate command-line arguments."""
    parser = argparse.ArgumentParser()
    parser.add_argument(
        "-p",
        "--platform",
        choices=("cm0", "cm4", "avr2", "avr5", "host"),
        help="print a detailed size breakdown for this target platform",
    )
    parser.add_argument(
        "--update-readme",
        action="store_true",
        help="rewrite the README size table from the current source",
    )
    args = parser.parse_args()
    if not (args.platform or args.update_readme):
        parser.error("one of -p/--platform or --update-readme is required")
    if args.platform and args.update_readme:
        parser.error("-p/--platform cannot be combined with --update-readme")
    return args


def _git_root() -> pathlib.Path:
    """Return the root of the current file git repository."""
    cur = pathlib.Path(__file__).resolve()
    while cur != cur.parent:
        if (cur / ".git").exists():  # dir for a normal clone, file for a worktree
            return cur
        cur = cur.parent

    msg = f"{__file__} not in git repo"
    raise ValueError(msg)


def _build(platform: str, flags: list[str]) -> str:
    """Build a nanoprintf implementation object for platform + flags."""

    cc_cmd = []

    match platform:
        case "host":
            cc_exe = "cc"
            nm_exe = "nm"

        case _ if platform.startswith("cm"):
            cc_exe = "arm-none-eabi-gcc"
            cc_cmd += [f"-mcpu=cortex-m{platform[-1]}"]

            if platform == "cm4":
                cc_cmd += ["-mfloat-abi=hard"]

            nm_exe = "arm-none-eabi-nm"

        case _ if platform.startswith("avr"):
            cc_exe = "avr-gcc"
            cc_cmd += [f"-mmcu={platform}"]
            nm_exe = "avr-nm"

        case unknown:
            msg = f"Unknown platform {unknown}"
            raise ValueError(msg)

    cc_cmd.insert(0, cc_exe)
    cc_cmd += [
        "-c",
        "-x",
        "c",
        "-Os",
        "-Wall",
        "-Wextra",
        "-Werror",
        f"-I{_git_root()}",
        "-o",
        "npf.o",
        "-DNANOPRINTF_IMPLEMENTATION",
        *flags,
        "-",
    ]

    nm_cmd = [nm_exe, "--print-size", "--size-sort", "npf.o"]

    print(" ".join(cc_cmd))
    print(" ".join(nm_cmd), flush=True)

    with tempfile.TemporaryDirectory() as temp_dir:
        subprocess.run(cc_cmd, check=True, cwd=temp_dir, input=rb'#include "nanoprintf.h"')
        return subprocess.run(
            nm_cmd, check=True, cwd=temp_dir, stdout=subprocess.PIPE
        ).stdout.decode()


def _total_size(build_output: str) -> int:
    """Sum the sizes of the text symbols reported by nm."""
    total = 0
    for line in build_output.split("\n"):
        if not (parts := [x for x in line.split() if x.strip()]):
            continue

        if parts[0] in ("u", "U"):
            continue

        if len(parts) >= 3 and parts[2] not in ("t", "T"):
            continue

        total += int(parts[1], 16)

    return total


def _measure(build_output: str) -> None:
    """Print the nm output and the accumulated total size."""
    for line in build_output.split("\n"):
        if line.strip():
            print(line)

    total = _total_size(build_output)
    print(f"Total size: 0x{total:x} ({total}) bytes")


_MANDATORY = (
    "FIELD_WIDTH_FORMAT_SPECIFIERS",
    "PRECISION_FORMAT_SPECIFIERS",
    "FLOAT_FORMAT_SPECIFIERS",
    "SMALL_FORMAT_SPECIFIERS",
    "LARGE_FORMAT_SPECIFIERS",
    "BINARY_FORMAT_SPECIFIERS",
    "WRITEBACK_FORMAT_SPECIFIERS",
    "ALT_FORM_FLAG",
)

_OPTIONAL = (
    "FLOAT_SINGLE_PRECISION",
    "FLOAT_HEX_FORMAT_SPECIFIER",
    "FLOAT_SCI_FORMAT_SPECIFIER",
    "FLOAT_SHORTEST_FORMAT_SPECIFIER",
    "FIXED_WIDTH_FORMAT_SPECIFIERS",
)


def _flags(**on: int) -> list[str]:
    """Build the -D list for a configuration.

    nanoprintf requires every mandatory flag to be defined once any of them is, so
    those are always emitted; the optional ones default to 0 in the header and are
    emitted only when set. Keyword names are the NANOPRINTF_USE_ prefix stripped.
    """
    if unknown := set(on) - set(_MANDATORY) - set(_OPTIONAL):
        msg = f"unknown configuration flags: {sorted(unknown)}"
        raise ValueError(msg)

    return [f"-DNANOPRINTF_USE_{f}={int(on.get(f, 0))}" for f in _MANDATORY] + [
        f"-DNANOPRINTF_USE_{f}=1" for f in _OPTIONAL if on.get(f)
    ]


# The float table's zero: what a float build has apart from the floats.
_BASE = {
    "FIELD_WIDTH_FORMAT_SPECIFIERS": 1,
    "SMALL_FORMAT_SPECIFIERS": 1,
    "ALT_FORM_FLAG": 1,
}

_INT_CONFIGS = [
    ("Minimal", _flags()),
    ("Minimal + binary", _flags(BINARY_FORMAT_SPECIFIERS=1)),
    ("Field width", _flags(**_BASE)),
    ("Field width + precision", _flags(**_BASE, PRECISION_FORMAT_SPECIFIERS=1)),
    (
        "Field width + precision + binary",
        _flags(**_BASE, PRECISION_FORMAT_SPECIFIERS=1, BINARY_FORMAT_SPECIFIERS=1),
    ),
]

_SCI = {"FLOAT_SCI_FORMAT_SPECIFIER": 1}
_SHORTEST = {"FLOAT_SHORTEST_FORMAT_SPECIFIER": 1}
_HEX = {"FLOAT_HEX_FORMAT_SPECIFIER": 1}
_ALL_FLOAT = {**_SCI, **_SHORTEST, **_HEX}
_EVERYTHING = {**_ALL_FLOAT, "LARGE_FORMAT_SPECIFIERS": 1,
               "BINARY_FORMAT_SPECIFIERS": 1, "WRITEBACK_FORMAT_SPECIFIERS": 1}

# Rows of the float table, each measured with precision on and off. Every row also
# gets _BASE and %f; the name lists what the row adds on top.
_FLOAT_CONFIGS = [
    ("%f", {}),
    ("%f %e", _SCI),
    ("%f %g", _SHORTEST),
    ("%f %e %g", {**_SCI, **_SHORTEST}),
    ("%f %a", _HEX),
    ("%f %e %g %a", _ALL_FLOAT),
    ("%f %e %g %a, single-precision", {**_ALL_FLOAT, "FLOAT_SINGLE_PRECISION": 1}),
    ("Everything (adds large, binary, write-back)", _EVERYTHING),
]


def _float_flags(extra: dict[str, int], *, precision: bool) -> list[str]:
    """Flags for one cell of the float table."""
    return _flags(
        **_BASE, FLOAT_FORMAT_SPECIFIERS=1,
        PRECISION_FORMAT_SPECIFIERS=int(precision), **extra,
    )


# 'wN' and 'wfN' resolve to length modifiers the rows above already pay for, so
# what they cost is the delta against the row they follow rather than a table of
# their own. Measured in the per-platform breakdown only.
_FIXED = {"FIXED_WIDTH_FORMAT_SPECIFIERS": 1}
_PREC = {"PRECISION_FORMAT_SPECIFIERS": 1}

_FIXED_WIDTH_CONFIGS = [
    ("Field width + precision + fixed-width", _flags(**_BASE, **_PREC, **_FIXED)),
    ("Field width + precision + large",
     _flags(**_BASE, **_PREC, LARGE_FORMAT_SPECIFIERS=1)),
    ("Field width + precision + large + fixed-width",
     _flags(**_BASE, **_PREC, LARGE_FORMAT_SPECIFIERS=1, **_FIXED)),
    ("Everything + fixed-width",
     _float_flags({**_EVERYTHING, **_FIXED}, precision=True)),
]


def _configs() -> list[tuple[str, list[str]]]:
    """Every measured configuration, for the per-platform breakdown."""
    out = list(_INT_CONFIGS)
    for name, extra in _FLOAT_CONFIGS:
        for precision in (True, False):
            suffix = "" if precision else ", no precision"
            out.append((f"{name}{suffix}", _float_flags(extra, precision=precision)))
    return out + _FIXED_WIDTH_CONFIGS


_README_BEGIN = "<!-- BEGIN SIZE REPORT (generated by tests/size_report.py --update-readme) -->"
_README_END = "<!-- END SIZE REPORT -->"
_README_RANGE_BEGIN = "<!-- BEGIN SIZE RANGE -->"
_README_RANGE_END = "<!-- END SIZE RANGE -->"


def _sizes(flags: list[str]) -> tuple[int, int]:
    """Cortex-M0 and Cortex-M4 text size for one configuration."""
    return _total_size(_build("cm0", flags)), _total_size(_build("cm4", flags))


def _readme_regions() -> tuple[str, str]:
    """Build the two Cortex-M0/M4 size tables and the summary size-range snippet."""
    m4_sizes = []

    int_rows = ["| Integer only | Cortex-M0 | Cortex-M4 |", "|---|--:|--:|"]
    for name, flags in _INT_CONFIGS:
        m0, m4 = _sizes(flags)
        m4_sizes.append(m4)
        int_rows.append(f"| {name} | {m0} | {m4} |")

    # the float table is two-dimensional: specifier set down, precision across
    float_rows = [
        (
            "| Floating point | Cortex-M0 | Cortex-M0, no precision "
            "| Cortex-M4 | Cortex-M4, no precision |"
        ),
        "|---|--:|--:|--:|--:|",
    ]
    for name, extra in _FLOAT_CONFIGS:
        cells = []
        for precision in (True, False):
            m0, m4 = _sizes(_float_flags(extra, precision=precision))
            m4_sizes.append(m4)
            cells.append((m0, m4))
        (p_m0, p_m4), (n_m0, n_m4) = cells
        md = re.sub(r"%\w", r"`\g<0>`", name)  # specifiers read better as code
        float_rows.append(f"| {md} | {p_m0} | {n_m0} | {p_m4} | {n_m4} |")

    low = min(m4_sizes) // 10 * 10
    high = -(-max(m4_sizes) // 100) * 100
    table = "\n".join(int_rows) + "\n\n" + "\n".join(float_rows)
    return table, f"*~{low}-{high} bytes of object code*"


def _replace_region(text: str, begin: str, end: str, replacement: str) -> str:
    """Return text with the region between the begin/end markers replaced."""
    try:
        start = text.index(begin) + len(begin)
        stop = text.index(end)
    except ValueError as exc:
        msg = f"README.md is missing the {begin} / {end} markers"
        raise ValueError(msg) from exc

    return f"{text[:start]}{replacement}{text[stop:]}"


def _render_readme(readme: str, table: str, size_range: str) -> str:
    """Return the README text with the generated regions replaced."""
    readme = _replace_region(readme, _README_RANGE_BEGIN, _README_RANGE_END, size_range)
    return _replace_region(readme, _README_BEGIN, _README_END, f"\n\n{table}\n\n")


def _readme() -> int:
    """Rewrite the README size table from the current source."""
    toolchain = subprocess.run(
        ["arm-none-eabi-gcc", "--version"], check=True, stdout=subprocess.PIPE
    ).stdout.decode().splitlines()[0]
    print(toolchain, flush=True)

    readme_path = _git_root() / "README.md"
    original = readme_path.read_text(encoding="utf-8")
    table, size_range = _readme_regions()

    try:
        updated = _render_readme(original, table, size_range)
    except ValueError as exc:
        print(exc, file=sys.stderr)
        return 1

    readme_path.write_text(updated, encoding="utf-8", newline="\n")
    print("Updated README size table.")
    return 0


def main() -> int:
    """Entry point"""
    args = _parse_args()

    if args.update_readme:
        return _readme()

    for name, flags in _configs():
        print(f'Configuration "{name}":')
        _measure(_build(args.platform, flags))
        print()

    return 0


if __name__ == "__main__":
    sys.exit(main())
