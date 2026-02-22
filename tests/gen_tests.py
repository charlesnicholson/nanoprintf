"""Generate the conformance test build artifacts for nanoprintf.

Enumerates all valid flag combinations (192), then for each generates
both a C and C++ compilation, for a total of 384 test objects:
  - main.c        : declares + calls all per-combo test functions
  - Makefile       : POSIX make rules (cc + c++)
  - build.bat      : Windows batch file (cl.exe / link.exe)
"""

import argparse
import itertools
import os
import pathlib
import sys


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
]


def valid_combos() -> list[dict[str, int]]:
    """Return every valid flag combination (skip float=1 when precision=0)."""
    combos = []
    for bits in itertools.product((0, 1), repeat=len(FLAGS)):
        combo = dict(zip(FLAGS, bits, strict=True))
        if combo["NANOPRINTF_USE_FLOAT_FORMAT_SPECIFIERS"] == 1 and \
           combo["NANOPRINTF_USE_PRECISION_FORMAT_SPECIFIERS"] == 0:
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
    }
    parts = [f"{short[k]}={v}" for k, v in combo.items()]
    return f"[{lang}] " + " ".join(parts)


def write_main_c(combos: list[dict[str, int]], out: pathlib.Path) -> bool:
    """Write main.c that declares and calls every combo's test function."""
    n = len(combos)
    total = n * 2

    lines = [
        '#include <stdio.h>',
        '#include <stdlib.h>',
        '',
    ]
    for i in range(total):
        lines.append(f'int npf_test_combo_{i}(void);')
        lines.append(f'extern int npf_test_combo_{i}_pass_count;')
    lines += [
        '',
        'int main(void) {',
        '    int total_fail = 0;',
        '    int total_pass = 0;',
        '    int combo_fail;',
        '',
    ]
    for i in range(total):
        combo = combos[i % n]
        lang = "C" if i < n else "C++"
        label = combo_label(combo, lang)
        lines.append(f'    combo_fail = npf_test_combo_{i}();')
        lines.append('    if (combo_fail != 0)')
        lines.append(f'        fprintf(stderr, "FAILED combo {i}/{total}: {label}\\n");')
        lines.append('    total_fail += combo_fail;')
        lines.append(f'    total_pass += npf_test_combo_{i}_pass_count;')
        lines.append('')
    lines += [
        '    if (total_fail != 0) {',
        f'        fprintf(stderr, "FAILED: %d assertion(s) across {total} combos\\n", total_fail);',
        '    } else {',
        f'        fprintf(stderr, "PASSED: %d assertions across {total} combos ({n} flags x 2 langs)\\n", total_pass);',
        '    }',
        '    return total_fail != 0 ? EXIT_FAILURE : EXIT_SUCCESS;',
        '}',
        '',
    ]
    return _write_if_changed(out / "main.c", "\n".join(lines))


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

    lines = [
        f"CC = {cc}",
        f"CXX = {cxx}",
        f"CFLAGS = {c_cflags}",
        f"CXXFLAGS = {cxx_cflags}",
        f"LDFLAGS = {arch_flag} {san_flags} -lm".strip(),
        f"CONFORMANCE_C = {conformance_rel}",
        "CONFORMANCE_CXX = conformance.cc",
        f"HARNESS = {os.path.relpath(test_dir / 'test_harness.h', out)}",
        f"NANOPRINTF = {os.path.relpath(repo_root / 'nanoprintf.h', out)}",
        "DEPS = $(CONFORMANCE_C) $(HARNESS) $(NANOPRINTF)",
        "",
        "all: npf_conformance.timestamp",
        "",
        "# Copy .c to .cc so the C++ compiler sees a C++ extension",
        "$(CONFORMANCE_CXX): $(CONFORMANCE_C)",
        "\tcp $< $@",
        "",
        "npf_conformance.timestamp: npf_conformance",
        "\t./npf_conformance && touch $@",
        "",
        f"npf_conformance: main.o {' '.join(obj_names)}",
        "\t$(CXX) $(LDFLAGS) -o $@ $^",
        "",
        "main.o: main.c",
        "\t$(CC) $(CFLAGS) -c -o $@ main.c",
        "",
    ]

    for i in range(total):
        combo = combos[i % n]
        dflags = define_flags(combo, i)
        is_cxx = i >= n
        if is_cxx:
            lines.append(f"combo_{i}.o: $(DEPS) $(CONFORMANCE_CXX)")
            lines.append(f"\t$(CXX) $(CXXFLAGS) {dflags} -c -o $@ $(CONFORMANCE_CXX)")
        else:
            lines.append(f"combo_{i}.o: $(DEPS)")
            lines.append(f"\t$(CC) $(CFLAGS) {dflags} -c -o $@ $(CONFORMANCE_C)")
        lines.append("")

    lines += [
        "clean:",
        "\trm -f *.o *.cc npf_conformance npf_conformance.timestamp",
        "",
    ]

    return _write_if_changed(out / "Makefile", "\n".join(lines))


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

    common = ["/nologo", "/Os", "/W4", "/WX",
              "/wd4474", "/wd4476", "/wd4477", "/wd4505", "/wd4778",
              f"/I{include_rel}", f"/I{test_rel}"]
    cxx_extra = ["/TP", "/std:c++20", "/EHsc"]

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
        flags = common + (cxx_extra if is_cxx else [])
        commands.append(["cl.exe", *flags, *dflags, "/c", f"/Fo{obj_name}", conformance_rel])

    changed = _write_if_changed(
        out / "compile_commands.json", json.dumps(commands, indent=1) + "\n"
    )

    # Linker response file
    rsp_lines = ["main.obj", *obj_names]
    changed |= _write_if_changed(out / "link.rsp", "\n".join(rsp_lines) + "\n")
    return changed


def main() -> int:
    """Parse args and generate build artifacts."""
    parser = argparse.ArgumentParser(description="Generate nanoprintf conformance test build")
    parser.add_argument("--output", type=pathlib.Path, default=None,
                        help="Output directory (default: tests/generated/)")
    parser.add_argument("--cc", default="cc", help="C compiler (default: cc)")
    parser.add_argument("--cxx", default="c++", help="C++ compiler (default: c++)")
    parser.add_argument("--arch", type=int, choices=(32, 64), default=64,
                        help="Target architecture (default: 64)")
    parser.add_argument("--sanitizer", choices=("none", "asan", "ubsan"), default="none",
                        help="Sanitizer to enable")
    parser.add_argument("--extra-cflags", default="", help="Extra CFLAGS")
    parser.add_argument("--verbose", action="store_true", help="Verbose output")
    args = parser.parse_args()

    script_dir = pathlib.Path(__file__).resolve().parent
    out = args.output or script_dir / "generated"
    out = out.resolve()
    out.mkdir(parents=True, exist_ok=True)

    combos = valid_combos()
    total = len(combos) * 2

    changed = write_main_c(combos, out)
    if sys.platform == "win32":
        changed |= write_compile_commands(combos, out)
        if args.verbose or changed:
            print(f"Generated build for {total} combos ({len(combos)} flags x 2 langs) in {out}")
    else:
        changed |= write_makefile(combos, out, cc=args.cc, cxx=args.cxx, arch=args.arch,
                                  sanitizer=args.sanitizer, extra_cflags=args.extra_cflags)
        if args.verbose or changed:
            print(f"Generated build for {total} combos ({len(combos)} flags x 2 langs) in {out}")
    return 0


if __name__ == "__main__":
    sys.exit(main())
