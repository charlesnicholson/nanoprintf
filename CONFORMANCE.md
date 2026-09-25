# nanoprintf and the C Standard

This document lists where nanoprintf's output differs from what the C Standard requires of `printf`. It also records what nanoprintf does where the Standard leaves the behavior implementation-defined or undefined.

The reference texts are the C11 draft N1570, section 7.21.6.1 (`fprintf`), and the C23 draft N3220, section 7.23.6.1. The two sections use the same paragraph numbers through paragraph 13. C23 adds a paragraph 14 about `%B`, so "Returns" is paragraph 14 in C11 and 15 in C23, and "Environmental limits" is 15 in C11 and 16 in C23. References here use C23 numbering and are written as, for example, p6 for 7.23.6.1 paragraph 6. `snprintf` is 7.23.6.5.

Unless a section says otherwise, the examples assume:

* every feature flag is enabled,
* `NANOPRINTF_CONVERSION_BUFFER_SIZE` is 64, the default,
* `NANOPRINTF_CONVERSION_FLOAT_TYPE` is 32 bits wide, the default,
* the target has 64-bit pointers and a 32-bit little-endian `wchar_t`.

Where an example gives a correctly rounded result, it was computed from the exact binary value of the argument, not taken from another `printf`.

Each entry is one of four kinds:

* **Deviation**: for a valid call, nanoprintf's output or return value differs from what the Standard requires.
* **Implementation-defined**: the Standard lets the implementation choose. The entry records nanoprintf's choice.
* **Undefined**: the Standard places no requirement on the call. The entry records what nanoprintf does today. Code should not depend on it.
* **Configuration**: the behavior depends on a compile-time option.

Behavior not listed here is meant to follow the Standard. Report any difference as a bug.

## Contents

