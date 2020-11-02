#include "your_project_printf.h"
#include <cstdio>

int main() {
    char buf[32];
    your_project_snprintf(buf, sizeof(buf), "%s %s", "hello", "nanoprintf");
    printf("%s\n", buf);
}

