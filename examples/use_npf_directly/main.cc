#include "../../nanoprintf.h"
#include <cstdio>

int main() {
    char buf[32];
    npf_snprintf(buf, sizeof(buf), "%s %s", "hello", "nanoprintf");
    printf("%s\n", buf);
}
