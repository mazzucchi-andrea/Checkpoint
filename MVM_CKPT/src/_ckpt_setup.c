#include <asm/prctl.h>

#include <immintrin.h> // AVX

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <sys/mman.h>
#include <sys/types.h>

#include "ckpt_setup.h"

extern int arch_prctl(int code, unsigned long addr);

void _tls_setup() {
    void *addr = mmap(NULL, 128, PROT_READ | PROT_WRITE, MAP_ANONYMOUS | MAP_PRIVATE, 0, 0);
    if (addr == MAP_FAILED) {
        perror("tls_setup fail caused by mmap");
        exit(EXIT_FAILURE);
    }
    if (arch_prctl(ARCH_SET_GS, (unsigned long)addr)) {
        perror("tls_setup fail caused by arch_prctl");
        exit(EXIT_FAILURE);
    }
}

void _restore_area(u_int8_t *area) {
    u_int8_t *bitarray = area + 2 * ALLOCATOR_AREA_SIZE;
    u_int8_t *src = area + ALLOCATOR_AREA_SIZE;
    u_int8_t *dst = area;
    u_int16_t current_word;
    int target_offset;

    for (int offset = 0; offset < BITMAP_SIZE; offset += 8) {
        if (*(u_int64_t *)(bitarray + offset) == 0) {
            continue;
        }
        for (int i = 0; i < 8; i += 2) {
            current_word = *(u_int16_t *)(bitarray + offset + i);
            if (current_word == 0) {
                continue;
            }
            for (int k = 0; k < 16; k++) {
                if (((current_word >> k) & 1) == 1) {
                    target_offset = ((offset + i) * 8 + k) * MOD;
                    memcpy(dst + target_offset, src + target_offset, MOD);
                }
            }
        }
    }
    memset(bitarray, 0, BITMAP_SIZE);
}

void _set_ckpt(void *area) {
    memcpy((void *)(area + ALLOCATOR_AREA_SIZE), area, ALLOCATOR_AREA_SIZE);
    memset((void *)(area + 2 * ALLOCATOR_AREA_SIZE), 0, BITMAP_SIZE);
}