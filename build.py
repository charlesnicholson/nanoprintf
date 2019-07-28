import argparse
import os
import shutil
import subprocess
import stat
import sys
import tarfile
import urllib.request
import zipfile

SCRIPT_PATH = os.path.dirname(os.path.realpath(__file__))


def parse_args():
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
        '--build-32-bit', help='Compile in 32-bit mode', action='store_true')
    parser.add_argument(
        '--download',
        help='Download CMake and Ninja, don\'t use local copies',
        action='store_true')
    parser.add_argument('-v', help='verbose', action='store_true')
    return parser.parse_args()


def download_file(url, local_path, verbose):
    if verbose:
        print("Downloading:\n  Remote: {}\n  Local: {}".format(url, local_path))
    with urllib.request.urlopen(url) as rsp, open(local_path, 'wb') as file:
        shutil.copyfileobj(rsp, file)


def get_cmake(download, verbose):
    if not download:
        cmake = shutil.which('cmake')
        if cmake:
            return cmake

    cmake_prefix = 'cmake-3.14.5-{}-x86_64'.format(
        {'darwin': 'Darwin', 'linux': 'linux', 'win32': 'win64'}[sys.platform])
    cmake_local_dir = os.path.join(SCRIPT_PATH, 'external', 'cmake')
    cmake_file = cmake_prefix + '.tar.gz'
    cmake_local_tgz = os.path.join(cmake_local_dir, cmake_file)
    cmake_local_exe = os.path.join(
        cmake_local_dir,
        cmake_prefix,
        'CMake.app/Contents' if sys.platform == 'darwin' else '',
        'bin',
        'cmake')

    if not os.path.exists(cmake_local_exe):
        if not os.path.exists(cmake_local_tgz):
            os.makedirs(cmake_local_dir, exist_ok=True)
            download_file(
                'https://cmake.org/files/v3.14/{}'.format(cmake_file),
                cmake_local_tgz,
                verbose)
        with tarfile.open(cmake_local_tgz, 'r') as tar:
            tar.extractall(path=cmake_local_dir)

    return cmake_local_exe


def get_ninja(download, verbose):
    if not download:
        ninja = shutil.which('ninja')
        if ninja:
            return ninja

    ninja_local_dir = os.path.join(SCRIPT_PATH, 'external', 'ninja')
    ninja_file = 'ninja-{}.zip'.format({'darwin': 'mac',
                                        'linux': 'linux',
                                        'win32': 'win'}[sys.platform])
    ninja_local_zip = os.path.join(ninja_local_dir, ninja_file)
    ninja_local_exe = os.path.join(ninja_local_dir, 'ninja')

    if not os.path.exists(ninja_local_exe):
        if not os.path.exists(ninja_local_zip):
            os.makedirs(ninja_local_dir, exist_ok=True)
            download_file(
                'https://github.com/ninja-build/ninja/releases/download/v1.9.0/{}'.format(
                    ninja_file),
                ninja_local_zip,
                verbose)
        with zipfile.ZipFile(ninja_local_zip, 'r') as zip_file:
            zip_file.extractall(ninja_local_dir)
        os.chmod(ninja_local_exe, os.stat(
            ninja_local_exe).st_mode | stat.S_IEXEC)  # zipfile loses +x

    return ninja_local_exe


def configure_cmake(cmake_exe, ninja, args):
    build_path = os.path.join(SCRIPT_PATH, 'build', 'ninja', args.cfg)
    if os.path.exists(os.path.join(build_path, 'CMakeFiles')):
        return True

    os.makedirs(build_path)

    cmake_args = [cmake_exe,
                  SCRIPT_PATH,
                  '-G',
                  'Ninja',
                  '-DCMAKE_MAKE_PROGRAM={}'.format(ninja),
                  '-DCMAKE_BUILD_TYPE={}'.format(args.cfg),
                  '-DNPF_32BIT={}'.format('ON' if args.build_32_bit else 'OFF')]
    return not subprocess.run(cmake_args, cwd=build_path).returncode


def build_cmake(cmake_exe, args):
    build_path = os.path.join(SCRIPT_PATH, 'build', 'ninja', args.cfg)
    cmake_args = [cmake_exe, '--build', build_path]
    if args.v:
        cmake_args += ['--', '-v']
    return not subprocess.run(cmake_args).returncode


def main():
    args = parse_args()
    cmake = get_cmake(args.download, args.v)
    ninja = get_ninja(args.download, args.v)
    built_ok = configure_cmake(cmake, ninja, args) and build_cmake(cmake, args)
    return int(not built_ok)  # 0 is success


if __name__ == '__main__':
    sys.exit(main())
