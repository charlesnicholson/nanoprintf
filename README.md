# nanoprintf

[![CircleCI](https://circleci.com/gh/charlesnicholson/nanoprintf.svg?style=shield)](https://circleci.com/gh/charlesnicholson/nanoprintf) [![](https://img.shields.io/badge/license-public_domain-brightgreen.svg)](https://github.com/charlesnicholson/nanoprintf/blob/master/LICENSE)

nanoprintf is an implementation of snprintf and vsnprintf for embedded systems that aims for C11 standard compliance. 

nanoprintf makes no memory allocations, uses less than 100 bytes of stack, and is around 5KB of ARM Cortex-M object code when optimized with all the bells and whistles turned on (slightly larger on x64).

nanoprintf is a [single header file](https://github.com/charlesnicholson/nanoprintf/blob/readme/nanoprintf.h) in the style of the [stb libraries](https://github.com/nothings/stb). The rest of the repository is tests and scaffolding and not required for use.

nanoprintf is written in C89 for maximal compiler compatibility. C99 or C++11 compilers are required (for `uint64_t` and other types) if floating point conversion or large modifiers are enabled. nanoprintf does include standard headers but only uses them for types and argument lists; no calls are made into stdlib / libc, with the possible exception of double-to-float conversion.

nanoprintf is statically configurable so users can find a balance between size, compiler requirements, and feature set. Floating point conversion, "large" length modifiers, and size write-back are all configurable and are only compiled if explicitly requested, see [Configuration](https://github.com/charlesnicholson/nanoprintf/tree/readme#configuration) for details.

## Usage

1. Copy `nanoprintf.h` into your codebase somewhere.
1. Add the following code to one of your `.c` or `.cpp` files to compile the nanoprintf implementation:

	```
	#define NANOPRINTF_IMPLEMENTATION
	#include "path/to/nanoprintf.h"
	```

1. `#include "path/to/nanoprintf.h"` as usual to expose the function prototypes.

## API

nanoprintf has 4 main functions:
* `npf_snprintf`: Use like [snprintf](https://en.cppreference.com/w/c/io/fprintf).
* `npf_vsnprintf`: Use like [vsnprintf](https://en.cppreference.com/w/c/io/vfprintf) (`va_list` support).
* `npf_pprintf`: Use like [printf](https://en.cppreference.com/w/c/io/fprintf) with a per-character write callback (semihosting, UART, etc).
* `npf_vpprintf`: Use like `npf_pprintf` but takes a `va_list`.

The `pprintf` variations take a callback that returns an `int`. If the callback returns `NPF_EOF`, the print functions will stop printing and return immediately.

nanoprintf does *not* provide `printf` itself; that's seen as project- or platform-specific. nanoprintf is hopefully a good building block for rolling your own `printf`, though.

## Configuration

nanoprintf has the following static configuration flags. You can either inject them into your compiler (usually `-D` flags) or wrap `nanoprintf.h` in [your own header](https://github.com/charlesnicholson/nanoprintf/blob/readme/unit_tests/nanoprintf_in_unit_tests.h) that sets them up, and then `#include` your header instead of `nanoprintf.h` in your application.

* `NANOPRINTF_USE_FLOAT_FORMAT_SPECIFIERS`: Set to `0` or `1`. Enables floating-point specifiers.
* `NANOPRINTF_USE_LARGE_FORMAT_SPECIFIERS`: Set to `0` or `1`. Enables oversized modifiers.
* `NANOPRINTF_USE_WRITEBACK_FORMAT_SPECIFIERS`: Set to `0` or `1`. Enables `%n` for write-back.
* `NANOPRINTF_VISIBILITY_STATIC`: Optional define. Marks prototypes as `static` to sandbox nanoprintf.

If a disabled format specifier feature is used, no conversion will occur and the format specifier string simply will be printed instead.

## Features

Like `printf`, `nanoprintf` expects a conversion specification string of the following form:

`[flags][field width][.precision][length modifier][conversion specifier]`

* **Flags**

	None or more of the following:
	* `0`: Pad the field with leading zero characters.
	* `-`: Left-justify the conversion result in the field.
	* `+`: Signed conversions always begin with `+` or `-` characters.
	* ` `: (space) A space character is inserted if the first converted character is not a sign.
	* `#`: Writes extra characters (`0x` for hex, `.` for empty floats, '0' for empty octals, etc).
* **Field width**

	A number that specifies the total field width for the conversion, adds padding. If field width is `*`, the field width is read from the next vararg.
* **Precision**

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

## Acknowledgments

Float-to-int conversion is done using [Wojciech Muła](mailto:zdjęcia@garnek.pl)'s [algorithm](http://0x80.pl/notesen/2015-12-29-float-to-string.html).
