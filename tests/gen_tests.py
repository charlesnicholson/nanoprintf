"""Generate the conformance test build artifacts for nanoprintf.

Enumerates all valid flag combinations, then for each generates both a C and a
C++ compilation. --shard I/N takes a strided slice of the combinations so CI can
spread them over parallel jobs; the stride keeps each shard's flag mix varied,
which balances the compile cost better than contiguous blocks would. Artifacts:
  - main.c                : declares + calls all per-combo test functions
  - Makefile               : POSIX make rules (cc + c++)
  - compile_commands.json  : Windows parallel builds (cl.exe)
"""

import argparse
import itertools
import os
import pathlib
import sys
import textwrap


def _write_if_changed(path: pathlib.Path, content: str) -> bool:
    """Write content to path only if it differs. Returns True if changed."""
    if path.exists() and path.read_text() == content:
        return False
    path.write_text(content)
    return True


FLAGS = [
    "NANOPRINTF_USE_FIELD_WIDTH_FORMAT_SPECIFIERS",
    "NANOPRINTF_USE_PRECISION_FORMAT_SPECIFIERS",
    "NANOPRINTF_USE_FLOAT_FORMAT_SPECIFIERS",
    "NANOPRINTF_USE_LARGE_FORMAT_SPECIFIERS",
    "NANOPRINTF_USE_SMALL_FORMAT_SPECIFIERS",
    "NANOPRINTF_USE_BINARY_FORMAT_SPECIFIERS",
    "NANOPRINTF_USE_WRITEBACK_FORMAT_SPECIFIERS",
    "NANOPRINTF_USE_ALT_FORM_FLAG",
    "NANOPRINTF_USE_FLOAT_SINGLE_PRECISION",
    "NANOPRINTF_USE_FLOAT_HEX_FORMAT_SPECIFIER",
    "NANOPRINTF_USE_FLOAT_SCI_FORMAT_SPECIFIER",
    "NANOPRINTF_USE_FLOAT_SHORTEST_FORMAT_SPECIFIER",
    "NANOPRINTF_USE_FIXED_WIDTH_FORMAT_SPECIFIERS",
    "NANOPRINTF_USE_DIVISION_FREE_CONVERSION",
]

# Flags that are only meaningful when NANOPRINTF_USE_FLOAT_FORMAT_SPECIFIERS is 1.
FLOAT_DEPENDENT_FLAGS = [
    "NANOPRINTF_USE_FLOAT_SINGLE_PRECISION",
    "NANOPRINTF_USE_FLOAT_HEX_FORMAT_SPECIFIER",
    "NANOPRINTF_USE_FLOAT_SCI_FORMAT_SPECIFIER",
    "NANOPRINTF_USE_FLOAT_SHORTEST_FORMAT_SPECIFIER",
]


# float=1 + precision=0 is legal, but crossing it with everything would nearly
# double the matrix. Precision compiles out every path where it interacts with the
# other flags, so the float output that survives varies only with these; the flags
# outside the set are pinned to 1 rather than enumerated.
NO_PRECISION_FLOAT_VARIED = {
    "NANOPRINTF_USE_PRECISION_FORMAT_SPECIFIERS",
    "NANOPRINTF_USE_FIELD_WIDTH_FORMAT_SPECIFIERS",
    "NANOPRINTF_USE_ALT_FORM_FLAG",
    "NANOPRINTF_USE_DIVISION_FREE_CONVERSION",
    "NANOPRINTF_USE_FIXED_WIDTH_FORMAT_SPECIFIERS",
    *FLOAT_DEPENDENT_FLAGS,
}


# 'wN' and 'wfN' resolve to an already-existing length modifier while the format
# string is parsed, so they only interact with the flags owning those modifiers
# and the conversions they can precede. Crossing fixed-width=1 with the float
# family would grow the matrix by half for no coverage those flags can affect;
# the flags outside this set are pinned to 0 instead.
FIXED_WIDTH_VARIED = {
    "NANOPRINTF_USE_FIXED_WIDTH_FORMAT_SPECIFIERS",
    "NANOPRINTF_USE_FIELD_WIDTH_FORMAT_SPECIFIERS",
    "NANOPRINTF_USE_PRECISION_FORMAT_SPECIFIERS",
    "NANOPRINTF_USE_FLOAT_FORMAT_SPECIFIERS",
    "NANOPRINTF_USE_LARGE_FORMAT_SPECIFIERS",
    "NANOPRINTF_USE_SMALL_FORMAT_SPECIFIERS",
    "NANOPRINTF_USE_BINARY_FORMAT_SPECIFIERS",
    "NANOPRINTF_USE_WRITEBACK_FORMAT_SPECIFIERS",
    "NANOPRINTF_USE_ALT_FORM_FLAG",
}


