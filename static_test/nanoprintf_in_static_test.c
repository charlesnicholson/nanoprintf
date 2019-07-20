#define NANOPRINTF_VISIBILITY_STATIC
#define NANOPRINTF_IMPLEMENTATION
#include "../nanoprintf.h"

int do_private_nanoprintf_stuff() {
    return npf_snprintf(NULL, 0, "%s", "hello world");
}
