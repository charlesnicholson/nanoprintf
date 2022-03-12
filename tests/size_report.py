"""Accumulate size output from nm and print the total."""
import argparse
import pathlib
import subprocess
import sys
import tempfile


def _parse_args():
    parser = argparse.ArgumentParser()
    parser.add_argument('-p',
                        '--platform',
                        required=True,
                        choices=('cm0', 'cm4', 'host'),
                        help='target platform')
    parser.add_argument('-v',
                        '--verbose',
                        action='store_true',
                        help='verbose')

    return parser.parse_args()


def _repo_root():
    """Return the root of the current file git repository"""
    cur = pathlib.Path(__file__).resolve()
    while cur != cur.parent:
        if (cur / '.git').is_dir():
            return cur
        cur = cur.parent

    raise ValueError(f'{__file__} not in git repo')


_REPO_ROOT = _repo_root()
_C_FILE = rb'''
#define NANOPRINTF_IMPLEMENTATION
#include "nanoprintf.h"
'''


def build(platform, flags):
    cc = 'cc' if platform == 'host' else 'arm-none-eabi-gcc'
    nm = 'nm' if platform == 'host' else 'arm-none-eabi-nm'

    compile = [cc, '-c', '-x', 'c', '-Os']
    compile += {'cm0': ['-mcpu=cortex-m0'],
                'cm4': ['-mcpu=cortex-m4', '-mfloat-abi=hard'],
                'host': []}[platform]
    compile += flags
    compile += [f'-I{_REPO_ROOT}', '-o', 'npf.o', '-']
    dump = [nm, '--print-size', '--size-sort', 'npf.o']

    print(' '.join(compile))
    print(' '.join(dump))
    sys.stdout.flush()

    with tempfile.TemporaryDirectory() as td:
        subprocess.run(compile, check=True, cwd=td, input=_C_FILE)
        return subprocess.run(
            dump,
            check=True,
            cwd=td,
            stdout=subprocess.PIPE).stdout.decode()


def measure(build_output):
    """Parse the results of nm to accumulate and print a total size."""
    total = 0
    for line in build_output.split('\n'):
        parts = [l for l in line.split() if l.strip()]
        if parts:
            print(line)
        if not parts or parts[0] in ('u', 'U'):
            continue
        if len(parts) >= 3 and parts[2] not in ('t', 'T'):
            continue
        total += int(line.split()[1], 16)
    print(f'Total size: 0x{total:x} ({total}) bytes')


_CONFIGS = [
    {'name': 'Minimal', 'flags': [
        '-DNANOPRINTF_USE_FIELD_WIDTH_FORMAT_SPECIFIERS=0',
        '-DNANOPRINTF_USE_PRECISION_FORMAT_SPECIFIERS=0',
        '-DNANOPRINTF_USE_FLOAT_FORMAT_SPECIFIERS=0',
        '-DNANOPRINTF_USE_LARGE_FORMAT_SPECIFIERS=0',
        '-DNANOPRINTF_USE_BINARY_FORMAT_SPECIFIERS=0',
        '-DNANOPRINTF_USE_WRITEBACK_FORMAT_SPECIFIERS=0',
    ]},
    {'name': 'Binary', 'flags': [
        '-DNANOPRINTF_USE_FIELD_WIDTH_FORMAT_SPECIFIERS=0',
        '-DNANOPRINTF_USE_PRECISION_FORMAT_SPECIFIERS=0',
        '-DNANOPRINTF_USE_FLOAT_FORMAT_SPECIFIERS=0',
        '-DNANOPRINTF_USE_LARGE_FORMAT_SPECIFIERS=0',
        '-DNANOPRINTF_USE_BINARY_FORMAT_SPECIFIERS=1',
        '-DNANOPRINTF_USE_WRITEBACK_FORMAT_SPECIFIERS=0',
    ]},
    {'name': 'Field Width + Precision', 'flags': [
        '-DNANOPRINTF_USE_FIELD_WIDTH_FORMAT_SPECIFIERS=1',
        '-DNANOPRINTF_USE_PRECISION_FORMAT_SPECIFIERS=1',
        '-DNANOPRINTF_USE_FLOAT_FORMAT_SPECIFIERS=0',
        '-DNANOPRINTF_USE_LARGE_FORMAT_SPECIFIERS=0',
        '-DNANOPRINTF_USE_BINARY_FORMAT_SPECIFIERS=0',
        '-DNANOPRINTF_USE_WRITEBACK_FORMAT_SPECIFIERS=0',
    ]},
    {'name': 'Field Width + Precision + Binary', 'flags': [
        '-DNANOPRINTF_USE_FIELD_WIDTH_FORMAT_SPECIFIERS=1',
        '-DNANOPRINTF_USE_PRECISION_FORMAT_SPECIFIERS=1',
        '-DNANOPRINTF_USE_FLOAT_FORMAT_SPECIFIERS=0',
        '-DNANOPRINTF_USE_LARGE_FORMAT_SPECIFIERS=0',
        '-DNANOPRINTF_USE_BINARY_FORMAT_SPECIFIERS=1',
        '-DNANOPRINTF_USE_WRITEBACK_FORMAT_SPECIFIERS=0',
    ]},
    {'name': 'Float', 'flags': [
        '-DNANOPRINTF_USE_FIELD_WIDTH_FORMAT_SPECIFIERS=0',
        '-DNANOPRINTF_USE_PRECISION_FORMAT_SPECIFIERS=1',
        '-DNANOPRINTF_USE_FLOAT_FORMAT_SPECIFIERS=1',
        '-DNANOPRINTF_USE_LARGE_FORMAT_SPECIFIERS=0',
        '-DNANOPRINTF_USE_BINARY_FORMAT_SPECIFIERS=0',
        '-DNANOPRINTF_USE_WRITEBACK_FORMAT_SPECIFIERS=0',
    ]},
    {'name': 'Everything', 'flags': [
        '-DNANOPRINTF_USE_FIELD_WIDTH_FORMAT_SPECIFIERS=1',
        '-DNANOPRINTF_USE_PRECISION_FORMAT_SPECIFIERS=1',
        '-DNANOPRINTF_USE_FLOAT_FORMAT_SPECIFIERS=1',
        '-DNANOPRINTF_USE_LARGE_FORMAT_SPECIFIERS=1',
        '-DNANOPRINTF_USE_BINARY_FORMAT_SPECIFIERS=1',
        '-DNANOPRINTF_USE_WRITEBACK_FORMAT_SPECIFIERS=1',
    ]},
]


def main():
    args = _parse_args()
    if args.verbose:
        print(f'{__file__}:')
        for arg in vars(args):
            print(f'  {arg}: {getattr(args, arg)}')

    for cfg in _CONFIGS:
        print(cfg['name'])
        measure(build(args.platform, cfg['flags']))
        print()
    return 0


if __name__ == '__main__':
    sys.exit(main())