def valid_combos() -> list[dict[str, int]]:
    """Return every valid flag combination.

    Constraints:
      - every flag in FLOAT_DEPENDENT_FLAGS requires float=1
      - float=1 + precision=0 is sampled over NO_PRECISION_FLOAT_VARIED only
      - fixed-width=1 requires small=1, and is sampled over FIXED_WIDTH_VARIED only
    """
    combos = []
    for bits in itertools.product((0, 1), repeat=len(FLAGS)):
        combo = dict(zip(FLAGS, bits, strict=True))
        if combo["NANOPRINTF_USE_FLOAT_FORMAT_SPECIFIERS"] == 0 and any(
            combo[flag] == 1 for flag in FLOAT_DEPENDENT_FLAGS
        ):
            continue
        if combo["NANOPRINTF_USE_FIXED_WIDTH_FORMAT_SPECIFIERS"] == 1 and (
            combo["NANOPRINTF_USE_SMALL_FORMAT_SPECIFIERS"] == 0
            or any(v == 1 for k, v in combo.items() if k not in FIXED_WIDTH_VARIED)
        ):
            continue
        if (
            combo["NANOPRINTF_USE_FLOAT_FORMAT_SPECIFIERS"] == 1
            and combo["NANOPRINTF_USE_PRECISION_FORMAT_SPECIFIERS"] == 0
            and any(v == 0 for k, v in combo.items()
                    if k not in NO_PRECISION_FLOAT_VARIED)
        ):
            continue
        combos.append(combo)
    return combos


def combo_label(combo: dict[str, int], lang: str) -> str:
    """Short human-readable label for a flag combo."""
    short = {
        "NANOPRINTF_USE_FIELD_WIDTH_FORMAT_SPECIFIERS": "fw",
        "NANOPRINTF_USE_PRECISION_FORMAT_SPECIFIERS": "prec",
        "NANOPRINTF_USE_FLOAT_FORMAT_SPECIFIERS": "float",
        "NANOPRINTF_USE_LARGE_FORMAT_SPECIFIERS": "large",
        "NANOPRINTF_USE_SMALL_FORMAT_SPECIFIERS": "small",
        "NANOPRINTF_USE_BINARY_FORMAT_SPECIFIERS": "bin",
        "NANOPRINTF_USE_WRITEBACK_FORMAT_SPECIFIERS": "wb",
        "NANOPRINTF_USE_ALT_FORM_FLAG": "alt",
        "NANOPRINTF_USE_FLOAT_SINGLE_PRECISION": "sp",
        "NANOPRINTF_USE_FLOAT_HEX_FORMAT_SPECIFIER": "hexa",
        "NANOPRINTF_USE_FLOAT_SCI_FORMAT_SPECIFIER": "sci",
        "NANOPRINTF_USE_FLOAT_SHORTEST_FORMAT_SPECIFIER": "shortest",
        "NANOPRINTF_USE_FIXED_WIDTH_FORMAT_SPECIFIERS": "fixedw",
        "NANOPRINTF_USE_DIVISION_FREE_CONVERSION": "divfree",
    }
    parts = [f"{short[k]}={v}" for k, v in combo.items()]
    return f"[{lang}] " + " ".join(parts)


def write_main_c(combos: list[dict[str, int]], out: pathlib.Path,
                 shard: str = "1/1") -> bool:
    """Write main.c that declares and calls every combo's test function."""
    n = len(combos)
    total = n * 2

    declarations = "\n".join(
        f"int npf_test_combo_{i}(void);\nextern int npf_test_combo_{i}_pass_count;"
        for i in range(total)
    )

    calls = "\n\n".join(
        f"    combo_fail = npf_test_combo_{i}();\n"
        f"    if (combo_fail != 0)\n"
        f'        fprintf(stderr, "FAILED combo {i}/{total}: {label}\\n");\n'
        f"    total_fail += combo_fail;\n"
        f"    total_pass += npf_test_combo_{i}_pass_count;"
        for i in range(total)
        for label in [combo_label(combos[i % n], "C" if i < n else "C++")]
    )

    content = f"""\
#include <stdio.h>
#include <stdlib.h>

{declarations}

int main(void) {{
    int total_fail = 0;
    int total_pass = 0;
    int combo_fail;

{calls}

    if (total_fail != 0) {{
        fprintf(stderr, "FAILED: %d assertion(s) across {total} combos\\n", total_fail);
    }} else {{
        fprintf(stderr, "PASSED: %d assertions across {total} objects ({n} flag combos, shard {shard}, x 2 langs)\\n", total_pass);
    }}
    return total_fail != 0 ? EXIT_FAILURE : EXIT_SUCCESS;
}}
"""
    return _write_if_changed(out / "main.c", content)


