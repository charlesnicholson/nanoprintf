"""Build script for nanoprintf. Configures and runs CMake to build tests."""

import argparse
import os
import pathlib
import shutil
import subprocess
import stat
import sys
import tarfile
import urllib.request
import zipfile

_SCRIPT_PATH = pathlib.Path(__file__).resolve().parent
_NINJA_URL = "https://github.com/ninja-build/ninja/releases/download/v1.12.1/{}"
_CMAKE_VERSION = "4.0.1"
_CMAKE_URL = (
    "https://github.com/Kitware/CMake/releases/download/"
    f"v{_CMAKE_VERSION}/cmake-{_CMAKE_VERSION}" + "-{}.{}"
)


def _parse_args() -> argparse.Namespace:
    """Parse command-line arguments."""
    parser = argparse.ArgumentParser()
    parser.add_argument(
        "--cfg",
        choices=["Debug", "RelWithDebInfo", "Release"],
        default="Release",
        const="Release",
        nargs="?",
        help="CMake configuration",
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
    parser.add_argument(
        "--paland",
        help="Compile with Paland's printf conformance suite",
        action="store_true",
    )
    parser.add_argument(
        "--download",
        help="Download CMake and Ninja, don't use local copies",
        action="store_true",
    )
    parser.add_argument("--ubsan", action="store_true", help="Clang UB sanitizer")
    parser.add_argument("--asan", action="store_true", help="Clang addr sanitizer")
    parser.add_argument("-v", "--verbose", action="store_true", help="verbose")
    return parser.parse_args()


def download_file(url: str, local_path: pathlib.Path, verbose: bool) -> None:
    """Download a file from url to local_path."""
    if verbose:
        print(f"Downloading:\n  Remote: {url}\n  Local: {local_path}")
    with urllib.request.urlopen(url) as rsp, open(local_path, "wb") as file:
        shutil.copyfileobj(rsp, file)


def _get_cmake(download: bool, verbose: bool) -> pathlib.Path:
    """Return the path to system CMake, or download and unpack a local copy."""
    if not download:
        cmake = shutil.which("cmake")
        if cmake:
            return pathlib.Path(cmake)

    plat = {
        "darwin": "macos-universal",
        "linux": "linux-x86_64",
        "win32": "windows-x86_64",
    }[sys.platform]

    suffix = "zip" if sys.platform == "win32" else "tar.gz"

    cmake_prefix = f"cmake-{_CMAKE_VERSION}-{plat}"
    cmake_local_dir = _SCRIPT_PATH / "external/cmake"
    cmake_file = f"{cmake_prefix}.{suffix}"
    cmake_local_archive = cmake_local_dir / cmake_file
    cmake_local_exe = (
        cmake_local_dir
        / cmake_prefix
        / ("CMake.app/Contents" if sys.platform == "darwin" else "")
        / "bin/cmake"
    )

    if not cmake_local_exe.exists():
        if not cmake_local_archive.exists():
            cmake_local_dir.mkdir(parents=True, exist_ok=True)
            download_file(_CMAKE_URL.format(plat, suffix), cmake_local_archive, verbose)

        match suffix:
            case "tar.gz":
                with tarfile.open(cmake_local_archive, "r") as tar:
                    for member in tar.getmembers():
                        member_path = pathlib.Path(
                            cmake_local_dir / member.name
                        ).resolve()
                        if cmake_local_dir not in member_path.parents:
                            raise ValueError(
                                "Tar file contents move upwards past sandbox root"
                            )

                    tar.extractall(path=cmake_local_dir)

            case "zip":
                with zipfile.ZipFile(cmake_local_archive, "r") as zip_file:
                    zip_file.extractall(cmake_local_dir)

    return cmake_local_exe


def _get_ninja(download: bool, verbose: bool) -> pathlib.Path:
    """Return the path to system Ninja, or download and unpack a local copy."""
    if not download:
        ninja = shutil.which("ninja")
        if ninja:
            return pathlib.Path(ninja)

    ninja_local_dir = _SCRIPT_PATH / "external/ninja"
    plat = {"darwin": "mac", "linux": "linux", "win32": "win"}[sys.platform]
    ninja_file = f"ninja-{plat}.zip"
    ninja_local_zip = ninja_local_dir / ninja_file
    ninja_local_exe = ninja_local_dir / f"ninja{'.exe' if plat == 'win' else ''}"

    if not ninja_local_exe.exists():
        if not ninja_local_zip.exists():
            ninja_local_dir.mkdir(parents=True, exist_ok=True)
            download_file(_NINJA_URL.format(ninja_file), ninja_local_zip, verbose)

        with zipfile.ZipFile(ninja_local_zip, "r") as zip_file:
            zip_file.extractall(ninja_local_dir)

        os.chmod(ninja_local_exe, os.stat(ninja_local_exe).st_mode | stat.S_IEXEC)

    return ninja_local_exe


def _configure_cmake(
    cmake_exe: pathlib.Path, ninja: pathlib.Path, args: argparse.Namespace
) -> bool:
    """Prepare CMake for building nanoprintf tests under 'build/ninja/<cfg>'."""
    build_path = _SCRIPT_PATH / "build/ninja" / args.cfg
    if (build_path / "CMakeFiles").exists():
        return True

    sys.stdout.flush()
    build_path.mkdir(parents=True)

    cmake_args = [
        cmake_exe,
        _SCRIPT_PATH,
        "-G",
        "Ninja",
        "-DCMAKE_EXPORT_COMPILE_COMMANDS=ON",
        f"-DCMAKE_MAKE_PROGRAM={ninja}",
        f"-DCMAKE_BUILD_TYPE={args.cfg}",
        f"-DNPF_PALAND={'ON' if args.paland else 'OFF'}",
        f"-DNPF_32BIT={'ON' if args.arch == 32 else 'OFF'}",
        f"-DNPF_CLANG_ASAN={'ON' if args.asan else 'OFF'}",
        f"-DNPF_CLANG_UBSAN={'ON' if args.ubsan else 'OFF'}",
    ]

    try:
        return subprocess.run(cmake_args, cwd=build_path, check=True).returncode == 0
    except subprocess.CalledProcessError as cpe:
        return cpe.returncode == 0


def _build_cmake(cmake_exe: pathlib.Path, args: argparse.Namespace) -> bool:
    """Run CMake in build mode to compile and run the nanoprintf test suite."""
    sys.stdout.flush()
    build_path = _SCRIPT_PATH / "build/ninja" / args.cfg
    cmake_args = [cmake_exe, "--build", build_path] + (
        ["--", "-v"] if args.verbose else []
    )

    try:
        return subprocess.run(cmake_args, check=True).returncode == 0
    except subprocess.CalledProcessError as cpe:
        return cpe.returncode == 0


def main() -> int:
    """Parse args, find or get tools, configure CMake, build and run tests."""
    args = _parse_args()

    cmake = _get_cmake(args.download, args.verbose)
    if args.verbose:
        print(f"Found CMake at {cmake}")

    ninja = _get_ninja(args.download, args.verbose)
    if args.verbose:
        print(f"Found ninja at {ninja}")

    built_ok = _configure_cmake(cmake, ninja, args) and _build_cmake(cmake, args)
    return int(not built_ok)  # 0 is success


if __name__ == "__main__":
    sys.exit(main())
