# nanoprintf

[![Presubmit Checks](https://github.com/charlesnicholson/nanoprintf/workflows/Presubmit%20Checks/badge.svg)](https://github.com/charlesnicholson/nanoprintf/tree/main/.github/workflows)
[![](https://img.shields.io/badge/asan-clean-brightgreen)](https://en.wikipedia.org/wiki/AddressSanitizer)
[![](https://img.shields.io/badge/ubsan-clean-brightgreen)](https://clang.llvm.org/docs/UndefinedBehaviorSanitizer.html)
[![](https://img.shields.io/badge/pylint-10.0-brightgreen.svg)](https://www.pylint.org/)
[![](https://img.shields.io/badge/license-public_domain-brightgreen.svg)](https://github.com/charlesnicholson/nanoprintf/blob/master/LICENSE)
[![](https://img.shields.io/badge/license-0BSD-brightgreen)](https://github.com/charlesnicholson/nanoprintf/blob/master/LICENSE)

nanoprintf is an unencumbered implementation of snprintf and vsnprintf for embedded systems that, when fully enabled, aim for C11 standard compliance. The primary exceptions are `double` (they get casted to `float`), scientific notation (`%e`, `%g`, `%a`), and the conversions that require `wcrtomb` to exist. C23 binary integer output is optionally supported as per [N2630](http://www.open-std.org/jtc1/sc22/wg14/www/docs/n2630.pdf). Safety extensions for snprintf and vsnprintf can be optionally configured to return trimmed or fully-empty strings on buffer overflow events.

nanoprintf makes no memory allocations and uses less than 100 bytes of stack. It compiles to between *~760-2500 bytes of object code* on a Cortex-M0 architecture, depending on configuration.

All code is written in a minimal dialect of C99 for maximal compiler compatibility, compiles cleanly at the highest warning levels on clang + gcc + msvc, raises no issues from UBsan or Asan, and is exhaustively tested on 32-bit and 64-bit architectures. nanoprintf does include C standard headers but only uses them for C99 types and argument lists; no calls are made into stdlib / libc, with the exception of any internal double-to-float conversion ABI calls your compiler might emit. As usual, some Windows-specific headers are required if you're compiling natively for msvc.

nanoprintf is a [single header file](https://github.com/charlesnicholson/nanoprintf/blob/master/nanoprintf.h) in the style of the [stb libraries](https://github.com/nothings/stb). The rest of the repository is tests and scaffolding and not required for use.

nanoprintf is statically configurable so users can find a balance between size, compiler requirements, and feature set. Floating point conversion, "large" length modifiers, and size write-back are all configurable and are only compiled if explicitly requested, see [Configuration](https://github.com/charlesnicholson/nanoprintf#configuration) for details.

## Usage

Add the following code to one of your source files to compile the nanoprintf implementation:
```
// define your nanoprintf configuration macros here (see "Configuration" below)
#define NANOPRINTF_IMPLEMENTATION
#include "path/to/nanoprintf.h"
```

Then, in any file where you want to use nanoprintf, simply include the header and call the npf_ functions:
```
#include "nanoprintf.h"

void print_to_uart(void) {
  npf_pprintf(&my_uart_putc, NULL, "Hello %s%c %d %u %f\n", "worl", 'd', 1, 2, 3.f);
}

void print_to_buf(void *buf, unsigned len) {
  npf_snprintf(buf, len, "Hello %s", "world");
}
```

See the "[Use nanoprintf directly](https://github.com/charlesnicholson/nanoprintf/blob/master/examples/use_npf_directly/main.cpp)" and "[Wrap nanoprintf](https://github.com/charlesnicholson/nanoprintf/blob/master/examples/wrap_npf/main.cpp)" examples for more details.


## Motivation

I wanted a single-file public-domain drop-in printf that came in at under 1KB in the minimal configuration (bootloaders etc), and under 3KB with the floating-point bells and whistles enabled.

In firmware work, I generally want stdio's string formatting without the syscall or file descriptor layer requirements; they're almost never needed in tiny systems where you want to log into small buffers or emit directly to a bus. Also, many embedded stdio implementations are larger or slower than they need to be- this is important for bootloader work. If you don't need any of the syscalls or stdio bells + whistles, you can simply use nanoprintf and `nosys.specs` and slim down your build.

## Philosophy

This code is optimized for size, not readability or structure. Unfortunately modularity and "cleanliness" (whatever that means) adds overhead at this small scale, so most of the functionality and logic is pushed together into `npf_vpprintf`. This is not what normal embedded systems code should look like; it's `#ifdef` soup and hard to make sense of, and I apologize if you have to spelunk around in the implementation. Hopefully the various tests will serve as guide rails if you hack around in it.

Alternately, perhaps you're a significantly better programmer than I! In that case, please help me make this code smaller and cleaner without making the footprint larger, or nudge me in the right direction. :)

## API

nanoprintf has 4 main functions:
* `npf_snprintf`: Use like [snprintf](https://en.cppreference.com/w/c/io/fprintf).
* `npf_vsnprintf`: Use like [vsnprintf](https://en.cppreference.com/w/c/io/vfprintf) (`va_list` support).
* `npf_pprintf`: Use like [printf](https://en.cppreference.com/w/c/io/fprintf) with a per-character write callback (semihosting, UART, etc).
* `npf_vpprintf`: Use like `npf_pprintf` but takes a `va_list`.

The `pprintf` variations take a callback that receives the character to print and a user-provided context pointer.

Pass `NULL` or `nullptr` to `npf_[v]snprintf` to write nothing, and only return the length of the formatted string.

nanoprintf does *not* provide `printf` or `putchar` itself; those are seen as system-level services and nanoprintf is a utility library. nanoprintf is hopefully a good building block for rolling your own `printf`, though.

## Configuration

### Features
nanoprintf has the following static configuration flags.

* `NANOPRINTF_USE_FIELD_WIDTH_FORMAT_SPECIFIERS`: Set to `0` or `1`. Enables field width specifiers.
* `NANOPRINTF_USE_PRECISION_FORMAT_SPECIFIERS`: Set to `0` or `1`. Enables precision specifiers.
* `NANOPRINTF_USE_FLOAT_FORMAT_SPECIFIERS`: Set to `0` or `1`. Enables floating-point specifiers.
* `NANOPRINTF_USE_LARGE_FORMAT_SPECIFIERS`: Set to `0` or `1`. Enables oversized modifiers.
* `NANOPRINTF_USE_BINARY_FORMAT_SPECIFIERS`: Set to `0` or `1`. Enables binary specifiers.
* `NANOPRINTF_USE_WRITEBACK_FORMAT_SPECIFIERS`: Set to `0` or `1`. Enables `%n` for write-back.
* `NANOPRINTF_VISIBILITY_STATIC`: Optional define. Marks prototypes as `static` to sandbox nanoprintf.

If no configuration flags are specified, nanoprintf will default to "reasonable" embedded values in an attempt to be helpful: floats are enabled, but writeback, binary, and large formatters are disabled. If any configuration flags are explicitly specified, nanoprintf requires that all flags are explicitly specified.

If a disabled format specifier feature is used, no conversion will occur and the format specifier string simply will be printed instead.

### Sprintf Safety
By default, npf_snprintf and npf_vsnprintf behave according to the C Standard: the provided buffer will be filled but not overrun, though a null-terminator `0` byte will *not* be written at the end if the buffer is exhausted!

nanoprintf offers three options for configuring safety:
* Do nothing. User-provided buffers will not be null-terminated if exhausted.
* `NANOPRINTF_SNPRINTF_SAFE_TRIM_STRING_ON_OVERFLOW`: When exhausted, the final byte of the buffer will be overwritten with a null-terimator byte. This is similar in spirit to the behavior of [BSD strlcpy](https://linux.die.net/man/3/strlcpy).
* `NANOPRINTF_SNPRINTF_SAFE_EMPTY_STRING_ON_OVERFLOW`: When exhausted, the _first_ byte of the buffer will be overwritten with a null-terminator byte. This is similar in spirit to [Microsoft's snprintf_s](https://docs.microsoft.com/en-us/cpp/c-runtime-library/reference/snprintf-s-snprintf-s-l-snwprintf-s-snwprintf-s-l).

In any of the above cases, nanoprintf will still return the number of bytes that would have been written to the buffer, had there been enough room. This value does not account for the null-terminator byte, in accordance with the C Standard.

### Thread Safety
nanoprintf uses only stack memory and no concurrency primitives, so internally it is oblivious to its execution environment. This makes it safe to call from multiple execution contexts concurrently, or to interrupt a `npf_` call with another `npf_` call (say, an ISR or something). If you use `npf_pprintf` concurrently with the same `npf_putc` target, it's up to you to ensure correctness inside your callback. If you `npf_snprintf` from multiple threads to the same buffer, you will have an obvious data race.

## Formatting

Like `printf`, `nanoprintf` expects a conversion specification string of the following form:

`[flags][field width][.precision][length modifier][conversion specifier]`

* **Flags**

	None or more of the following:
	* `0`: Pad the field with leading zero characters.
	* `-`: Left-justify the conversion result in the field.
	* `+`: Signed conversions always begin with `+` or `-` characters.
	* ` `: (space) A space character is inserted if the first converted character is not a sign.
	* `#`: Writes extra characters (`0x` for hex, `.` for empty floats, '0' for empty octals, etc).
* **Field width** (if enabled)

	A number that specifies the total field width for the conversion, adds padding. If field width is `*`, the field width is read from the next vararg.
* **Precision** (if enabled)

	Prefixed with a `.`, a number that specifies the precision of the number or string. If precision is `*`, the precision is read from the next vararg.
* **Length modifier**

	None or more of the following:
	* `h`: Use `short` for integral and write-back vararg width.
	* `L`: Use `long double` for float vararg width (note: it will then be casted down to `float`)
	* `l`: Use `long`, `double`, or wide vararg width.
	* `hh`: Use `char` for integral and write-back vararg width.
	* `ll`: (large specifier) Use `long long` for integral and write-back vararg width.
	* `j`: (large specifier) Use the `[u]intmax_t` types for integral and write-back vararg width.
	* `z`: (large specifier) Use the `size_t` types for integral and write-back vararg width.
	* `t`: (large specifier) Use the `ptrdiff_t` types for integral and write-back vararg width.
* **Conversion specifier**

	Exactly one of the following:
	* `%`: Percent-sign literal
	* `c`: Character
	* `s`: Null-terminated strings
	* `i`/`d`: Signed integers
	* `u`: Unsigned integers
	* `o`: Unsigned octal integers
	* `x` / `X`: Unsigned hexadecimal integers
	* `p`: Pointers
	* `n`: Write the number of bytes written to the pointer vararg
	* `f`/`F`: Floating-point values
	* `b`/`B`: Binary integers

## Floating Point

Floating point conversion is performed by extracting the value into 64:64 fixed-point with an extra field that specifies the number of leading zero fractional digits before the first nonzero digit. No rounding is currently performed; values are simply truncated at the specified precision. This is done for simplicity, speed, and code footprint.

Because the float -> fixed code operates on the raw float value bits, no floating point operations are performed. This allows nanoprintf to efficiently format floats on soft-float architectures like Cortex-M0, and to function identically with or without optimizations like "fast math". Despite `nano` in the name, there's no way to do away with double entirely, since the C language standard says that floats are promoted to double any time they're passed into variadic argument lists. nanoprintf casts all doubles back down to floats before doing any conversions. No other single- or double- precision operations are performed.

## Limitations

No wide-character support exists: the `%lc` and `%ls` fields require that the arg be converted to a char array as if by a call to [wcrtomb](http://man7.org/linux/man-pages/man3/wcrtomb.3.html). When locale and character set conversions get involved, it's hard to keep the name "nano". Accordingly, `%lc` and `%ls` behave like `%c` and `%s`, respectively.

Currently the only supported float conversions are the decimal forms: `%f` and `%F`. Pull requests welcome!

## Measurement

The CI build is set up to use gcc and nm to measure the compiled size of every pull request. See the [Presubmit Checks](https://github.com/charlesnicholson/nanoprintf/actions/workflows/presubmit.yml) "size reports" job output for recent runs.

The following size measurements are taken against the Cortex-M0 build.

```
Configuration "Minimal":
arm-none-eabi-gcc -c -x c -Os -I/__w/nanoprintf/nanoprintf -o npf.o -mcpu=cortex-m0 -DNANOPRINTF_IMPLEMENTATION -DNANOPRINTF_USE_FIELD_WIDTH_FORMAT_SPECIFIERS=0 -DNANOPRINTF_USE_PRECISION_FORMAT_SPECIFIERS=0 -DNANOPRINTF_USE_FLOAT_FORMAT_SPECIFIERS=0 -DNANOPRINTF_USE_LARGE_FORMAT_SPECIFIERS=0 -DNANOPRINTF_USE_BINARY_FORMAT_SPECIFIERS=0 -DNANOPRINTF_USE_WRITEBACK_FORMAT_SPECIFIERS=0 -
arm-none-eabi-nm --print-size --size-sort npf.o
00000014 00000002 t npf_bufputc_nop
00000016 00000010 t npf_putc_cnt
00000000 00000014 t npf_bufputc
00000298 00000016 T npf_pprintf
000002e0 00000016 T npf_snprintf
000002ae 00000032 T npf_vsnprintf
00000026 00000272 T npf_vpprintf
Total size: 0x2f6 (758) bytes

Configuration "Binary":
arm-none-eabi-gcc -c -x c -Os -I/__w/nanoprintf/nanoprintf -o npf.o -mcpu=cortex-m0 -DNANOPRINTF_IMPLEMENTATION -DNANOPRINTF_USE_FIELD_WIDTH_FORMAT_SPECIFIERS=0 -DNANOPRINTF_USE_PRECISION_FORMAT_SPECIFIERS=0 -DNANOPRINTF_USE_FLOAT_FORMAT_SPECIFIERS=0 -DNANOPRINTF_USE_LARGE_FORMAT_SPECIFIERS=0 -DNANOPRINTF_USE_BINARY_FORMAT_SPECIFIERS=1 -DNANOPRINTF_USE_WRITEBACK_FORMAT_SPECIFIERS=0 -
arm-none-eabi-nm --print-size --size-sort npf.o
00000014 00000002 t npf_bufputc_nop
00000016 00000010 t npf_putc_cnt
00000000 00000014 t npf_bufputc
000002de 00000016 T npf_pprintf
00000328 00000016 T npf_snprintf
000002f4 00000034 T npf_vsnprintf
00000026 000002b8 T npf_vpprintf
Total size: 0x33e (830) bytes

Configuration "Field Width + Precision":
arm-none-eabi-gcc -c -x c -Os -I/__w/nanoprintf/nanoprintf -o npf.o -mcpu=cortex-m0 -DNANOPRINTF_IMPLEMENTATION -DNANOPRINTF_USE_FIELD_WIDTH_FORMAT_SPECIFIERS=1 -DNANOPRINTF_USE_PRECISION_FORMAT_SPECIFIERS=1 -DNANOPRINTF_USE_FLOAT_FORMAT_SPECIFIERS=0 -DNANOPRINTF_USE_LARGE_FORMAT_SPECIFIERS=0 -DNANOPRINTF_USE_BINARY_FORMAT_SPECIFIERS=0 -DNANOPRINTF_USE_WRITEBACK_FORMAT_SPECIFIERS=0 -
arm-none-eabi-nm --print-size --size-sort npf.o
00000014 00000002 t npf_bufputc_nop
00000016 00000010 t npf_putc_cnt
00000000 00000014 t npf_bufputc
00000546 00000016 T npf_pprintf
00000590 00000016 T npf_snprintf
0000055c 00000034 T npf_vsnprintf
00000026 00000520 T npf_vpprintf
Total size: 0x5a6 (1446) bytes

Configuration "Field Width + Precision + Binary":
arm-none-eabi-gcc -c -x c -Os -I/__w/nanoprintf/nanoprintf -o npf.o -mcpu=cortex-m0 -DNANOPRINTF_IMPLEMENTATION -DNANOPRINTF_USE_FIELD_WIDTH_FORMAT_SPECIFIERS=1 -DNANOPRINTF_USE_PRECISION_FORMAT_SPECIFIERS=1 -DNANOPRINTF_USE_FLOAT_FORMAT_SPECIFIERS=0 -DNANOPRINTF_USE_LARGE_FORMAT_SPECIFIERS=0 -DNANOPRINTF_USE_BINARY_FORMAT_SPECIFIERS=1 -DNANOPRINTF_USE_WRITEBACK_FORMAT_SPECIFIERS=0 -
arm-none-eabi-nm --print-size --size-sort npf.o
00000014 00000002 t npf_bufputc_nop
00000016 00000010 t npf_putc_cnt
00000000 00000014 t npf_bufputc
00000590 00000016 T npf_pprintf
000005d8 00000016 T npf_snprintf
000005a6 00000032 T npf_vsnprintf
00000026 0000056a T npf_vpprintf
Total size: 0x5ee (1518) bytes

Configuration "Float":
arm-none-eabi-gcc -c -x c -Os -I/__w/nanoprintf/nanoprintf -o npf.o -mcpu=cortex-m0 -DNANOPRINTF_IMPLEMENTATION -DNANOPRINTF_USE_FIELD_WIDTH_FORMAT_SPECIFIERS=0 -DNANOPRINTF_USE_PRECISION_FORMAT_SPECIFIERS=1 -DNANOPRINTF_USE_FLOAT_FORMAT_SPECIFIERS=1 -DNANOPRINTF_USE_LARGE_FORMAT_SPECIFIERS=0 -DNANOPRINTF_USE_BINARY_FORMAT_SPECIFIERS=0 -DNANOPRINTF_USE_WRITEBACK_FORMAT_SPECIFIERS=0 -
arm-none-eabi-nm --print-size --size-sort npf.o
00000014 00000002 t npf_bufputc_nop
00000016 00000010 t npf_putc_cnt
00000000 00000014 t npf_bufputc
0000059c 00000016 T npf_pprintf
000005e4 00000016 T npf_snprintf
000005b2 00000032 T npf_vsnprintf
00000026 00000576 T npf_vpprintf
Total size: 0x5fa (1530) bytes

Configuration "Everything":
arm-none-eabi-gcc -c -x c -Os -I/__w/nanoprintf/nanoprintf -o npf.o -mcpu=cortex-m0 -DNANOPRINTF_IMPLEMENTATION -DNANOPRINTF_USE_FIELD_WIDTH_FORMAT_SPECIFIERS=1 -DNANOPRINTF_USE_PRECISION_FORMAT_SPECIFIERS=1 -DNANOPRINTF_USE_FLOAT_FORMAT_SPECIFIERS=1 -DNANOPRINTF_USE_LARGE_FORMAT_SPECIFIERS=1 -DNANOPRINTF_USE_BINARY_FORMAT_SPECIFIERS=1 -DNANOPRINTF_USE_WRITEBACK_FORMAT_SPECIFIERS=1 -
arm-none-eabi-nm --print-size --size-sort npf.o
00000014 00000002 t npf_bufputc_nop
00000016 00000010 t npf_putc_cnt
00000000 00000014 t npf_bufputc
00000934 00000016 T npf_pprintf
0000097c 00000016 T npf_snprintf
0000094a 00000032 T npf_vsnprintf
00000026 0000090e T npf_vpprintf
Total size: 0x992 (2450) bytes
```

## Development

To get the environment and run tests:

1. Clone or fork this repository.
1. Run `./b` from the root (or `py -3 build.py` from the root, for Windows users)

This will build all of the unit, conformance, and compilation tests for your host environment. Any test failures will return a non-zero exit code.

The nanoprintf development environment uses [cmake](https://cmake.org/) and [ninja](https://ninja-build.org/). If you have these in your path, `./b` will use them. If not, `./b` will download and deploy them into `path/to/your/nanoprintf/external`.

nanoprintf uses GitHub Actions for all continuous integration builds. The GitHub Linux builds use [this](https://github.com/charlesnicholson/docker-images/packages/751874) Docker image from [my Docker repository](https://github.com/charlesnicholson/docker-images).

The matrix builds [Debug, Release] x [32-bit, 64-bit] x [Mac, Windows, Linux] x [gcc, clang, msvc], minus the 32-bit clang Mac configurations.

One test suite is a fork from the [printf test suite](), which is MIT licensed. It exists as a submodule for licensing purposes- nanoprintf is public domain, so this particular test suite is optional and excluded by default. To build it, retrieve it by updating submodules and add the `--paland` flag to your `./b` invocation. It is not required to use nanoprintf at all.

## Acknowledgments

I implemented Float-to-int conversion using the ideas from [Wojciech Muła](mailto:zdjęcia@garnek.pl)'s [float -> 64:64 fixed algorithm](http://0x80.pl/notesen/2015-12-29-float-to-string.html).

I ported the [printf test suite](https://github.com/eyalroz/printf/blob/master/test/test_suite.cpp) to nanoprintf. It was originally from the [mpaland printf project](https://github.com/mpaland/printf) codebase but adopted and improved by [Eyal Rozenberg](https://github.com/eyalroz) and others. (Nanoprintf has many of its own tests, but these are also very thorough and very good!)

The binary implementation is based on the requirements specified by [Jörg Wunsch](https://github.com/dl8dtl)'s [N2630 proposal](http://www.open-std.org/jtc1/sc22/wg14/www/docs/n2630.pdf), hopefully to be accepted into C23!