def define_flags(combo: dict[str, int], idx: int, *, msvc: bool = False) -> str:
    """Return the -D flags string for a single combo."""
    pfx = "/D" if msvc else "-D"
    parts = [f"{pfx}{k}={v}" for k, v in combo.items()]
    parts.append(f"{pfx}NPF_TEST_FUNC=npf_test_combo_{idx}")
    parts.append(f"{pfx}NPF_TEST_PASS_COUNT=npf_test_combo_{idx}_pass_count")
    return " ".join(parts)


def write_makefile(
    combos: list[dict[str, int]],
    out: pathlib.Path,
    *,
    cc: str,
    cxx: str,
    arch: int,
    sanitizer: str,
    extra_cflags: str,
) -> bool:
    """Write a POSIX Makefile that compiles all combos and links them."""
    n = len(combos)
    total = n * 2
    test_dir = out.parent  # tests/
    repo_root = test_dir.parent
    conformance_c = test_dir / "conformance.c"

    # Paths relative to the output (generated/) directory
    conformance_rel = os.path.relpath(conformance_c, out)
    include_rel = os.path.relpath(repo_root, out)
    test_rel = os.path.relpath(test_dir, out)

    obj_names = [f"combo_{i}.o" for i in range(total)]

    san_flags = ""
    if sanitizer == "asan":
        san_flags = "-fsanitize=address"
    elif sanitizer == "ubsan":
        san_flags = "-fsanitize=undefined"

    arch_flag = f"-m{arch}" if arch == 32 else ""

    common_warn = (
        "-Wno-gnu-zero-variadic-macro-arguments "
        "-Wno-format -Wno-format-extra-args -Wno-format-security "
        "-Wno-format-zero-length -Wno-format-overflow -Wno-format-truncation "
        "-Wno-unused-function"
    )

    c_cflags = (
        f"-Os -std=c17 -Wall -Wextra -Wundef -Werror "
        f"{common_warn} -Wno-missing-prototypes "
        f"-I{include_rel} -I{test_rel} "
        f"{arch_flag} {san_flags} {extra_cflags}".rstrip()
    )

    cxx_cflags = (
        f"-Os -std=c++20 -Wall -Wextra -Wundef -Werror "
        f"{common_warn} "
        f"-Wno-old-style-cast -Wno-zero-as-null-pointer-constant "
        f"-I{include_rel} -I{test_rel} "
        f"{arch_flag} {san_flags} {extra_cflags}".rstrip()
    )

    nanoprintf_rel = os.path.relpath(repo_root / "nanoprintf.h", out)

    header = f"""\
CC = {cc}
CXX = {cxx}
CFLAGS = {c_cflags}
CXXFLAGS = {cxx_cflags}
LDFLAGS = {arch_flag} {san_flags} -lm
CONFORMANCE_C = {conformance_rel}
CONFORMANCE_CXX = conformance.cc
NANOPRINTF = {nanoprintf_rel}
DEPS = $(CONFORMANCE_C) $(NANOPRINTF)

all: npf_conformance.timestamp

# Copy .c to .cc so the C++ compiler sees a C++ extension
$(CONFORMANCE_CXX): $(CONFORMANCE_C)
\tcp $< $@

npf_conformance.timestamp: npf_conformance
\t./npf_conformance && touch $@

npf_conformance: main.o {" ".join(obj_names)}
\t$(CXX) $(LDFLAGS) -o $@ $^

main.o: main.c
\t$(CC) $(CFLAGS) -c -o $@ main.c
"""

    combo_rules = "\n\n".join(
        f"combo_{i}.o: $(DEPS) $(CONFORMANCE_CXX)\n"
        f"\t$(CXX) $(CXXFLAGS) {dflags} -c -o $@ $(CONFORMANCE_CXX)"
        if i >= n
        else f"combo_{i}.o: $(DEPS)\n\t$(CC) $(CFLAGS) {dflags} -c -o $@ $(CONFORMANCE_C)"
        for i in range(total)
        for dflags in [define_flags(combos[i % n], i)]
    )

    footer = textwrap.dedent("""\
        clean:
        \trm -f *.o *.cc npf_conformance npf_conformance.timestamp
    """)

    content = f"{header}\n{combo_rules}\n\n{footer}"
    return _write_if_changed(out / "Makefile", content)


