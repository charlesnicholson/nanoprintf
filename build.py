"""Windows build script for nanoprintf. Invokes cl.exe / link.exe directly."""

import argparse
import os
import pathlib
import subprocess
import sys

_SCRIPT_PATH = pathlib.Path(__file__).resolve().parent

_UNIT_SRCS = [
    "tests/unit_parse_format_spec.cc",
    "tests/unit_binary.cc",
    "tests/unit_bufputc.cc",
    "tests/unit_ftoa_nan.cc",
    "tests/unit_ftoa_rev.cc",
    "tests/unit_ftoa_rev_08.cc",
    "tests/unit_ftoa_rev_16.cc",
    "tests/unit_ftoa_rev_32.cc",
    "tests/unit_ftoa_rev_64.cc",
    "tests/unit_utoa_rev.cc",
    "tests/unit_snprintf.cc",
    "tests/unit_snprintf_safe_empty.cc",
    "tests/unit_vpprintf.cc",
]


def _parse_args() -> argparse.Namespace:
    """Parse command-line arguments."""
    parser = argparse.ArgumentParser()
    parser.add_argument(
        "--cfg",
        choices=["Debug", "RelWithDebInfo", "Release"],
        default="Release",
        const="Release",
        nargs="?",
        help="Build configuration",
    )
    parser.add_argument(
        "--arch",
        type=int,
        choices=(32, 64),
        default=64,
        const=64,
        nargs="?",
        help="Target architecture",
    )
    parser.add_argument("-v", "--verbose", action="store_true", help="verbose")
    return parser.parse_args()


def _run(args: list[str | pathlib.Path], *, verbose: bool, **kwargs: object) -> None:
    """Run a subprocess, printing the command if verbose."""
    if verbose:
        print(f"  {' '.join(str(a) for a in args)}")
    subprocess.run(args, check=True, **kwargs)  # pyright: ignore[reportCallIssue]


def _build_conformance(args: argparse.Namespace) -> bool:
    """Generate, build, and run the conformance test suite."""
    gen_script = _SCRIPT_PATH / "tests" / "gen_tests.py"
    gen_dir = _SCRIPT_PATH / "tests" / "generated"

    gen_args: list[str | pathlib.Path] = [
        sys.executable,
        str(gen_script),
        "--output",
        str(gen_dir),
        "--arch",
        str(args.arch),
    ]
    if args.verbose:
        gen_args.append("--verbose")

    try:
        _run(gen_args, verbose=args.verbose)
    except subprocess.CalledProcessError:
        return False

    # Build with batch file, then run separately
    try:
        _run([str(gen_dir / "build.bat")], verbose=args.verbose, cwd=gen_dir)
    except subprocess.CalledProcessError:
        return False

    exe = gen_dir / "npf_conformance.exe"
    try:
        _run([str(exe)], verbose=args.verbose)
    except subprocess.CalledProcessError:
        return False

    return True


def _build_unit_tests(args: argparse.Namespace) -> bool:
    """Build and run both unit test variants with cl.exe."""
    build_dir = _SCRIPT_PATH / "build"
    build_dir.mkdir(parents=True, exist_ok=True)

    opt_flag = "/Od" if args.cfg == "Debug" else "/Os"
    cxx_flags = [
        "/nologo",
        opt_flag,
        "/std:c++20",
        "/EHsc",
        "/W4",
        "/WX",
        "/wd4464",
        "/wd4514",
        "/wd4710",
        "/wd4711",
        "/wd4619",
        "/wd4820",
        "/wd5039",
        "/wd5262",
        "/wd5264",
    ]

    # Compile doctest_main.cc once
    doctest_obj = build_dir / "doctest_main.obj"
    try:
        _run(
            ["cl.exe", *cxx_flags, "/c", f"/Fo{doctest_obj}", "tests/doctest_main.cc"],
            verbose=args.verbose,
        )
    except subprocess.CalledProcessError:
        return False

    for variant, large_val in [("normal", "0"), ("large", "1")]:
        var_dir = build_dir / f"unit_{variant}"
        var_dir.mkdir(parents=True, exist_ok=True)

        objs: list[pathlib.Path] = []
        for src in _UNIT_SRCS:
            obj = var_dir / (pathlib.Path(src).stem + ".obj")
            objs.append(obj)
            try:
                _run(
                    [
                        "cl.exe",
                        *cxx_flags,
                        f"/DNANOPRINTF_USE_ALT_FORM_FLAG=1",
                        f"/DDOCTEST_CONFIG_SUPER_FAST_ASSERTS",
                        f"/DNANOPRINTF_USE_LARGE_FORMAT_SPECIFIERS={large_val}",
                        *(
                            ["/DNANOPRINTF_32_BIT_TESTS"]
                            if args.arch == 32
                            else []
                        ),
                        "/c",
                        f"/Fo{obj}",
                        src,
                    ],
                    verbose=args.verbose,
                )
            except subprocess.CalledProcessError:
                return False

        exe = build_dir / f"unit_tests_{variant}.exe"
        try:
            _run(
                [
                    "link.exe",
                    "/nologo",
                    f"/out:{exe}",
                    str(doctest_obj),
                    *(str(o) for o in objs),
                ],
                verbose=args.verbose,
            )
        except subprocess.CalledProcessError:
            return False

        try:
            _run([str(exe), "-m"], verbose=args.verbose)
        except subprocess.CalledProcessError:
            return False

    return True


def _build_compile_only(args: argparse.Namespace) -> bool:
    """Build compile-only targets (verify compilation, not run)."""
    build_dir = _SCRIPT_PATH / "build"
    build_dir.mkdir(parents=True, exist_ok=True)

    opt_flag = "/Od" if args.cfg == "Debug" else "/Os"

    targets: list[tuple[str, list[str], list[str]]] = [
        # (name, extra_cl_flags, source_files)
        (
            "npf_static",
            ["/nologo", opt_flag],
            ["tests/static_nanoprintf.c", "tests/static_main.c"],
        ),
        (
            "npf_include_multiple",
            ["/nologo", opt_flag, "/W4", "/WX", "/wd4464", "/wd4514", "/wd4710", "/wd4711"],
            ["tests/include_multiple.c"],
        ),
        (
            "use_npf_directly",
            ["/nologo", opt_flag, "/std:c++20", "/EHsc"],
            [
                "examples/use_npf_directly/your_project_nanoprintf.cc",
                "examples/use_npf_directly/main.cc",
            ],
        ),
        (
            "wrap_npf",
            ["/nologo", opt_flag, "/std:c++20", "/EHsc"],
            [
                "examples/wrap_npf/your_project_printf.cc",
                "examples/wrap_npf/main.cc",
            ],
        ),
    ]

    for name, flags, srcs in targets:
        exe = build_dir / f"{name}.exe"
        try:
            _run(
                ["cl.exe", *flags, f"/Fe{exe}", *srcs, "/link", "/nologo"],
                verbose=args.verbose,
            )
        except subprocess.CalledProcessError:
            return False

    return True


def main() -> int:
    """Parse args, build conformance + unit tests + compile-only targets."""
    os.chdir(_SCRIPT_PATH)
    args = _parse_args()

    print("=== Building conformance tests ===")
    if not _build_conformance(args):
        print("Conformance tests FAILED")
        return 1

    print("=== Building unit tests ===")
    if not _build_unit_tests(args):
        print("Unit tests FAILED")
        return 1

    print("=== Building compile-only targets ===")
    if not _build_compile_only(args):
        print("Compile-only targets FAILED")
        return 1

    return 0


if __name__ == "__main__":
    sys.exit(main())
