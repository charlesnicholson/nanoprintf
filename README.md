# nanoprintf

[![Presubmit Checks](https://github.com/charlesnicholson/nanoprintf/workflows/Presubmit%20Checks/badge.svg)](https://github.com/charlesnicholson/nanoprintf/tree/master/.github/workflows) [![](https://img.shields.io/badge/pylint-10.0-brightgreen.svg)](https://www.pylint.org/) [![](https://img.shields.io/badge/license-public_domain-brightgreen.svg)](https://github.com/charlesnicholson/nanoprintf/blob/master/LICENSE)

nanoprintf is an implementation of snprintf and vsnprintf for embedded systems that, when fully enabled, aims for C11 standard compliance.

nanoprintf makes no memory allocations and uses less than 100 bytes of stack. nanoprintf compiles to somewhere between 1-3KB of code on a Cortex-M architecture.

nanoprintf is a [single header file](https://github.com/charlesnicholson/nanoprintf/blob/master/nanoprintf.h) in the style of the [stb libraries](https://github.com/nothings/stb). The rest of the repository is tests and scaffolding and not required for use.

nanoprintf is written in a minimal dialect of C99 for maximal compiler compatibility, and compiles cleanly at the highest warning levels on clang, gcc, and msvc in both 32- and 64-bit modes. It's _really_ hard to write portable C89 code, btw, when you don't have any guarantee about what integral type to use to hold a converted pointer representation.

nanoprintf does include C standard headers but only uses them for C99 types and argument lists; no calls are made into stdlib / libc, with the exception of any internal double-to-float conversion ABI calls your compiler might emit. As usual, some Windows-specific headers are required if you're compiling natively for msvc.

nanoprintf is statically configurable so users can find a balance between size, compiler requirements, and feature set. Floating point conversion, "large" length modifiers, and size write-back are all configurable and are only compiled if explicitly requested, see [Configuration](https://github.com/charlesnicholson/nanoprintf#configuration) for details.

## Motivation

[tinyprintf](https://github.com/cjlano/tinyprintf) doesn't print floating point values.

"[printf](https://github.com/mpaland/printf)" defines the actual standard library `printf` symbol, which isn't always what you want. It stores the final converted string (with padding and precision) in a temporary buffer, which makes supporting longer strings more costly. It also doesn't support the `%n` "write-back" specifier.

Also, no embedded-friendly printf projects that I could find are both in the public domain *and* have single-file implementations.

## Philosophy

This code is optimized for size, not readability or structure. Unfortunately modularity and "cleanliness" even in C adds overhead at this small scale, so most of the functionality and logic is pushed together into `npf_vpprintf`. This is not what normal embedded systems code should look like; it's `#ifdef` soup and hard to make sense of, and I apologize if you have to spelunk around in the implementation. Hopefully the various tests will serve as guide rails if you hack around in it.

Alternately, perhaps you're a significantly better programmer than I! In that case, please help me make this code smaller and cleaner without making the footprint larger, or nudge me in the right direction. :)

## Usage

Integrate nanoprintf into your codebase in one of two ways:
1. Create a header file that sets up the flags and includes `nanoprintf.h`. Call the nanoprintf API directly wherever you want to use it. Add a c/c++ file that compiles the nanoprintf implementation.
1. Create your own header file that wraps the parts of the nanoprintf API that you want to expose. Sandbox all of nanoprintf inside a single c/c++ file that forwards your function to nanoprintf.

Add the following code to one of your `.c` or `.cpp` files to compile the nanoprintf implementation:

```
#define NANOPRINTF_IMPLEMENTATION
#include "path/to/nanoprintf.h"
```
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

nanoprintf has the following static configuration flags. You can either inject them into your compiler (usually `-D` flags) or wrap `nanoprintf.h` in [your own header](https://github.com/charlesnicholson/nanoprintf/blob/master/unit_tests/nanoprintf_in_unit_tests.h) that sets them up, and then `#include` your header instead of `nanoprintf.h` in your application.

If no configuration flags are specified, nanoprintf will default to "reasonable" embedded values in an attempt to be helpful: floats enabled, writeback and large formatters disabled. If any configuration flags are explicitly specified, nanoprintf requires that all flags are explicitly specified.

* `NANOPRINTF_USE_FIELD_WIDTH_FORMAT_SPECIFIERS`: Set to `0` or `1`. Enables field width specifiers.
* `NANOPRINTF_USE_PRECISION_FORMAT_SPECIFIERS`: Set to `0` or `1`. Enables precision specifiers.
* `NANOPRINTF_USE_FLOAT_FORMAT_SPECIFIERS`: Set to `0` or `1`. Enables floating-point specifiers.
* `NANOPRINTF_USE_LARGE_FORMAT_SPECIFIERS`: Set to `0` or `1`. Enables oversized modifiers.
* `NANOPRINTF_USE_WRITEBACK_FORMAT_SPECIFIERS`: Set to `0` or `1`. Enables `%n` for write-back.
* `NANOPRINTF_VISIBILITY_STATIC`: Optional define. Marks prototypes as `static` to sandbox nanoprintf.

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

Compiling with all optional features disabled yields ~1KB of ARM Cortex-M0 object code:
```
Minimal configuration:
 .text.npf__bufputc_nop         0x2
 .text.npf__bufputc             0x16
 .text.npf_pprintf              0x2c
 .text.npf_snprintf             0x2c
 .text.npf__itoa_rev            0x42
 .text.npf_vsnprintf            0x48
 .text.npf__utoa_rev            0x4a
 .text.npf__parse_format_spec   0xca
 .text.npf_vpprintf             0x210
total:                          0x41e (1054 bytes)
```

Compiling with field width and precision specifiers enabled yields ~1.7KB:
```
"Small" configuration: (field witdh + precision)
 .text.npf__bufputc_nop         0x2
 .text.npf__bufputc             0x16
 .text.npf_pprintf              0x2c
 .text.npf_snprintf             0x2c
 .text.npf__itoa_rev            0x42
 .text.npf_vsnprintf            0x48
 .text.npf__utoa_rev            0x4a
 .text.npf__parse_format_spec   0x1a0
 .text.npf_vpprintf             0x3c4
total:                          0x6a8 (1704 bytes)
```

Compiling with all optional features enabled is closer to ~2.8KB:
```
Everything:
 .text.npf__bufputc_nop         0x2
 .text.npf__bufputc             0x16
 .text.npf_snprintf             0x2c
 .text.npf_pprintf              0x2c
 .text.npf_vsnprintf            0x48
 .text.npf__itoa_rev            0x5a
 .text.npf__utoa_rev            0x6c
 .text.npf__fsplit_abs          0x100
 .text.npf__ftoa_rev            0x130
 .text.npf__parse_format_spec   0x204
 .text.npf_vpprintf             0x528
total:                          0xada (2778 bytes)
```

## Development

To get the environment and run tests (linux / mac only for now):

1. Clone or fork this repository.
1. Run `./b` from the root.

This will build all of the unit, conformance, and compilation tests for your host environment. Any test failures will return a non-zero exit code.

The nanoprintf development environment uses [cmake](https://cmake.org/) and [ninja](https://ninja-build.org/). If you have these in your path, `./b` will use them. If not, `./b` will download and deploy them into `path/to/your/nanoprintf/external`.

nanoprintf uses GitHub Actions for all continuous integration builds. The GitHub Linux builds use [this](https://hub.docker.com/r/charlesnicholson/circleci-images) Docker image on [Docker Hub](https://hub.docker.com/). The Dockerfile lives [here](https://github.com/charlesnicholson/circleci-images).

The matrix builds [Debug, Release] x [32-bit, 64-bit] x [Mac, Windows, Linux] x [gcc, clang, msvc], minus the 32-bit clang Mac configurations.

## Limitations

No wide-character support exists: the `%lc` and `%ls` fields require that the arg be converted to a char array as if by a call to [wcrtomb](http://man7.org/linux/man-pages/man3/wcrtomb.3.html). When locale and character set conversions get involved, it's hard to keep the name "nano". Accordingly, `%lc` and `%ls` behave like `%c` and `%s`, respectively.

Currently the only supported float conversions are the decimal forms: `%f` and `%F`. Pull requests welcome!

## Acknowledgments

Float-to-int conversion is done using [Wojciech Muła](mailto:zdjęcia@garnek.pl)'s float -> 64:64 fixed [algorithm](http://0x80.pl/notesen/2015-12-29-float-to-string.html).
