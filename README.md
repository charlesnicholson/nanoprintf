# nanoprintf

[![CircleCI](https://circleci.com/gh/charlesnicholson/nanoprintf.svg?style=shield)](https://circleci.com/gh/charlesnicholson/nanoprintf) [![](https://img.shields.io/badge/license-public_domain-brightgreen.svg)](https://github.com/charlesnicholson/nanoprintf/blob/master/LICENSE)

nanoprintf is an almost-standard-compliant implementation of snprintf and vsnprintf for embedded systems. Zero memory allocations, less than 100 bytes of stack, and around 5KB of ARM Cortex-M object code when optimized with all the bells and whistles turned on.

nanoprintf is a [single header file](https://github.com/charlesnicholson/nanoprintf/blob/readme/nanoprintf.h) in the style of the [stb libraries](https://github.com/nothings/stb). The rest of the repository is tests and scaffolding and not required for use.

## Usage

1. Copy `nanoprintf.h` into your codebase somewhere.
1. Add the following code to one of your `.c` or `.cpp` files to compile the nanoprintf implementation:

	```
	#define NANOPRINTF_IMPLEMENTATION
	#include "path/to/nanoprintf.h"
	```

1. `#include "path/to/nanoprintf.h"` as usual to expose the function prototypes.

## Configuration

nanoprintf has the following static configuration flags. You can either inject them into your compiler (usually `-D` flags) or wrap `nanoprintf.h` in [your own header](https://github.com/charlesnicholson/nanoprintf/blob/readme/unit_tests/nanoprintf_in_unit_tests.h) that sets them up, and then `#include` your header instead of `nanoprintf.h` in your application.

* `NANOPRINTF_USE_FLOAT_FORMAT_SPECIFIERS`: Set to `0` or `1`. Enables floating-point conversion operators.
* `NANOPRINTF_USE_C99_FORMAT_SPECIFIERS`: Set to `0` or `1`. Enables C99-specific conversion operators.
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
	* 'l': Use `long`, `double`, or wide vararg width.
	* `hh`: (C99-only specifier) Use `char` for integral and write-back vararg width.
	* `ll`: (C99-only specifier) Use `long long` for integral and write-back vararg width.
	* `j`: (C99-only specifier) Use the `[u]intmax_t` types for integral and write-back vararg width.
	* `z`: (C99-only specifier) Use the `size_t` types for integral and write-back vararg width.
	* `t`: (C99-only specifier) Use the `ptrdiff_t` types for integral and write-back vararg width.
* **Conversion specifier**
	
	Exactly one of the following:
	* `%c`: Characters
	* `%s`: Null-terminated strings
	* `%i`/`%d`: Signed integers
	* `%u`: Unsigned integers
	* `%o`: Unsigned octal integers
	* `%x` / `%X`: Unsigned hexadecimal integers
	* `%p`: Pointers
	* `%n`: Write the number of bytes written to the pointer vararg
	* `%f`/`%F`: Floating-point values
