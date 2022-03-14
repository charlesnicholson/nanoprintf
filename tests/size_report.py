"""Compile and analyze nanoprintf for different architectures."""
import argparse
import pathlib
import subprocess
import sys
import tempfile


def parse_args():
    """Parse and validate command-line arguments."""
    parser = argparse.ArgumentParser()
    parser.add_argument('-p',
                        '--platform',
                        required=True,
                        choices=('cm0', 'cm4', 'host'),
                        help='target platform')
    return parser.parse_args()


def git_root():
    """Return the root of the current file git repository."""
    cur = pathlib.Path(__file__).resolve()
    while cur != cur.parent:
        if (cur / '.git').is_dir():
            return cur
        cur = cur.parent

    raise ValueError(f'{__file__} not in git repo')


def build(platform, flags):
    """Build a nanoprintf implementation object for platform + flags."""
    cc_exe = 'cc' if platform == 'host' else 'arm-none-eabi-gcc'
    cc_cmd = [cc_exe, '-c', '-x', 'c', '-Os', f'-I{git_root()}', '-o', 'npf.o']
    cc_cmd += {'cm0': ['-mcpu=cortex-m0'],
               'cm4': ['-mcpu=cortex-m4', '-mfloat-abi=hard'],
               'host': []}[platform]
    cc_cmd += ['-DNANOPRINTF_IMPLEMENTATION'] + flags + ['-']
    nm_exe = 'nm' if platform == 'host' else 'arm-none-eabi-nm'
    nm_cmd = [nm_exe, '--print-size', '--size-sort', 'npf.o']

    print(' '.join(cc_cmd))
    print(' '.join(nm_cmd), flush=True)

    with tempfile.TemporaryDirectory() as temp_dir:
        subprocess.run(
            cc_cmd,
            check=True,
            cwd=temp_dir,
            input=rb'#include "nanoprintf.h"')
        return subprocess.run(
            nm_cmd,
            check=True,
            cwd=temp_dir,
            stdout=subprocess.PIPE).stdout.decode()


def measure(build_output):
    """Parse the results of nm to accumulate and print a total size."""
    total = 0
    for line in build_output.split('\n'):
        parts = [l for l in line.split() if l.strip()]
        if not parts:
            continue
        print(line)
        if parts[0] in ('u', 'U'):
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
    """Entry point"""
    args = parse_args()
    for cfg in _CONFIGS:
        print(f'Configuration "{cfg["name"]}":')
        measure(build(args.platform, cfg['flags']))
        print()
    return 0


if __name__ == '__main__':
    sys.exit(main())
