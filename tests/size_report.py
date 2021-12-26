"""Accumulate size output from nm and print the total."""
import sys

def main():
    total = 0
    for line in sys.stdin:
        print(line, end='')
        total += int(line.split()[1], 16)

    print(f'Total size: 0x{total:x} ({total}) bytes')
    return 0

if __name__ == '__main__':
    sys.exit(main())
