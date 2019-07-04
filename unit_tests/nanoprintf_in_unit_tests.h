#define NANOPRINTF_USE_C99_FORMAT_SPECIFIERS 1
#define NANOPRINTF_USE_FLOAT_FORMAT_SPECIFIERS 1

#include "../nanoprintf.h"

#include <string>

// CHECK_EQUAL("abc", std::string("abc")) does "first == second"
inline int operator==(char const* literal, std::string const& s) {
    return s == literal;
}
