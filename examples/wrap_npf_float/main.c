#include "your_project_printf.h"
#include <stdio.h>

int main(void) {
    char buf[64];
    your_project_snprintf(buf, sizeof(buf), "%s %.2f", "pi is", 3.14f);
    printf("%s\n", buf);
}
