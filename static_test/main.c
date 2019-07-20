#include "../nanoprintf.h"

int npf_snprintf(char *buffer, size_t bufsz, const char *format, ...) {
    (void)buffer;
    (void)bufsz;
    (void)format;
    return 0;
}

int npf_vsnprintf(char *buffer, size_t bufsz, char const *format,
                  va_list vlist) {
    (void)buffer;
    (void)bufsz;
    (void)format;
    (void)vlist;
    return 0;
}

int npf_pprintf(npf_putc pc, void *pc_ctx, char const *format, ...) {
    (void)pc;
    (void)pc_ctx;
    (void)format;
    return 0;
}

int npf_vpprintf(npf_putc pc, void *pc_ctx, char const *format, va_list vlist) {
    (void)pc;
    (void)pc_ctx;
    (void)format;
    (void)vlist;
    return 0;
}

extern void do_private_nanoprintf_stuff();

int main(void) {
    do_private_nanoprintf_stuff();
    return 0;
}