def write_compile_commands(
    combos: list[dict[str, int]],
    out: pathlib.Path,
) -> bool:
    """Write compile_commands.json and link.rsp for Windows parallel builds."""
    import json

    n = len(combos)
    total = n * 2
    test_dir = out.parent
    repo_root = test_dir.parent
    conformance_c = test_dir / "conformance.c"
    conformance_rel = os.path.relpath(conformance_c, out)
    include_rel = os.path.relpath(repo_root, out)
    test_rel = os.path.relpath(test_dir, out)

    common = [
        "/nologo",
        "/Os",
        "/W4",
        "/WX",
        "/Zc:preprocessor",
        # Narrowing a constant is the whole point of the truncation tests.
        "/wd4310",
        # MSVC's own 'w' length modifier means wide, so its format checker
        # reads the C23 'wN' / 'wfN' the malformed-specifier tests pass to the
        # system snprintf as a wide conversion with the wrong argument.
        "/wd4473",
        "/wd4474",
        "/wd4475",
        "/wd4476",
        "/wd4477",
        "/wd4505",
        "/wd4778",
        f"/I{include_rel}",
        f"/I{test_rel}",
    ]
    c_extra = ["/std:c11"]
    cxx_extra = ["/TP", "/std:c++20", "/EHsc", "/Zc:__cplusplus"]

    commands: list[list[str]] = []

    # main.c
    commands.append(["cl.exe", *common, "/c", "/Fomain.obj", "main.c"])

    obj_names: list[str] = []
    for i in range(total):
        combo = combos[i % n]
        is_cxx = i >= n
        dflags = define_flags(combo, i, msvc=True).split()
        obj_name = f"combo_{i}.obj"
        obj_names.append(obj_name)
        flags = common + (cxx_extra if is_cxx else c_extra)
        commands.append(
            ["cl.exe", *flags, *dflags, "/c", f"/Fo{obj_name}", conformance_rel]
        )

    changed = _write_if_changed(
        out / "compile_commands.json", json.dumps(commands, indent=1) + "\n"
    )

    # Linker response file
    rsp_lines = ["main.obj", *obj_names]
    changed |= _write_if_changed(out / "link.rsp", "\n".join(rsp_lines) + "\n")
    return changed


def main() -> int:
    """Parse args and generate build artifacts."""
    parser = argparse.ArgumentParser(
        description="Generate nanoprintf conformance test build"
    )
    parser.add_argument(
        "--output",
        type=pathlib.Path,
        default=None,
        help="Output directory (default: tests/generated/)",
    )
    parser.add_argument("--cc", default="cc", help="C compiler (default: cc)")
    parser.add_argument("--cxx", default="c++", help="C++ compiler (default: c++)")
    parser.add_argument(
        "--arch",
        type=int,
        choices=(32, 64),
        default=64,
        help="Target architecture (default: 64)",
    )
    parser.add_argument(
        "--sanitizer",
        choices=("none", "asan", "ubsan"),
        default="none",
        help="Sanitizer to enable",
    )
    parser.add_argument(
        "--shard",
        default="1/1",
        metavar="I/N",
        help="Build shard I of N (1-based). Default 1/1 builds every combination.",
    )
    parser.add_argument("--extra-cflags", default="", help="Extra CFLAGS")
    parser.add_argument("--verbose", action="store_true", help="Verbose output")
    args = parser.parse_args()

    script_dir = pathlib.Path(__file__).resolve().parent
    out = args.output or script_dir / "generated"
    out = out.resolve()
    out.mkdir(parents=True, exist_ok=True)

    try:
        shard_index, shard_count = (int(x) for x in args.shard.split("/", 1))
    except ValueError:
        parser.error(f"--shard wants I/N, got {args.shard!r}")
    if not 1 <= shard_index <= shard_count:
        parser.error(f"--shard {args.shard} is out of range")

    combos = valid_combos()
    all_count = len(combos)
    # Stride rather than slice: consecutive combinations differ only in the
    # last flags, so a contiguous block would be far cheaper to build than
    # its neighbours and the shards would finish at wildly different times.
    combos = combos[shard_index - 1 :: shard_count]
    total = len(combos) * 2

    changed = write_main_c(combos, out, args.shard)
    if sys.platform == "win32":
        changed |= write_compile_commands(combos, out)
        if args.verbose or changed:
            print(
                f"Generated build for {total} objects ({len(combos)} of {all_count} flag "
                f"combos, shard {args.shard}, x 2 langs) in {out}"
            )
    else:
        changed |= write_makefile(
            combos,
            out,
            cc=args.cc,
            cxx=args.cxx,
            arch=args.arch,
            sanitizer=args.sanitizer,
            extra_cflags=args.extra_cflags,
        )
        if args.verbose or changed:
            print(
                f"Generated build for {total} objects ({len(combos)} of {all_count} flag "
                f"combos, shard {args.shard}, x 2 langs) in {out}"
            )
    return 0


if __name__ == "__main__":
    sys.exit(main())