1. [Configuration](#1-configuration)
2. [Deviations](#2-deviations)
3. [Implementation-defined choices](#3-implementation-defined-choices)
4. [Undefined behavior](#4-undefined-behavior)
5. [Items from issue #299](#5-items-from-issue-299)

## 1. Configuration

A conforming `printf` accepts every flag, length modifier, and conversion specifier the Standard defines. nanoprintf compiles each group in or out.

| Option | What it adds | Default |
|---|---|---|
| none, always present | `%%`, `c`, `s`, `d`, `i`, `u`, `o`, `x`, `X`, `p`, the `+` and space flags, the `l` modifier | on |
| `NANOPRINTF_USE_FIELD_WIDTH_FORMAT_SPECIFIERS` | field width, literal and `*`, and the `-` and `0` flags | 1 |
| `NANOPRINTF_USE_PRECISION_FORMAT_SPECIFIERS` | precision, literal and `.*` | 1 |
| `NANOPRINTF_USE_ALT_FORM_FLAG` | the `#` flag | 1 |
| `NANOPRINTF_USE_SMALL_FORMAT_SPECIFIERS` | `hh`, `h` | 1 |
| `NANOPRINTF_USE_LARGE_FORMAT_SPECIFIERS` | `ll`, `j`, `z`, `t` | 0 |
| `NANOPRINTF_USE_FIXED_WIDTH_FORMAT_SPECIFIERS` | `wN`, `wfN` (C23) | 0 |
| `NANOPRINTF_USE_FLOAT_FORMAT_SPECIFIERS` | `f`, `F`, and the `L` modifier | 1 |
| `NANOPRINTF_USE_FLOAT_SCI_FORMAT_SPECIFIER` | `e`, `E` | 0 |
| `NANOPRINTF_USE_FLOAT_SHORTEST_FORMAT_SPECIFIER` | `g`, `G` | 0 |
| `NANOPRINTF_USE_FLOAT_HEX_FORMAT_SPECIFIER` | `a`, `A` | 0 |
| `NANOPRINTF_USE_BINARY_FORMAT_SPECIFIERS` | `b`, `B` (C23) | 0 |
| `NANOPRINTF_USE_WRITEBACK_FORMAT_SPECIFIERS` | `n` | 0 |

The Default column is the configuration nanoprintf picks when none of its options are defined. In that configuration `%lld`, `%zu`, `%jd`, `%td`, `%e`, `%g`, `%a`, `%b`, and `%n` do not convert. A build has every C11 feature only when every row except the two C23 rows is enabled. It has every C23 feature only when every row is enabled. See the README's [Configuration](README.md#configuration) section for the rules on defining the options.

When a conversion specification uses a feature that is compiled out:

* The specification does not parse. nanoprintf writes the `%`, then continues at the next character, so the text of the specification appears in the output unchanged.
* The argument is not consumed. Every later conversion in the same call reads the wrong argument. In a build without `%e`, `npf_snprintf(buf, n, "%e %d", 1.5, 7)` prints `%e` followed by whatever `%d` reads from the slot that holds `1.5`.
* With precision compiled out, any precision stops the specification from parsing, `%.2f` and `%.3s` included. Floating-point conversions without a precision still work at the default precision: 6 for `f`, `e`, and `g`, and the full mantissa for `a`.

No option parses a disabled specification and discards its argument. Discarding the argument needs its type, and the type-handling code is what the option removes.

## 2. Deviations

### 2.1 Floating-point output is limited by the conversion buffer

**Standard**: p8 requires every finite value to be converted at the requested precision. p16 requires any single conversion to be able to produce at least 4095 characters.

**nanoprintf**: `%f`, `%e`, `%g`, and `%a` build their result in a stack buffer of `NANOPRINTF_CONVERSION_BUFFER_SIZE` bytes. When the result does not fit, the conversion prints `err`, or `ERR` for `F`, `E`, `G`, and `A`. The sign and the `+` and space flags still apply, and a field width pads with spaces. The `0` flag is ignored, the same as for `inf`. `%f` of `-1e300` prints `-err`.

The buffer holds the digits, the decimal point, and for `%e` and `%a` the exponent. It does not hold the sign, a `0x` prefix, or padding. In the table, "size" is `NANOPRINTF_CONVERSION_BUFFER_SIZE` and "point" is 1 when a decimal point is printed, which happens when the precision is nonzero or `#` is given.

| Conversion | Prints `err` when | Limit at size 64 |
|---|---|---|
| `%e` | precision > size − 8 | precision 56 |
| `%g` | P > size − 7, however short the result would be | P = 57. `%.58g` of `1.0` prints `err` |
| `%f`, with `%e` or `%g` enabled | precision > size − 2, or integer digits + point + precision > size | `%.62f` of `0.5` fits, `%.63f` does not |
| `%f`, with neither enabled | integer digits + point + precision > size. With an intermediate type of 32 bits or fewer, also point + precision > size − 10, and the integer check can fail one digit early | `%.53f` of `0.5` fits, `%.54f` does not. `%.53f` of `5e9` prints `err` although its 64 characters would fit |
| `%a` | precision > size − 8 | precision 56. `%.57a` of `1.0` prints `err` |

"Integer digits" means the digits nanoprintf generates, which can be wrong (see [2.2](#22-decimal-conversions-are-not-correctly-rounded)). With the 32-bit intermediate, `%f` of `1e57` generates 57 integer digits instead of 58.

With the defaults, `%f` prints `err` for values with 58 or more integer digits, which is roughly 1e57 and up. `DBL_MAX` has 309 integer digits. A larger buffer raises every limit in the table. The buffer is on the stack.

### 2.2 Decimal conversions are not correctly rounded

**Standard**: p8 says the value "is rounded to the appropriate number of digits". p13, a recommended practice, says the result should be correctly rounded when it has at most `DECIMAL_DIG` significant digits (17 for IEEE 754 double). Past that many digits it should lie between the two adjacent `DECIMAL_DIG`-digit decimal strings that bound the value. 3.12 defines "correctly rounded" as the nearest representable result under the current rounding mode.

**nanoprintf**: `%f`, `%e`, and `%g` scale the binary mantissa to decimal inside an unsigned integer of type `NANOPRINTF_CONVERSION_FLOAT_TYPE`, which is `unsigned int` by default. Each scaling step can drop low bits, so the digits are only as accurate as that integer allows. The README's [Accuracy](README.md#accuracy) section has a table of worst-case correct digits.

With the default 32-bit intermediate:

| Call | nanoprintf | Correctly rounded |
|---|---|---|
| `%.1f` of `0.05` | `0.0` | `0.1`. The double is 0.05000000000000000277… |
| `%.2f` of `1.005` | `1.01` | `1.00`. The double is 1.00499999999999989… |
| `%.17f` of `0.1` | `0.10000000000000000` | `0.10000000000000001` |
| `%e` of `1e300` | `9.999999e+299` | `1.000000e+300` |
| `%e` of `5e-324` | `0.000000e+00` | `4.940656e-324` |
| `%f` of `1e40` | `9999999890000000000000000000000000000000.000000` | `10000000000000000303786028427003666890752.000000` |

The `5e-324` row also breaks the p8 rule that `%e` puts a nonzero digit before the point for a nonzero value. With the 32-bit intermediate, subnormal values print as zero.

With `NANOPRINTF_CONVERSION_FLOAT_TYPE` set to `uint64_t`, the first five rows come out correctly rounded. The `1e40` row prints `10000000000000000290000000000000000000000.000000`. Digits past what the intermediate can hold come out as zeros. That result has more than 17 significant digits and is inside the p13 bounds. Other results are still wrong:

* `%.25f` of `1.2899085044826509e-09` prints `0.0000000012899085044826508`. The double is 1.28990850448265085026…e-9, so the correctly rounded result ends in `509`. The result has 17 significant digits, so p13 asks for correct rounding.
* In samples of 20000 random doubles each, `%.16e` was wrong in the last digit for 9 values within five decades of 1, for 3356 within five decades of 1e-300, and for 6550 within five decades of 1e300.
* In samples of 200000 random doubles each, `%f` results with at most 17 significant digits were all correctly rounded between 1e-5 and 1e10, and 67 were wrong between 1e-20 and 1e-5.

Issue #299 asks whether `%f` rounds exactly once the intermediate has N bits, the double has M mantissa bits, and N − log2(5) ≥ M. It does not. `uint64_t` gives N − log2(5) ≈ 61.7 against M = 53, and the `%.25f` result above is still wrong. The README states the narrower condition that holds: integer parts below 2^N, and fractions with at most N − log2(5) bits after the binary point, convert exactly. A double with binary exponent e has 52 − e fraction bits. `1.2899085044826509e-09` has e = −30, so its fraction has 82 bits, more than the 61 a `uint64_t` intermediate keeps. With `uint8_t`, `%f` of `3.14159` prints `3.143750` and `%f` of `1234.5` prints `1220.000000`.

**Rounding direction**: nanoprintf always rounds to nearest with ties to even and ignores `fesetround`. Under `FE_UPWARD`, `FE_DOWNWARD`, or `FE_TOWARDZERO`, the result is not the one 3.12 describes. A narrow intermediate can also see a tie that is not there. That is the cause of the `0.05` row above.

### 2.3 `%lc` and `%ls` are not supported

**Standard**: p7 and p8 say `%lc` converts a `wint_t` to a multibyte character as if by `wcrtomb`, and `%ls` converts a `wchar_t` string the same way.

**nanoprintf**: has no `wcrtomb`, so `%lc` and `%ls` do not parse. They print verbatim and do not consume their argument, the same as any other unsupported conversion (see [1](#1-configuration)). `%lC` and `%lS` behave the same way. Because nothing is converted, no encoding error can occur, which is why nanoprintf never returns a negative value.

### 2.4 `long double` is converted to `double`

**Standard**: p7 says `L` makes `a`, `A`, `e`, `E`, `f`, `F`, `g`, and `G` apply to a `long double` argument.

**nanoprintf**: reads the `long double` and converts it to `double` before formatting, or to `float` in single-precision mode. Nothing is lost where `long double` has the same format as `double`. Where it is wider, such as the x87 80-bit format or the 128-bit format on AArch64 Linux, the output has only `double` precision, and values outside the range of `double` print as `inf` or `0`.

### 2.5 Single-precision mode

`NANOPRINTF_USE_FLOAT_SINGLE_PRECISION=1` departs from the Standard on purpose. The `npf_snprintf` and `npf_pprintf` macros convert every `float` and `double` argument to `float` at the call site. The consequences:

* `double` values outside the range of `float` print as `inf` or `0`. `%f` of `1e300` prints `inf`, and `%f` of `1e-50` prints `0.000000`.
* The digits are those of the `float` value. `%.10f` of `0.1` prints `0.1000000015`.
* Under GCC, Clang, and C++, the macros do not wrap a `long double` argument, but the implementation still reads a wrapped `float`, so `%Lf` is undefined behavior. With Clang on arm64, `npf_snprintf(buf, n, "%Lf", 1.5L)` prints `0.000000`. The C11 `_Generic` path, which other C compilers use, does wrap `long double`.
* A call takes at most 63 arguments after the format string. A 64th fails to compile. The Standard requires a compiler to accept 127 arguments in one function call (5.2.5.2).
* On the C11 `_Generic` path, an argument whose type is not in the selection list fails to compile. `bool` and `unsigned char *` are two such types.
* `npf_vsnprintf` and `npf_vpprintf` expect wrapped arguments. A `va_list` from a variadic function that did not apply `NPF_MAP_ARGS` holds `double` values, and the conversion reads the wrong type. The README's [Writing variadic wrappers](README.md#writing-variadic-wrappers) section shows the required pattern.

### 2.6 Field width and precision are capped

**Standard**: p4 places no upper bound on a field width or a precision. p16 requires an implementation to support at least 4095 characters from one conversion.

**nanoprintf**: reduces any field width or precision above 65280 to 65280, whether it comes from the format string or from a `*` argument. The cap is 8192 where `int` is 16 bits. If `NANOPRINTF_CONVERSION_BUFFER_SIZE` is set above 65280, the cap equals the buffer size. `%70000d` produces a 65280-character field. `%.70000s` writes at most 65280 bytes of a longer string. Nothing reports that the cap applied. The cap keeps the length arithmetic from overflowing `int`.

### 2.7 Return value

**Standard**: p15 says `fprintf` returns the number of characters transmitted, or a negative value on an output or encoding error. C23 also requires a negative value "if the implementation does not support a specified width length modifier". 7.23.6.5p3 says `snprintf` returns the length the output would have had with enough room.

**nanoprintf**: never returns a negative value.

* An unsupported `wN` or `wfN` width, such as `%w24d`, or any `wN` when fixed-width support is compiled out, prints the text of the specification, and the call returns the character count. C23 requires a negative return.
* The count is an `int`. The Standard does not say what happens when the output is longer than `INT_MAX` characters. In nanoprintf the count overflows, which is undefined behavior. On a target with a 16-bit `int`, that happens past 32767 characters.

### 2.8 Locale and multibyte format strings

**Standard**: 7.1.1p2 says the decimal-point character can be changed by `setlocale`, through `LC_NUMERIC`. p3 defines the format as a multibyte character sequence.

**nanoprintf**: always prints `.` and never reads the locale, which matches the `"C"` locale only. It scans the format one byte at a time for `%`. That gives the same result for any encoding in which the byte 0x25 only ever stands for `%`, such as ASCII and UTF-8.

### 2.9 Empty string on overflow (opt-in)

**Standard**: 7.23.6.5p2 says `snprintf` writes the output up to the size limit and puts a null character at the end of what it wrote.

**nanoprintf**: with `NANOPRINTF_SNPRINTF_SAFE_EMPTY_STRING_ON_OVERFLOW` defined, output that does not fit leaves a null character in the first byte of the buffer instead. The other bytes hold the truncated output, and the last byte is not a null character. The return value does not change. Without the macro, nanoprintf follows the Standard.

## 3. Implementation-defined choices

These choices conform to the Standard. They are listed so callers know what to expect.

* **Infinity and NaN** (p8): nanoprintf prints `inf` and `nan`, or `INF` and `NAN` for uppercase conversions. It never prints `infinity` or a `nan(...)` payload. The sign bit is honored, so a NaN with the sign bit set prints `-nan`. The `+` and space flags apply. The `0` and `#` flags have no effect, as footnote 323 requires.
* **`%p`** (p8): the value of the pointer in lowercase hexadecimal, zero-filled to two digits per byte of `void *`. On a 64-bit target, a null pointer prints `0000000000000000` and `0x1234` prints `0000000000001234`. There is no `0x` prefix. Field width and `-` apply. The `+` and space flags are ignored. Section [4](#4-undefined-behavior) covers `#`, `0`, a precision, and length modifiers on `%p`, which the Standard leaves undefined.
* **`%a` layout** (p8, footnote 324): without a precision, `%a` prints 13 digits after the point, trailing zeros included. That is enough for an exact representation, which is what p8 asks for. `%a` of `1.0` prints `0x1.0000000000000p+0`. Normal values have a leading `1`. Subnormal values have a leading `0` and the exponent `-1022`: `%a` of `5e-324` prints `0x0.0000000000001p-1022`. Zero prints `0x0.0000000000000p+0`. A carry from rounding can make the leading digit `2`: `%.1a` of `0x1.f8p+0` prints `0x2.0p+0`.
* **`%B`** (C23 p8, p14): optional in C23. nanoprintf supports it whenever `%b` is enabled, with the `0B` prefix under `#`.
* **`wN` and `wfN` widths** (C23 p7): the Standard requires every width `<stdint.h>` defines. nanoprintf supports 8, 16, 32, and 64. It stops the build with `#error` if `<stdint.h>` defines a 24-, 40-, 48-, or 128-bit type, rather than build without it.

`%c` of a null character is not a choice. p8 defines it, and nanoprintf follows it: `npf_snprintf(buf, n, "a%cb", 0)` returns 3 and writes the bytes `61 00 62`.

The `npf_putc` callback is not part of the Standard, but one property matters for code ported from `fputc`. The callback receives each byte as `(int)(char)`. Where `char` is signed, bytes 0x80 to 0xFF arrive as negative values, so `%c` of `0xE9` reaches the callback as `-23`. `fputc` works with `unsigned char` values instead.

## 4. Undefined behavior

The Standard places no requirement on these calls. The list records what nanoprintf does today, so the behavior can be recognized. It is not a promise.

* **Invalid conversion specification** (p9): nanoprintf writes the `%`, then continues at the character after it, so the rest of the specification is copied as ordinary text. No argument is consumed. `"%y|%d"` with the arguments `1, 2` prints `%y|1`. A `%` at the end of the format prints `%`. Features that are compiled out behave the same way (see [1](#1-configuration)).
* **Uppercase conversion letters**: the parser folds the conversion character to lowercase, so an uppercase letter works as the lowercase conversion of the same name. `%D`, `%I`, `%U`, and `%O` convert as `%d`, `%i`, `%u`, and `%o`. `%C` and `%S` convert as `%c` and `%s`. `%P` prints a pointer in uppercase hexadecimal. `%N` performs the `%n` write-back when write-back is enabled. Of the uppercase letters, the Standard defines only `X`, `F`, `E`, `G`, `A`, and `B`.
* **Negative literal precision** (p4 requires a nonnegative decimal integer): `%.-5d` is accepted, and the precision is treated as omitted, not as 5. `%.-5d` of `42` prints `42`. Because the precision counts as omitted, the `0` flag stays in effect: `%08.-5d` of `42` prints `00000042`.
* **Precision on `c`, `p`, `%`, or `n`** (p4): taken as a minimum digit count. `%.3c` of `'x'` prints `00x`, `%.2%` prints `0%`, and `%.3n` writes `000` to the output. `%.2p` of `0x1234` prints `1234`, and `%.0p` of a null pointer prints nothing.
* **A length modifier on a conversion it does not apply to** (p7): ignored. `%hs`, `%lp`, and `%hf` behave as `%s`, `%p`, and `%f`. `%Ld` reads an `int`.
* **A flag on a conversion it does not apply to** (p6): `0` pads `s`, `c`, `p`, and `%` with zeros, so `%05s` of `"ab"` prints `000ab`. `#` has no effect on `d`, `i`, `u`, `c`, and `s`. On `p` it adds `0x` to a non-null pointer, as `%#x` does for a nonzero value.
* **Anything between the two characters of `%%`** (p8 requires exactly `%%`): flags, field width, and precision apply. `%5%` prints `    %`. `%*%` consumes an `int`.
* **`%n` with flags, a field width, or a precision** (p8): the count is stored first, then the padding is written. `"ab%5n|"` stores 2 and prints `ab     |`.
* **`%n` with a null pointer**: nanoprintf writes through it.
* **`%s` with a null pointer** (p8 requires a pointer to an array): prints nothing, as if the argument were `""`. A field width still applies.
* **`snprintf` with a null buffer and a nonzero size** (7.23.6.5p2 allows a null pointer only when the size is 0): nothing is written, and the call returns the length.
* **An argument of the wrong type** (p9): read as the type the conversion specification names.

## 5. Items from issue #299

[Issue #299](https://github.com/charlesnicholson/nanoprintf/issues/299) asked for this list. Some of its items no longer describe nanoprintf, and a few describe conforming behavior as non-standard. This table gives the current state of each one.

| Item in the issue | Current state | Section |
|---|---|---|
| `long double` is parsed, then cast to `double` | Still true | [2.4](#24-long-double-is-converted-to-double) |
| `a`, `e`, `g` are parsed and treated as `f` | No longer true. Each is a real conversion behind its own option. Compiled out, it prints verbatim | [1](#1-configuration) |
| `aA eE gG H D DD wN wfN` are unsupported | Only `H`, `D`, and `DD` remain unsupported. They apply to the decimal floating types, which C23 makes a conditional feature (6.2.5p13) | [1](#1-configuration) |
| Which options enable which features | Documented | [1](#1-configuration) |
| No `l` for `%c` and `%s` | Still unsupported. `%lc` and `%ls` used to print the argument's bytes as narrow text, and now print verbatim | [2.3](#23-lc-and-ls-are-not-supported) |
| Rounding is imperfect | Still true. Ties now go to even | [2.2](#22-decimal-conversions-are-not-correctly-rounded) |
| Output that does not fit the buffer prints `err` | Still true. The exact conditions are listed | [2.1](#21-floating-point-output-is-limited-by-the-conversion-buffer) |
| Infinity prints as `inf` | Implementation-defined, conforms | [3](#3-implementation-defined-choices) |
| NaN prints without a payload | Implementation-defined, conforms | [3](#3-implementation-defined-choices) |
| `%p` is `%#.*x` with the pointer's width | Close. There is no `0x` without `#`, and length modifiers are ignored | [3](#3-implementation-defined-choices) |
| `%B` is supported | True when binary support is enabled. C23 makes `%B` optional | [3](#3-implementation-defined-choices) |
| A null `%s` argument prints as `""` | True. The Standard makes this undefined | [4](#4-undefined-behavior) |
| A null character for `%c` is undefined | Not so. The Standard defines it, and nanoprintf writes and counts the byte | [3](#3-implementation-defined-choices) |
| A null `%n` pointer is not checked | True. The Standard makes this undefined | [4](#4-undefined-behavior) |
| An unknown specification is printed verbatim | True. The `%` is written and scanning continues at the next character. Uppercase letters now fold to lowercase conversions | [4](#4-undefined-behavior) |
| A disabled specification is treated as unknown and its argument is not consumed | True | [1](#1-configuration) |
| A width or precision above `INT_MAX` is undefined behavior | Fixed. Both are capped at 65280 | [2.6](#26-field-width-and-precision-are-capped) |
| A negative `*` width acting as `-` plus the magnitude is non-standard | Not so. p5 requires exactly that. A negative `*` precision counts as omitted, which p5 also requires | none, conforms |
| `%.-5i` is accepted and the minus is ignored | Accepted, but the precision is treated as omitted, not as 5 | [4](#4-undefined-behavior) |
| An option to parse disabled specifications and consume their arguments | Not implemented | [1](#1-configuration) |
| Comment: exact conditions for `err` in `%f` and `%a` | Listed per conversion and per build. `%a` prints `err` for a precision above size − 8 | [2.1](#21-floating-point-output-is-limited-by-the-conversion-buffer) |
| Comment: is `%f` exact when N − log2(5) ≥ M | No. A counterexample for `uint64_t` and two for `uint8_t` are given | [2.2](#22-decimal-conversions-are-not-correctly-rounded) |
