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
        required=True,
        choices=("cm0", "cm4", "avr2", "avr5", "host"),
        help="target platform",
    )
    return parser.parse_args()


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
        subprocess.run(
            cc_cmd, check=True, cwd=temp_dir, input=rb'#include "nanoprintf.h"'
        )
        return subprocess.run(
            nm_cmd, check=True, cwd=temp_dir, stdout=subprocess.PIPE
        ).stdout.decode()


def _measure(build_output: str) -> None:
    """Parse the results of nm to accumulate and print a total size."""
    total = 0
    for line in build_output.split("\n"):
        if not (parts := [x for x in line.split() if x.strip()]):
            continue

        print(line)

        if parts[0] in ("u", "U"):
            continue

        if len(parts) >= 3 and parts[2] not in ("t", "T"):
            continue

        total += int(line.split()[1], 16)

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


def main() -> int:
    """Entry point"""
    for cfg in _CONFIGS:
        print(f'Configuration "{cfg["name"]}":')
        _measure(_build(_parse_args().platform, cfg["flags"]))
        print()

    return 0


if __name__ == "__main__":
    sys.exit(main())
