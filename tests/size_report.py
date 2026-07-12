"""Compile and analyze nanoprintf for different architectures."""

import argparse
import pathlib
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
    group = parser.add_mutually_exclusive_group()
    group.add_argument(
        "--check-readme",
        action="store_true",
        help="verify the README size table matches the current source",
    )
    group.add_argument(
        "--update-readme",
        action="store_true",
        help="rewrite the README size table from the current source",
    )
    args = parser.parse_args()
    if not (args.platform or args.check_readme or args.update_readme):
        parser.error("one of -p/--platform, --check-readme, or --update-readme is required")
    if args.platform and (args.check_readme or args.update_readme):
        parser.error("-p/--platform cannot be combined with --check-readme/--update-readme")
    return args


def _git_root() -> pathlib.Path:
    """Return the root of the current file git repository."""
    cur = pathlib.Path(__file__).resolve()
    while cur != cur.parent:
        if (cur / ".git").is_dir():
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


_CONFIGS = [
    {
        "name": "Minimal",
        "flags": [
            "-DNANOPRINTF_USE_FIELD_WIDTH_FORMAT_SPECIFIERS=0",
            "-DNANOPRINTF_USE_PRECISION_FORMAT_SPECIFIERS=0",
            "-DNANOPRINTF_USE_FLOAT_FORMAT_SPECIFIERS=0",
            "-DNANOPRINTF_USE_SMALL_FORMAT_SPECIFIERS=0",
            "-DNANOPRINTF_USE_LARGE_FORMAT_SPECIFIERS=0",
            "-DNANOPRINTF_USE_BINARY_FORMAT_SPECIFIERS=0",
            "-DNANOPRINTF_USE_WRITEBACK_FORMAT_SPECIFIERS=0",
            "-DNANOPRINTF_USE_ALT_FORM_FLAG=0",
        ],
    },
    {
        "name": "Binary",
        "flags": [
            "-DNANOPRINTF_USE_FIELD_WIDTH_FORMAT_SPECIFIERS=0",
            "-DNANOPRINTF_USE_PRECISION_FORMAT_SPECIFIERS=0",
            "-DNANOPRINTF_USE_FLOAT_FORMAT_SPECIFIERS=0",
            "-DNANOPRINTF_USE_SMALL_FORMAT_SPECIFIERS=0",
            "-DNANOPRINTF_USE_LARGE_FORMAT_SPECIFIERS=0",
            "-DNANOPRINTF_USE_BINARY_FORMAT_SPECIFIERS=1",
            "-DNANOPRINTF_USE_WRITEBACK_FORMAT_SPECIFIERS=0",
            "-DNANOPRINTF_USE_ALT_FORM_FLAG=0",
        ],
    },
    {
        "name": "Field Width + Precision",
        "flags": [
            "-DNANOPRINTF_USE_FIELD_WIDTH_FORMAT_SPECIFIERS=1",
            "-DNANOPRINTF_USE_PRECISION_FORMAT_SPECIFIERS=1",
            "-DNANOPRINTF_USE_FLOAT_FORMAT_SPECIFIERS=0",
            "-DNANOPRINTF_USE_SMALL_FORMAT_SPECIFIERS=1",
            "-DNANOPRINTF_USE_LARGE_FORMAT_SPECIFIERS=0",
            "-DNANOPRINTF_USE_BINARY_FORMAT_SPECIFIERS=0",
            "-DNANOPRINTF_USE_WRITEBACK_FORMAT_SPECIFIERS=0",
            "-DNANOPRINTF_USE_ALT_FORM_FLAG=1",
        ],
    },
    {
        "name": "Field Width + Precision + Binary",
        "flags": [
            "-DNANOPRINTF_USE_FIELD_WIDTH_FORMAT_SPECIFIERS=1",
            "-DNANOPRINTF_USE_PRECISION_FORMAT_SPECIFIERS=1",
            "-DNANOPRINTF_USE_FLOAT_FORMAT_SPECIFIERS=0",
            "-DNANOPRINTF_USE_SMALL_FORMAT_SPECIFIERS=1",
            "-DNANOPRINTF_USE_LARGE_FORMAT_SPECIFIERS=0",
            "-DNANOPRINTF_USE_BINARY_FORMAT_SPECIFIERS=1",
            "-DNANOPRINTF_USE_WRITEBACK_FORMAT_SPECIFIERS=0",
            "-DNANOPRINTF_USE_ALT_FORM_FLAG=1",
        ],
    },
    {
        "name": "Float",
        "flags": [
            "-DNANOPRINTF_USE_FIELD_WIDTH_FORMAT_SPECIFIERS=0",
            "-DNANOPRINTF_USE_PRECISION_FORMAT_SPECIFIERS=1",
            "-DNANOPRINTF_USE_FLOAT_FORMAT_SPECIFIERS=1",
            "-DNANOPRINTF_USE_SMALL_FORMAT_SPECIFIERS=0",
            "-DNANOPRINTF_USE_LARGE_FORMAT_SPECIFIERS=0",
            "-DNANOPRINTF_USE_BINARY_FORMAT_SPECIFIERS=0",
            "-DNANOPRINTF_USE_WRITEBACK_FORMAT_SPECIFIERS=0",
            "-DNANOPRINTF_USE_ALT_FORM_FLAG=1",
        ],
    },
    {
        "name": "Float (single-precision)",
        "flags": [
            "-DNANOPRINTF_USE_FIELD_WIDTH_FORMAT_SPECIFIERS=0",
            "-DNANOPRINTF_USE_PRECISION_FORMAT_SPECIFIERS=1",
            "-DNANOPRINTF_USE_FLOAT_FORMAT_SPECIFIERS=1",
            "-DNANOPRINTF_USE_FLOAT_SINGLE_PRECISION=1",
            "-DNANOPRINTF_USE_SMALL_FORMAT_SPECIFIERS=0",
            "-DNANOPRINTF_USE_LARGE_FORMAT_SPECIFIERS=0",
            "-DNANOPRINTF_USE_BINARY_FORMAT_SPECIFIERS=0",
            "-DNANOPRINTF_USE_WRITEBACK_FORMAT_SPECIFIERS=0",
            "-DNANOPRINTF_USE_ALT_FORM_FLAG=1",
        ],
    },
    {
        "name": "Float + Hex Float",
        "flags": [
            "-DNANOPRINTF_USE_FIELD_WIDTH_FORMAT_SPECIFIERS=0",
            "-DNANOPRINTF_USE_PRECISION_FORMAT_SPECIFIERS=1",
            "-DNANOPRINTF_USE_FLOAT_FORMAT_SPECIFIERS=1",
            "-DNANOPRINTF_USE_FLOAT_HEX_FORMAT_SPECIFIER=1",
            "-DNANOPRINTF_USE_SMALL_FORMAT_SPECIFIERS=0",
            "-DNANOPRINTF_USE_LARGE_FORMAT_SPECIFIERS=0",
            "-DNANOPRINTF_USE_BINARY_FORMAT_SPECIFIERS=0",
            "-DNANOPRINTF_USE_WRITEBACK_FORMAT_SPECIFIERS=0",
            "-DNANOPRINTF_USE_ALT_FORM_FLAG=1",
        ],
    },
    {
        "name": "Everything",
        "flags": [
            "-DNANOPRINTF_USE_FIELD_WIDTH_FORMAT_SPECIFIERS=1",
            "-DNANOPRINTF_USE_PRECISION_FORMAT_SPECIFIERS=1",
            "-DNANOPRINTF_USE_FLOAT_FORMAT_SPECIFIERS=1",
            "-DNANOPRINTF_USE_SMALL_FORMAT_SPECIFIERS=1",
            "-DNANOPRINTF_USE_LARGE_FORMAT_SPECIFIERS=1",
            "-DNANOPRINTF_USE_BINARY_FORMAT_SPECIFIERS=1",
            "-DNANOPRINTF_USE_WRITEBACK_FORMAT_SPECIFIERS=1",
            "-DNANOPRINTF_USE_ALT_FORM_FLAG=1",
        ],
    },
]


