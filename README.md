# nanoprintf

[![Presubmit Checks](https://github.com/charlesnicholson/nanoprintf/workflows/Presubmit%20Checks/badge.svg)](https://github.com/charlesnicholson/nanoprintf/tree/master/.github/workflows) [![](https://img.shields.io/badge/pylint-10.0-brightgreen.svg)](https://www.pylint.org/) [![](https://img.shields.io/badge/license-public_domain-brightgreen.svg)](https://github.com/charlesnicholson/nanoprintf/blob/master/LICENSE)

nanoprintf is an implementation of snprintf and vsnprintf for embedded systems that, when fully enabled, aims for C11 standard compliance.

nanoprintf makes no memory allocations and uses less than 100 bytes of stack. nanoprintf compiles to somewhere between 1-3KB of code on a Cortex-M architecture.

nanoprintf is a [single header file](https://github.com/charlesnicholson/nanoprintf/blob/master/nanoprintf.h) in the style of the [stb libraries](https://github.com/nothings/stb). The rest of the repository is tests and scaffolding and not required for use.

nanoprintf is written in a minimal dialect of C99 for maximal compiler compatibility, and compiles cleanly at the highest warning levels on clang, gcc, and msvc in both 32- and 64-bit modes. It's _really_ hard to write portable C89 code, btw, when you don't have any guarantee about what integral type to use to hold a converted pointer representation.

nanoprintf does include C standard headers but only uses them for C99 types and argument lists; no calls are made into stdlib / libc, with the exception of any internal double-to-float conversion ABI calls your compiler might emit. As usual, some Windows-specific headers are required if you're compiling natively for msvc.

nanoprintf is statically configurable so users can find a balance between size, compiler requirements, and feature set. Floating point conversion, "large" length modifiers, and size write-back are all configurable and are only compiled if explicitly requested, see [Configuration](https://github.com/charlesnicholson/nanoprintf#configuration) for details.

## Motivation

I wanted a single-file public-domain drop-in printf that came in at under 1000 bytes in the minimal configuration (bootloaders etc), and under 3000 bytes with the floating-point bells and whistles enabled.

## Philosophy

This code is optimized for size, not readability or structure. Unfortunately modularity and "cleanliness" even in C adds overhead at this small scale, so most of the functionality and logic is pushed together into `npf_vpprintf`. This is not what normal embedded systems code should look like; it's `#ifdef` soup and hard to make sense of, and I apologize if you have to spelunk around in the implementation. Hopefully the various tests will serve as guide rails if you hack around in it.

Alternately, perhaps you're a significantly better programmer than I! In that case, please help me make this code smaller and cleaner without making the footprint larger, or nudge me in the right direction. :)

## Usage

Add the following code to one of your source files to compile the nanoprintf implementation:
```
// define your nanoprintf configuration macros here (see "Configuration" below)
#define NANOPRINTF_IMPLEMENTATION
#include "path/to/nanoprintf.h"
```

Then, in any file where you want to use nanoprintf, simply include the header and call the npf_ functions.

See the "[Use nanoprintf directly](https://github.com/charlesnicholson/nanoprintf/blob/master/examples/use_npf_directly/main.cpp)" and "[Wrap nanoprintf](https://github.com/charlesnicholson/nanoprintf/blob/master/examples/wrap_npf/main.cpp)" examples for more details.

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

nanoprintf has the following static configuration flags.

* `NANOPRINTF_USE_FIELD_WIDTH_FORMAT_SPECIFIERS`: Set to `0` or `1`. Enables field width specifiers.
* `NANOPRINTF_USE_PRECISION_FORMAT_SPECIFIERS`: Set to `0` or `1`. Enables precision specifiers.
* `NANOPRINTF_USE_FLOAT_FORMAT_SPECIFIERS`: Set to `0` or `1`. Enables floating-point specifiers.
* `NANOPRINTF_USE_LARGE_FORMAT_SPECIFIERS`: Set to `0` or `1`. Enables oversized modifiers.
* `NANOPRINTF_USE_WRITEBACK_FORMAT_SPECIFIERS`: Set to `0` or `1`. Enables `%n` for write-back.
* `NANOPRINTF_VISIBILITY_STATIC`: Optional define. Marks prototypes as `static` to sandbox nanoprintf.

If no configuration flags are specified, nanoprintf will default to "reasonable" embedded values in an attempt to be helpful: floats enabled, writeback and large formatters disabled. If any configuration flags are explicitly specified, nanoprintf requires that all flags are explicitly specified.

If a disabled format specifier feature is used, no conversion will occur and the format specifier string simply will be printed instead.

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
	* `%%`: Percent-sign literal
	* `%c`: Characters
	* `%s`: Null-terminated strings
	* `%i`/`%d`: Signed integers
	* `%u`: Unsigned integers
	* `%o`: Unsigned octal integers
	* `%x` / `%X`: Unsigned hexadecimal integers
	* `%p`: Pointers
	* `%n`: Write the number of bytes written to the pointer vararg
	* `%f`/`%F`: Floating-point values

## Floating Point

Floating point conversion is performed by extracting the value into 64:64 fixed-point with an extra field that specifies the number of leading zero fractional digits before the first nonzero digit. No rounding is currently performed; values are simply truncated at the specified precision. This is done for simplicity, speed, and code footprint.

Despite `nano` in the name, there's no way to do away with double entirely, since the C language standard says that floats are promoted to double any time they're passed into variadic argument lists. nanoprintf casts all doubles back down to floats before doing any conversions.

## Measurement

The CI build is set up to use gcc and nm to measure the compiled size of every pull request. See the [Presubmit Checks](https://github.com/charlesnicholson/nanoprintf/actions/workflows/presubmit.yml) "size reports" job output for recent runs.

Minimal configuration (all features disabled):
```
arm-none-eabi-gcc -mcpu=cortex-m0 -mthumb -Os -x c -c -o cm0-min.o -DNANOPRINTF_IMPLEMENTATION -DNANOPRINTF_USE_FIELD_WIDTH_FORMAT_SPECIFIERS=0 -DNANOPRINTF_USE_PRECISION_FORMAT_SPECIFIERS=0 -DNANOPRINTF_USE_FLOAT_FORMAT_SPECIFIERS=0 -DNANOPRINTF_USE_LARGE_FORMAT_SPECIFIERS=0 -DNANOPRINTF_USE_WRITEBACK_FORMAT_SPECIFIERS=0 - <<< '#include "nanoprintf.h"'
arm-none-eabi-nm --print-size --size-sort cm0-min.o | python tests/size_report.py

00000016 00000002 t npf_bufputc_nop
00000000 00000016 t npf_bufputc
0000036e 00000016 T npf_pprintf
000003b8 00000016 T npf_snprintf
00000384 00000034 T npf_vsnprintf
00000018 00000356 T npf_vpprintf

Total size: 0x3ce (974 bytes)
```

Medium configuration (field width and precision specifiers):
```
arm-none-eabi-gcc -mcpu=cortex-m0 -mthumb -Os -x c -c -o cm0-med.o -DNANOPRINTF_IMPLEMENTATION -DNANOPRINTF_USE_FIELD_WIDTH_FORMAT_SPECIFIERS=1 -DNANOPRINTF_USE_PRECISION_FORMAT_SPECIFIERS=1 -DNANOPRINTF_USE_FLOAT_FORMAT_SPECIFIERS=0 -DNANOPRINTF_USE_LARGE_FORMAT_SPECIFIERS=0 -DNANOPRINTF_USE_WRITEBACK_FORMAT_SPECIFIERS=0 - <<< '#include "nanoprintf.h"'
arm-none-eabi-nm --print-size --size-sort cm0-med.o | python tests/size_report.py

00000016 00000002 t npf_bufputc_nop
00000000 00000016 t npf_bufputc
0000065e 00000016 T npf_pprintf
000006a8 00000016 T npf_snprintf
00000674 00000034 T npf_vsnprintf
00000018 00000646 T npf_vpprintf

Total size: 0x6be (1726) bytes
```

Maximal configuration (field width, precision, large int, writeback, floating point):
```
arm-none-eabi-gcc -mcpu=cortex-m0 -mthumb -Os -x c -c -o cm0-max.o -DNANOPRINTF_IMPLEMENTATION -DNANOPRINTF_USE_FIELD_WIDTH_FORMAT_SPECIFIERS=1 -DNANOPRINTF_USE_PRECISION_FORMAT_SPECIFIERS=1 -DNANOPRINTF_USE_FLOAT_FORMAT_SPECIFIERS=1 -DNANOPRINTF_USE_LARGE_FORMAT_SPECIFIERS=1 -DNANOPRINTF_USE_WRITEBACK_FORMAT_SPECIFIERS=1 - <<< '#include "nanoprintf.h"'
arm-none-eabi-nm --print-size --size-sort cm0-max.o | python tests/size_report.py

00000016 00000002 t npf_bufputc_nop
00000000 00000016 t npf_bufputc
00000a2c 00000016 T npf_pprintf
00000a74 00000016 T npf_snprintf
00000a42 00000032 T npf_vsnprintf
00000018 00000a14 T npf_vpprintf

Total size: 0xa8a (2698) bytes
```

## Development

To get the environment and run tests (linux / mac only for now):

1. Clone or fork this repository.
1. Run `./b` from the root.

This will build all of the unit, conformance, and compilation tests for your host environment. Any test failures will return a non-zero exit code.

The nanoprintf development environment uses [cmake](https://cmake.org/) and [ninja](https://ninja-build.org/). If you have these in your path, `./b` will use them. If not, `./b` will download and deploy them into `path/to/your/nanoprintf/external`.

nanoprintf uses GitHub Actions for all continuous integration builds. The GitHub Linux builds use [this](https://github.com/charlesnicholson/docker-images/packages/751874) Docker image from [my Docker repository](https://github.com/charlesnicholson/docker-images).

The matrix builds [Debug, Release] x [32-bit, 64-bit] x [Mac, Windows, Linux] x [gcc, clang, msvc], minus the 32-bit clang Mac configurations.

One test suite is a fork from the [printf test suite](), which is MIT licensed. It exists as a submodule for licensing purposes- nanoprintf is public domain, so this particular test suite is optional and excluded by default. To build it, retrieve it by updating submodules and add the `--paland` flag to your `./b` invocation. It is not required to use nanoprintf at all.

## Limitations

No wide-character support exists: the `%lc` and `%ls` fields require that the arg be converted to a char array as if by a call to [wcrtomb](http://man7.org/linux/man-pages/man3/wcrtomb.3.html). When locale and character set conversions get involved, it's hard to keep the name "nano". Accordingly, `%lc` and `%ls` behave like `%c` and `%s`, respectively.

Currently the only supported float conversions are the decimal forms: `%f` and `%F`. Pull requests welcome!

## Acknowledgments

I implemented Float-to-int conversion using the ideas from [Wojciech Muła](mailto:zdjęcia@garnek.pl)'s [float -> 64:64 fixed algorithm](http://0x80.pl/notesen/2015-12-29-float-to-string.html).

I ported the [printf test suite](https://github.com/eyalroz/printf/blob/master/test/test_suite.cpp) to nanoprintf. It was originally from the [mpaland printf project](https://github.com/mpaland/printf) codebase but adopted and improved by [Eyal Rozenberg](https://github.com/eyalroz) and others. (Nanoprintf has many of its own tests, but these are also very thorough and very good!)
