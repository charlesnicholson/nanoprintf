# nanoprintf

[![CircleCI](https://circleci.com/gh/charlesnicholson/nanoprintf.svg?style=shield)](https://circleci.com/gh/charlesnicholson/nanoprintf) [![](https://img.shields.io/badge/license-public_domain-brightgreen.svg)](https://github.com/charlesnicholson/nanoprintf/blob/master/LICENSE)

nanoprintf is an almost-standard-compliant implementation of snprintf and vsnprintf for embedded systems. Zero memory allocations, less than 100 bytes of stack, less than 5kb object code (cortex-m, -Os) with the bells and whistles turned on.

nanoprintf is a [single header file](https://github.com/charlesnicholson/nanoprintf/blob/readme/nanoprintf.h) in the style of the [stb libraries](https://github.com/nothings/stb). The rest of the repository is tests and scaffolding and not required for use.

## Features

## Usage

1. Copy `nanoprintf.h` into your codebase somewhere.
1. Add the following code to one of your `.c` or `.cpp` files to compile the nanoprintf implementation:

	```
	#define NANOPRINTF_IMPLEMENTATION
	#include "path/to/nanoprintf.h"
	```

1. `#include "path/to/nanoprintf.h"` as usual to expose the function prototypes.

## Configuration

nanoprintf has the following static configuration flags. You can either inject them into your compiler (usually `-D` flags) or wrap `nanoprintf.h` in [your own header](https://github.com/charlesnicholson/nanoprintf/blob/readme/unit_tests/nanoprintf_in_unit_tests.h) that sets them up.

* `NANOPRINTF_USE_FLOAT_FORMAT_SPECIFIERS`: Set to `0` or `1`. Enables floating-point conversion operators.
* `NANOPRINTF_USE_C99_FORMAT_SPECIFIERS`: Set to `0` or `1`. Enables C99-specific conversion operators.
* `NANOPRINTF_VISIBILITY_STATIC`: Optional define with no value. Marks all prototypes as `static` to sandbox nanoprintf.