_README_BEGIN = "<!-- BEGIN SIZE REPORT (generated by tests/size_report.py --update-readme) -->"
_README_END = "<!-- END SIZE REPORT -->"
_README_RANGE_BEGIN = "<!-- BEGIN SIZE RANGE -->"
_README_RANGE_END = "<!-- END SIZE RANGE -->"


def _readme_regions() -> tuple[str, str]:
    """Build the Cortex-M0/M4 size table and the summary size-range snippet."""
    rows = ["| Configuration | Cortex-M0 | Cortex-M4 |", "|---|--:|--:|"]
    m4_sizes = []
    for cfg in _CONFIGS:
        m0 = _total_size(_build("cm0", cfg["flags"]))
        m4 = _total_size(_build("cm4", cfg["flags"]))
        m4_sizes.append(m4)
        rows.append(f"| {cfg['name']} | {m0} | {m4} |")

    low = min(m4_sizes) // 10 * 10
    high = -(-max(m4_sizes) // 100) * 100
    return "\n".join(rows), f"*~{low}-{high} bytes of object code*"


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


def _readme(*, check: bool) -> int:
    """Update the README size table, or (when check) verify it is current."""
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

    if not check:
        readme_path.write_text(updated, encoding="utf-8", newline="\n")
        print("Updated README size table.")
        return 0

    if updated != original:
        print(
            "README size table is out of date; "
            "run 'python tests/size_report.py --update-readme' with the toolchain "
            "above, or paste the expected regions:\n"
            f"\n{table}\n\n{size_range}",
            file=sys.stderr,
        )
        return 1

    print("README size table is up to date.")
    return 0


def main() -> int:
    """Entry point"""
    args = _parse_args()

    if args.check_readme or args.update_readme:
        return _readme(check=args.check_readme)

    for cfg in _CONFIGS:
        print(f'Configuration "{cfg["name"]}":')
        _measure(_build(args.platform, cfg["flags"]))
        print()

    return 0


if __name__ == "__main__":
    sys.exit(main())
