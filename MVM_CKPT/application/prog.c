#include <emmintrin.h> // SSE2
#include <errno.h>
#include <immintrin.h> // AVX
#include <linux/limits.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include <sys/types.h>
#include <time.h>

#include "ckpt_setup.h"

double test_checkpoint(u_int8_t *area, int64_t value) {
    int offset = 0;
    __attribute__((unused)) int64_t read_value;
    clock_t begin, end;
    double time_spent;

    begin = clock();
    _set_ckpt(area);
    for (int i = 0; i < WRITES; i++) {
        offset %= (ALLOCATOR_AREA_SIZE - 8);
        *(int64_t *)(area + offset) = value;
        offset += 4;
    }
    for (int i = 0; i < READS; i++) {
        offset %= (ALLOCATOR_AREA_SIZE - 8);
        read_value = *(int64_t *)(area + offset);
        offset += 4;
    }
    end = clock();

    time_spent = (double)(end - begin) / CLOCKS_PER_SEC;
    return time_spent;
}

double test_checkpoint_repeat(u_int8_t *area, int64_t value, int rep) {
    int offset = 0;
    __attribute__((unused)) int64_t read_value;
    clock_t begin, end;
    double time_spent;

    begin = clock();
    for (int r = 0; r < rep; r++) {
        offset = 0;
        for (int i = 0; i < WRITES; i++) {
            offset %= (ALLOCATOR_AREA_SIZE - 8);
            *(int64_t *)(area + offset) = value;
            offset += 4;
        }
        offset = 0;
        for (int i = 0; i < READS; i++) {
            offset %= (ALLOCATOR_AREA_SIZE - 8);
            read_value = *(int64_t *)(area + offset);
            offset += 4;
        }
    }
    end = clock();

    time_spent = (double)(end - begin) / CLOCKS_PER_SEC;
    return time_spent;
}

double restore_area_test(u_int8_t *area) {
    u_int8_t *bitmap = area + 2 * ALLOCATOR_AREA_SIZE;
    u_int8_t *src = area + ALLOCATOR_AREA_SIZE;
    u_int8_t *dst = area;
    u_int16_t current_word;
    int target_offset;
    clock_t begin, end;
    double time_spent;
    begin = clock();

    for (int offset = 0; offset < BITMAP_SIZE; offset += 8) {
        if (*(u_int64_t *)(bitmap + offset) == 0) {
            continue;
        }
        for (int i = 0; i < 8; i += 2) {
            current_word = *(u_int16_t *)(bitmap + offset + i);
            if (current_word == 0) {
                continue;
            }
            for (int k = 0; k < 16; k++) {
                if (((current_word >> k) & 1) == 1) {
#if MOD == 8
                    target_offset = ((offset + i) * 8 + k) * 8;
                    *(u_int64_t *)(dst + target_offset) = *(u_int64_t *)(src + target_offset);
#elif MOD == 16
                    target_offset = ((offset + i) * 8 + k) * 16;
                    *(__int128 *)(dst + target_offset) = *(__int128 *)(src + target_offset);
#elif MOD == 32
                    target_offset = ((offset + i) * 8 + k) * 32;
                    __m256i ckpt_value = _mm256_loadu_si256((__m256i *)(src + target_offset));
                    _mm256_storeu_si256((__m256i *)(dst + target_offset), ckpt_value);
#else
                    target_offset = ((offset + i) * 8 + k) * 64;
                    __m512i ckpt_value = _mm512_load_si512((void *)(src + target_offset));
                    _mm512_storeu_si512((void *)(dst + target_offset), ckpt_value);
#endif
                }
            }
        }
    }

    memset(bitmap, 0, BITMAP_SIZE);

    end = clock();
    time_spent = (double)(end - begin) / CLOCKS_PER_SEC;
    return time_spent;
}

void clean_cache(u_int8_t *area) {
    int cache_line_size = __builtin_cpu_supports("sse2") ? 64 : 32;
    for (int i = 0; i < (2 * ALLOCATOR_AREA_SIZE + BITMAP_SIZE);
         i += (cache_line_size / 8)) {
        _mm_clflush(area + i);
    }
}

int main(void) {
    double wr_time = 0.0, restore_time = 0.0;
    FILE *file;
    int64_t value;
    unsigned long base_addr = 8UL * 1024UL * ALLOCATOR_AREA_SIZE;
    size_t size = 2UL * ALLOCATOR_AREA_SIZE + BITMAP_SIZE;

    _tls_setup();

    srand(42);
    value = rand() % INT64_MAX;

    u_int8_t *area = mmap((void *)base_addr, size, PROT_READ | PROT_WRITE,
                          MAP_ANONYMOUS | MAP_PRIVATE | MAP_FIXED, -1, 0);
    if (area == MAP_FAILED) {
        perror("mmap failed");
        return errno;
    }

    for (int i = 0; i < 128; i++) {
#if CF == 1
        clean_cache(area);
#endif
        wr_time += test_checkpoint(area, value);
#if CF == 1
        clean_cache(area);
#endif
        restore_time += restore_area_test(area);
    }

    file = fopen("ckpt_test_results.csv", "a");
    if (file == NULL) {
        fprintf(stderr, "Error opening file!\n");
        return EXIT_FAILURE;
    }
    fprintf(file, "0x%x,%d,%d,%d,%d,%d,%f,%f\n", ALLOCATOR_AREA_SIZE, CF, MOD,
            WRITES + READS, WRITES, READS, wr_time / 128, restore_time / 128);
    fclose(file);

    file = fopen("ckpt_repeat_test_results.csv", "a");
    if (file == NULL) {
        fprintf(stderr, "Error opening file!\n");
        return EXIT_FAILURE;
    }

    for (int r = 2; r <= 10; r += 2) {
        for (int i = 0; i < 128; i++) {
#if CF == 1
            clean_cache(area);
#endif
            wr_time += test_checkpoint_repeat(area, value, r);
#if CF == 1
            clean_cache(area);
#endif
            restore_time += restore_area_test(area);
        }
        fprintf(file, "0x%x,%d,%d,%d,%d,%d,%d,%f,%f\n", ALLOCATOR_AREA_SIZE, CF,
                MOD, WRITES + READS, WRITES, READS, r, wr_time / 128,
                restore_time / 128);
    }
    fclose(file);

    return EXIT_SUCCESS;
}