"""Run the 16-bit-int conformance probe on an emulated AVR.

Needs avr-gcc and qemu-system-avr. Every other test target has a 32-bit int, so
this is the only place the width and precision arithmetic runs against an
INT_MAX of 32767.
"""

import pathlib
import subprocess
import sys

_MCU = "atmega2560"
_MACHINE = "mega2560"
_TIMEOUT_SEC = 120

# The conversions the probe checks; floats need a 64-bit double avr-libc lacks.
_FLAGS = [
    "-DNANOPRINTF_USE_FIELD_WIDTH_FORMAT_SPECIFIERS=1",
    "-DNANOPRINTF_USE_PRECISION_FORMAT_SPECIFIERS=1",
    "-DNANOPRINTF_USE_SMALL_FORMAT_SPECIFIERS=1",
    "-DNANOPRINTF_USE_LARGE_FORMAT_SPECIFIERS=1",
    "-DNANOPRINTF_USE_BINARY_FORMAT_SPECIFIERS=1",
    "-DNANOPRINTF_USE_WRITEBACK_FORMAT_SPECIFIERS=1",
    "-DNANOPRINTF_USE_FLOAT_FORMAT_SPECIFIERS=0",
    "-DNANOPRINTF_USE_ALT_FORM_FLAG=1",
    "-DNANOPRINTF_USE_FIXED_WIDTH_FORMAT_SPECIFIERS=1",
]

# The probe passes widths no buffer could hold on purpose, which is exactly what
# the format diagnostics exist to flag. The conformance build suppresses the same.
_NO_FORMAT_WARNINGS = ["-Wno-format", "-Wno-format-overflow", "-Wno-format-truncation"]


def _git_root() -> pathlib.Path:
    """Return the root of the current file git repository."""
    cur = pathlib.Path(__file__).resolve()
    while cur != cur.parent:
        if (cur / ".git").exists():  # dir for a normal clone, file for a worktree
            return cur
        cur = cur.parent

    msg = f"{__file__} not in git repo"
    raise ValueError(msg)


def main() -> int:
    """Build the probe, run it under qemu, and report what it printed."""
    root = _git_root()
    src = pathlib.Path(__file__).with_suffix(".c")
    elf = root / "avr_int16.elf"

    cc_cmd = [
        "avr-gcc",
        f"-mmcu={_MCU}",
        "-Os",
        "-std=c99",
        "-Wall",
        "-Wextra",
        "-Werror",
        *_NO_FORMAT_WARNINGS,
        f"-I{root}",
        *_FLAGS,
        "-o",
        str(elf),
        str(src),
    ]

    # qemu-system-avr has no way for the guest to halt it, so the probe spins
    # after printing and the timeout is what ends the run.
    qemu_cmd = [
        "qemu-system-avr",
        "-machine",
        _MACHINE,
        "-bios",
        str(elf),
        "-nographic",
        "-serial",
        "mon:stdio",
    ]

    print(" ".join(cc_cmd), flush=True)
    subprocess.run(cc_cmd, check=True)

    print(" ".join(qemu_cmd), flush=True)
    try:
        out = subprocess.run(
            qemu_cmd, stdout=subprocess.PIPE, stderr=subprocess.DEVNULL,
            timeout=_TIMEOUT_SEC, check=False,
        ).stdout.decode(errors="replace")
    except subprocess.TimeoutExpired as expired:
        out = (expired.stdout or b"").decode(errors="replace")
    finally:
        elf.unlink(missing_ok=True)

    print(out, end="", flush=True)

    if "DONE" not in out:
        print("FAILED: probe never reached its end", file=sys.stderr)
        return 1
    if "RESULT PASS" not in out:
        print("FAILED: probe reported failures", file=sys.stderr)
        return 1
    return 0


if __name__ == "__main__":
    sys.exit(main())
