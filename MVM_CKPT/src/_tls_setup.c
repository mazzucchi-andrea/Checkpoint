#include <asm/prctl.h>
#include <stddef.h>
#include <sys/mman.h>

#include "tls_setup.h"

extern int arch_prctl(int code, unsigned long addr);

void *tls_setup() {
    unsigned long addr;
    addr = (unsigned long)mmap(NULL, 64, PROT_READ | PROT_WRITE, MAP_ANONYMOUS | MAP_PRIVATE, 0, 0);
    *(unsigned long *)addr = addr;
    if (arch_prctl(ARCH_SET_GS, addr)) {
        return NULL;
    }
    return (void *)addr;
}