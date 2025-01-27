# nanoprintf

[![Presubmit Checks](https://github.com/charlesnicholson/nanoprintf/workflows/Presubmit%20Checks/badge.svg)](https://github.com/charlesnicholson/nanoprintf/tree/main/.github/workflows)
[![](https://img.shields.io/badge/asan-clean-brightgreen)](https://en.wikipedia.org/wiki/AddressSanitizer)
[![](https://img.shields.io/badge/ubsan-clean-brightgreen)](https://clang.llvm.org/docs/UndefinedBehaviorSanitizer.html)
[![](https://img.shields.io/badge/pylint-10.0-brightgreen.svg)](https://www.pylint.org/)
[![](https://img.shields.io/badge/license-public_domain-brightgreen.svg)](https://github.com/charlesnicholson/nanoprintf/blob/master/LICENSE)
[![](https://img.shields.io/badge/license-0BSD-brightgreen)](https://github.com/charlesnicholson/nanoprintf/blob/master/LICENSE)

nanoprintf is an unencumbered implementation of snprintf and vsnprintf for embedded systems that, when fully enabled, aim for C11 standard compliance. The primary exceptions are scientific notation (`%e`, `%g`, `%a`), and locale conversions that require `wcrtomb` to exist. C23 binary integer output is optionally supported as per [N2630](http://www.open-std.org/jtc1/sc22/wg14/www/docs/n2630.pdf). Safety extensions for snprintf and vsnprintf can be optionally configured to return trimmed or fully-empty strings on buffer overflow events.

Additionally, nanoprintf can be used to parse printf-style format strings to extract the various parameters and conversion specifiers, without doing any actual text formatting.

nanoprintf makes no memory allocations and uses less than 100 bytes of stack. It compiles to between *~740-2640 bytes of object code* on a Cortex-M0 architecture, depending on configuration.

All code is written in a minimal dialect of C99 for maximal compiler compatibility, compiles cleanly at the highest warning levels on clang + gcc + msvc, raises no issues from UBsan or Asan, and is exhaustively tested on 32-bit and 64-bit architectures. nanoprintf does include C standard headers but only uses them for C99 types and argument lists; no calls are made into stdlib / libc, with the exception of any internal large integer arithmetic calls your compiler might emit. As usual, some Windows-specific headers are required if you're compiling natively for msvc.

nanoprintf is a [single header file](https://github.com/charlesnicholson/nanoprintf/blob/master/nanoprintf.h) in the style of the [stb libraries](https://github.com/nothings/stb). The rest of the repository is tests and scaffolding and not required for use.

nanoprintf is statically configurable so users can find a balance between size, compiler requirements, and feature set. Floating-point conversion, "large" length modifiers, and size write-back are all configurable and are only compiled if explicitly requested, see [Configuration](https://github.com/charlesnicholson/nanoprintf#configuration) for details.

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

See the "[Use nanoprintf directly](https://github.com/charlesnicholson/nanoprintf/blob/master/examples/use_npf_directly/main.cc)" and "[Wrap nanoprintf](https://github.com/charlesnicholson/nanoprintf/blob/master/examples/wrap_npf/main.cc)" examples for more details.


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

### Return Values

The nanoprintf functions all return the same value: the number of characters that were either sent to the callback (for npf_pprintf) or the number of characters that would have been written to the buffer provided sufficient space. The null-terminator 0 byte is not part of the count.

The C Standard allows for the printf functions to return negative values in case string or character encodings can not be performed, or if the output stream encounters EOF. Since nanoprintf is oblivious to OS resources like files, and does not support the `l` length modifier for `wchar_t` support, any runtime errors are either internal bugs (please report!) or incorrect usage. Because of this, nanoprintf only returns non-negative values representing how many bytes the formatted string contains (again, minus the null-terminator byte).

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

### Floating-Point Conversion
nanoprintf has the following floating-point specific configuration defines.

* `NANOPRINTF_CONVERSION_BUFFER_SIZE`: Optional, defaults to `23`. Sets the size of a character buffer used for storing the converted value. Set to a larger number to enable printing of floating-point numbers with more characters. The buffer size does include the integer part, the fraction part and the decimal separator, but does not include the sign and the padding characters. If the number does not fit into buffer, an `err` is printed. Be careful with large sizes as the conversion buffer is allocated on stack memory.
* `NANOPRINTF_CONVERSION_FLOAT_TYPE`: Optional, defaults to `unsigned int`. Sets the integer type used for float conversion algorithm, which determines the conversion accuracy. Can be set to any unsigned integer type, like for example `uint64_t` or `uint8_t`.

### Sprintf Safety
By default, npf_snprintf and npf_vsnprintf behave according to the C Standard: the provided buffer will be filled but not overrun. If the string would have overrun the buffer, a null-terminator byte will be written to the final byte of the buffer. If the buffer is `null` or zero-sized, no bytes will be written.

If you define `NANOPRINTF_SNPRINTF_SAFE_EMPTY_STRING_ON_OVERFLOW` and your string is larger than your buffer, the _first_ byte of the buffer will be overwritten with a null-terminator byte. This is similar in spirit to [Microsoft's snprintf_s](https://docs.microsoft.com/en-us/cpp/c-runtime-library/reference/snprintf-s-snprintf-s-l-snwprintf-s-snwprintf-s-l).

In all cases, nanoprintf will return the number of bytes that would have been written to the buffer, had there been enough room. This value does not account for the null-terminator byte, in accordance with the C Standard.

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
	* `L`: Use `long double` for float vararg width (note: it will then be casted down to `double`)
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
	* `f`/`F`: Floating-point decimal
	* `e`/`E`: Floating-point scientific (unimplemented, prints float decimal)
	* `g`/`G`: Floating-point shortest (unimplemented, prints float decimal)
	* `a`/`A`: Floating-point hex (unimplemented, prints float decimal)
	* `b`/`B`: Binary integers

## Floating-Point

Floating-point conversion is performed by extracting the integer and fraction parts of the number into two separate integer variables. For each part the exponent is then scaled from base-2 to base-10 by iteratively multiplying and dividing the mantissa by 2 and 5 appropriately. The order of the scaling operations is selected dynamically (depending on value) to retain as much of the most significant bits of the mantissa as possible. The further the value is away from the decimal separator, the more of an error the scaling will accumulate. With a conversion integer type width of `N` bits on average the algorithm retains `N - log2(5)` or `N - 2.322` bits of accuracy. In addition integer parts up to `2 ^^ N - 1` and fraction parts with up to `N - 2.322` bits after the decimal separator are converted perfectly without loosing any bits.

Because the float -> fixed code operates on the raw float value bits, no floating-point operations are performed. This allows nanoprintf to efficiently format floats on soft-float architectures like Cortex-M0, to function identically with or without optimizations like "fast math", and to minimize the code footprint.

The `%e`/`%E`, `%a`/`%A`, and `%g`/`%G` specifiers are parsed but not formatted. If used, the output will be identical to if `%f`/`%F` was used. Pull requests welcome! :)

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
00000046 00000002 t npf_bufputc_nop
00000048 00000010 t npf_putc_cnt
00000032 00000014 t npf_bufputc
00000270 00000016 T npf_pprintf
000002cc 00000016 T npf_snprintf
00000000 00000032 t npf_utoa_rev
00000286 00000046 T npf_vsnprintf
00000058 00000218 T npf_vpprintf
Total size: 0x2e2 (738) bytes

Configuration "Binary":
arm-none-eabi-gcc -c -x c -Os -I/__w/nanoprintf/nanoprintf -o npf.o -mcpu=cortex-m0 -DNANOPRINTF_IMPLEMENTATION -DNANOPRINTF_USE_FIELD_WIDTH_FORMAT_SPECIFIERS=0 -DNANOPRINTF_USE_PRECISION_FORMAT_SPECIFIERS=0 -DNANOPRINTF_USE_FLOAT_FORMAT_SPECIFIERS=0 -DNANOPRINTF_USE_LARGE_FORMAT_SPECIFIERS=0 -DNANOPRINTF_USE_BINARY_FORMAT_SPECIFIERS=1 -DNANOPRINTF_USE_WRITEBACK_FORMAT_SPECIFIERS=0 -
arm-none-eabi-nm --print-size --size-sort npf.o
00000046 00000002 t npf_bufputc_nop
00000048 00000010 t npf_putc_cnt
00000032 00000014 t npf_bufputc
000002a8 00000016 T npf_pprintf
00000304 00000016 T npf_snprintf
00000000 00000032 t npf_utoa_rev
000002be 00000046 T npf_vsnprintf
00000058 00000250 T npf_vpprintf
Total size: 0x31a (794) bytes

Configuration "Field Width + Precision":
arm-none-eabi-gcc -c -x c -Os -I/__w/nanoprintf/nanoprintf -o npf.o -mcpu=cortex-m0 -DNANOPRINTF_IMPLEMENTATION -DNANOPRINTF_USE_FIELD_WIDTH_FORMAT_SPECIFIERS=1 -DNANOPRINTF_USE_PRECISION_FORMAT_SPECIFIERS=1 -DNANOPRINTF_USE_FLOAT_FORMAT_SPECIFIERS=0 -DNANOPRINTF_USE_LARGE_FORMAT_SPECIFIERS=0 -DNANOPRINTF_USE_BINARY_FORMAT_SPECIFIERS=0 -DNANOPRINTF_USE_WRITEBACK_FORMAT_SPECIFIERS=0 -
arm-none-eabi-nm --print-size --size-sort npf.o
00000046 00000002 t npf_bufputc_nop
00000048 00000010 t npf_putc_cnt
00000032 00000014 t npf_bufputc
000004fe 00000016 T npf_pprintf
0000055c 00000016 T npf_snprintf
00000000 00000032 t npf_utoa_rev
00000514 00000048 T npf_vsnprintf
00000058 000004a6 T npf_vpprintf
Total size: 0x572 (1394) bytes

Configuration "Field Width + Precision + Binary":
arm-none-eabi-gcc -c -x c -Os -I/__w/nanoprintf/nanoprintf -o npf.o -mcpu=cortex-m0 -DNANOPRINTF_IMPLEMENTATION -DNANOPRINTF_USE_FIELD_WIDTH_FORMAT_SPECIFIERS=1 -DNANOPRINTF_USE_PRECISION_FORMAT_SPECIFIERS=1 -DNANOPRINTF_USE_FLOAT_FORMAT_SPECIFIERS=0 -DNANOPRINTF_USE_LARGE_FORMAT_SPECIFIERS=0 -DNANOPRINTF_USE_BINARY_FORMAT_SPECIFIERS=1 -DNANOPRINTF_USE_WRITEBACK_FORMAT_SPECIFIERS=0 -
arm-none-eabi-nm --print-size --size-sort npf.o
00000046 00000002 t npf_bufputc_nop
00000048 00000010 t npf_putc_cnt
00000032 00000014 t npf_bufputc
00000560 00000016 T npf_pprintf
000005bc 00000016 T npf_snprintf
00000000 00000032 t npf_utoa_rev
00000576 00000046 T npf_vsnprintf
00000058 00000508 T npf_vpprintf
Total size: 0x5d2 (1490) bytes

Configuration "Float":
arm-none-eabi-gcc -c -x c -Os -I/__w/nanoprintf/nanoprintf -o npf.o -mcpu=cortex-m0 -DNANOPRINTF_IMPLEMENTATION -DNANOPRINTF_USE_FIELD_WIDTH_FORMAT_SPECIFIERS=0 -DNANOPRINTF_USE_PRECISION_FORMAT_SPECIFIERS=1 -DNANOPRINTF_USE_FLOAT_FORMAT_SPECIFIERS=1 -DNANOPRINTF_USE_LARGE_FORMAT_SPECIFIERS=0 -DNANOPRINTF_USE_BINARY_FORMAT_SPECIFIERS=0 -DNANOPRINTF_USE_WRITEBACK_FORMAT_SPECIFIERS=0 -
arm-none-eabi-nm --print-size --size-sort npf.o
00000046 00000002 t npf_bufputc_nop
00000048 00000010 t npf_putc_cnt
00000032 00000014 t npf_bufputc
00000618 00000016 T npf_pprintf
00000674 00000016 T npf_snprintf
00000000 00000032 t npf_utoa_rev
0000062e 00000046 T npf_vsnprintf
00000058 000005c0 T npf_vpprintf
Total size: 0x68a (1674) bytes

Configuration "Everything":
arm-none-eabi-gcc -c -x c -Os -I/__w/nanoprintf/nanoprintf -o npf.o -mcpu=cortex-m0 -DNANOPRINTF_IMPLEMENTATION -DNANOPRINTF_USE_FIELD_WIDTH_FORMAT_SPECIFIERS=1 -DNANOPRINTF_USE_PRECISION_FORMAT_SPECIFIERS=1 -DNANOPRINTF_USE_FLOAT_FORMAT_SPECIFIERS=1 -DNANOPRINTF_USE_LARGE_FORMAT_SPECIFIERS=1 -DNANOPRINTF_USE_BINARY_FORMAT_SPECIFIERS=1 -DNANOPRINTF_USE_WRITEBACK_FORMAT_SPECIFIERS=1 -
arm-none-eabi-nm --print-size --size-sort npf.o
0000005a 00000002 t npf_bufputc_nop
0000005c 00000010 t npf_putc_cnt
00000046 00000014 t npf_bufputc
000009da 00000016 T npf_pprintf
00000a38 00000016 T npf_snprintf
00000000 00000046 t npf_utoa_rev
000009f0 00000048 T npf_vsnprintf
0000006c 0000096e T npf_vpprintf
Total size: 0xa4e (2638) bytes
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

The basic idea of float-to-int conversion was inspired by [Wojciech Muła](mailto:zdjęcia@garnek.pl)'s [float -> 64:64 fixed algorithm](http://0x80.pl/notesen/2015-12-29-float-to-string.html) and extended further by adding dynamic scaling and configurable integer width by [Oskars Rubenis](https://github.com/Okarss).

I ported the [printf test suite](https://github.com/eyalroz/printf/blob/master/test/test_suite.cpp) to nanoprintf. It was originally from the [mpaland printf project](https://github.com/mpaland/printf) codebase but adopted and improved by [Eyal Rozenberg](https://github.com/eyalroz) and others. (Nanoprintf has many of its own tests, but these are also very thorough and very good!)

The binary implementation is based on the requirements specified by [Jörg Wunsch](https://github.com/dl8dtl)'s [N2630 proposal](http://www.open-std.org/jtc1/sc22/wg14/www/docs/n2630.pdf), hopefully to be accepted into C23!
