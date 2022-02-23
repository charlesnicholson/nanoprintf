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

SCRIPT_PATH = pathlib.Path(__file__).resolve().parent

NINJA_URL = 'https://github.com/ninja-build/ninja/releases/download/v1.10.2/{}'
CMAKE_URL = 'https://cmake.org/files/v3.22/{}'


def parse_args():
    """Parse command-line arguments."""
    parser = argparse.ArgumentParser()
    parser.add_argument(
        '--cfg',
        choices=[
            'Debug',
            'RelWithDebInfo',
            'Release'],
        default='Release',
        const='Release',
        nargs='?',
        help='CMake configuration')
    parser.add_argument(
        '--arch',
        type=int,
        choices=(32, 64),
        default=64,
        const=64,
        nargs='?',
        help='Target architecture')
    parser.add_argument(
        '--paland',
        help='Compile with Paland\'s printf conformance suite',
        action='store_true')
    parser.add_argument(
        '--download',
        help='Download CMake and Ninja, don\'t use local copies',
        action='store_true')
    parser.add_argument('--ubsan', action='store_true', help='Clang UB sanitizer')
    parser.add_argument('--asan', action='store_true', help='Clang addr sanitizer')
    parser.add_argument('-v', '--verbose', action='store_true', help='verbose')
    return parser.parse_args()


def download_file(url, local_path, verbose):
    """Download a file from url to local_path."""
    if verbose:
        print(f'Downloading:\n  Remote: {url}\n  Local: {local_path}')
    with urllib.request.urlopen(url) as rsp, open(local_path, 'wb') as file:
        shutil.copyfileobj(rsp, file)


def get_cmake(download, verbose):
    """Return the path to system CMake, or download and unpack a local copy."""
    if not download:
        cmake = shutil.which('cmake')
        if cmake:
            return cmake

    plat = {
        'darwin': 'macos-universal',
        'linux': 'linux-x86_64',
        'win32': 'win64-x86_64'}[
        sys.platform]
    cmake_prefix = f'cmake-3.22.2-{plat}'
    cmake_local_dir = SCRIPT_PATH / 'external' / 'cmake'
    cmake_file = f'{cmake_prefix}.tar.gz'
    cmake_local_tgz = cmake_local_dir / cmake_file
    cmake_local_exe = cmake_local_dir / cmake_prefix / \
        ('CMake.app/Contents' if sys.platform == 'darwin' else '') / 'bin' / 'cmake'

    if not cmake_local_exe.exists():
        if not cmake_local_tgz.exists():
            cmake_local_dir.mkdir(parents=True, exist_ok=True)
            download_file(
                CMAKE_URL.format(cmake_file),
                cmake_local_tgz,
                verbose)
        with tarfile.open(cmake_local_tgz, 'r') as tar:
            tar.extractall(path=cmake_local_dir)

    return cmake_local_exe


def get_ninja(download, verbose):
    """Return the path to system Ninja, or download and unpack a local copy."""
    if not download:
        ninja = shutil.which('ninja')
        if ninja:
            return ninja

    ninja_local_dir = SCRIPT_PATH / 'external' / 'ninja'
    plat = {'darwin': 'mac', 'linux': 'linux', 'win32': 'win'}[sys.platform]
    ninja_file = f'ninja-{plat}.zip'
    ninja_local_zip = ninja_local_dir / ninja_file
    ninja_local_exe = ninja_local_dir / f'ninja{".exe" if plat == "win" else ""}'

    if not ninja_local_exe.exists():
        if not ninja_local_zip.exists():
            ninja_local_dir.mkdir(parents=True, exist_ok=True)
            download_file(
                NINJA_URL.format(ninja_file),
                ninja_local_zip,
                verbose)
        with zipfile.ZipFile(ninja_local_zip, 'r') as zip_file:
            zip_file.extractall(ninja_local_dir)
        os.chmod(ninja_local_exe, os.stat(
            ninja_local_exe).st_mode | stat.S_IEXEC)

    return ninja_local_exe


def configure_cmake(cmake_exe, ninja, args):
    """Prepare CMake for building nanoprintf tests under 'build/ninja/<cfg>'."""
    build_path = SCRIPT_PATH / 'build' / 'ninja' / args.cfg
    if (build_path / 'CMakeFiles').exists():
        return True

    sys.stdout.flush()
    build_path.mkdir(parents=True)

    cmake_args = [cmake_exe,
                  SCRIPT_PATH,
                  '-G',
                  'Ninja',
                  '-DCMAKE_EXPORT_COMPILE_COMMANDS=ON',
                  f'-DCMAKE_MAKE_PROGRAM={ninja}',
                  f'-DCMAKE_BUILD_TYPE={args.cfg}',
                  f'-DNPF_PALAND={"ON" if args.paland else "OFF"}',
                  f'-DNPF_32BIT={"ON" if args.arch == 32 else "OFF"}',
                  f'-DNPF_CLANG_ASAN={"ON" if args.asan else "OFF"}',
                  f'-DNPF_CLANG_UBSAN={"ON" if args.ubsan else "OFF"}']
    try:
        return subprocess.run(
            cmake_args,
            cwd=build_path,
            check=True).returncode == 0
    except subprocess.CalledProcessError as cpe:
        return cpe.returncode == 0


def build_cmake(cmake_exe, args):
    """Run CMake in build mode to compile and run the nanoprintf test suite."""
    sys.stdout.flush()
    build_path = SCRIPT_PATH / 'build' / 'ninja' / args.cfg
    cmake_args = [cmake_exe, '--build', build_path] + \
        (['--', '-v'] if args.verbose else [])
    try:
        return subprocess.run(cmake_args, check=True).returncode == 0
    except subprocess.CalledProcessError as cpe:
        return cpe.returncode == 0


def main():
    """Parse args, find or get tools, configure CMake, build and run tests."""
    args = parse_args()

    cmake = get_cmake(args.download, args.verbose)
    if args.verbose:
        print(f'Found CMake at {cmake}')

    ninja = get_ninja(args.download, args.verbose)
    if args.verbose:
        print(f'Found ninja at {ninja}')

    built_ok = configure_cmake(cmake, ninja, args) and build_cmake(cmake, args)
    return int(not built_ok)  # 0 is success


if __name__ == '__main__':
    sys.exit(main())
