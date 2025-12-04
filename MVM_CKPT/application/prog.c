#include <emmintrin.h> // SSE2

#if MOD > 128
#include <immintrin.h> // AVX
#endif

#include <linux/limits.h>

#include <stdbool.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <sys/mman.h>
#include <sys/types.h>

#include <time.h>

#include "ckpt_setup.h"

#ifndef WRITES
#define WRITES 950
#endif

#ifndef READS
#define READS 50
#endif

#ifndef CF
#define CF 0
#endif

/* Save original values and set the bitarray bit before writing the new value and read */
double test_checkpoint(int8_t *area, int64_t value) {
    int offset = 0;
    int64_t read_value;
    clock_t begin, end;
    double time_spent;
    begin = clock();
    for (int i = 0; i < WRITES; i++) {
        offset %= (ALLOCATOR_AREA_SIZE - 8 + 1);
        *(int64_t *)(area + offset) = value;
        offset += 4;
    }
    offset = 0;
    for (int i = 0; i < READS; i++) {
        offset %= (ALLOCATOR_AREA_SIZE - 8 + 1);
        read_value = *(int64_t *)(area + offset);
        offset += 4;
    }
    end = clock();
    time_spent = (double)(end - begin) / CLOCKS_PER_SEC;
    return time_spent;
}

double test_checkpoint_repeat(int8_t *area, int64_t value, int rep) {
    int offset = 0;
    int64_t read_value;
    clock_t begin, end;
    double time_spent;
    begin = clock();
    for (int r = 0; r < rep; r++) {
        offset = 0;
        for (int i = 0; i < WRITES; i++) {
            offset %= (ALLOCATOR_AREA_SIZE - 8 + 1);
            *(int64_t *)(area + offset) = value;
            offset += 4;
        }
        offset = 0;
        for (int i = 0; i < READS; i++) {
            offset %= (ALLOCATOR_AREA_SIZE - 8 + 1);
            read_value = *(int64_t *)(area + offset);
            offset += 4;
        }
    }
    end = clock();
    time_spent = (double)(end - begin) / CLOCKS_PER_SEC;
    return time_spent;
}

double restore_area_test(int8_t *area) {
    int8_t *bitarray = area + 2 * ALLOCATOR_AREA_SIZE;
    int8_t *src = area + ALLOCATOR_AREA_SIZE;
    int8_t *dst = area;
    u_int16_t current_word;
    int target_offset;
    clock_t begin, end;
    double time_spent;
    begin = clock();

    for (int offset = 0; offset < BITARRAY_SIZE; offset += 8) {
        if (*(u_int64_t *)(bitarray + offset) == 0) {
            continue;
        }
        for (int i = 0; i < 32; i += 2) {
            current_word = *(u_int16_t *)(bitarray + offset + i);
            if (current_word == 0) {
                continue;
            }
            for (int k = 0; k < 16; k++) {
                if (((current_word >> k) & 1) == 1) {
#if MOD == 64
                    target_offset = ((offset + i) * 8 + k) * 8;
                    *(u_int64_t *)(dst + target_offset) = *(u_int64_t *)(src + target_offset);
#elif MOD == 128
                    target_offset = ((offset + i) * 8 + k) * 16;
                    *(__int128 *)(dst + target_offset) = *(__int128 *)(src + target_offset);
#elif MOD == 256
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
    memset(bitarray, 0, BITARRAY_SIZE);

    end = clock();
    time_spent = (double)(end - begin) / CLOCKS_PER_SEC;
    return time_spent;
}

void clean_cache(int8_t *area) {
    int cache_line_size = __builtin_cpu_supports("sse2") ? 64 : 32;
    for (int i = 0; i < (2 * ALLOCATOR_AREA_SIZE + BITARRAY_SIZE); i += (cache_line_size / 8)) {
        _mm_clflush(area + i);
    }
}

int main(int argc, char *argv[]) {
    double wr_time = 0, restore_time = 0;
    char *endptr;
    FILE *file;
    int64_t value;

    _tls_setup();

    srand(42);
    value = rand() % (0xFFFFFFFFFFFFFFFF - 1 + 1) + 1;

    size_t alignment = 8 * (1024 * ALLOCATOR_AREA_SIZE);
    int8_t *area = (int8_t *)aligned_alloc(alignment, ALLOCATOR_AREA_SIZE * 2 + BITARRAY_SIZE);
    if (area == NULL) {
        perror("aligned_alloc failed\n");
        return EXIT_FAILURE;
    }
    memset(area, 0, ALLOCATOR_AREA_SIZE * 2 + BITARRAY_SIZE);

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
        return 1;
    }
    fprintf(file, "0x%lx,%d,%d,%d,%d,%d,%f,%f\n", ALLOCATOR_AREA_SIZE, CF, MOD, WRITES + READS, WRITES, READS,
            wr_time / 128, restore_time / 128);
    fclose(file);

    file = fopen("ckpt_repeat_test_results.csv", "a");
    if (file == NULL) {
        fprintf(stderr, "Error opening file!\n");
        return 1;
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

        fprintf(file, "0x%lx,%d,%d,%d,%d,%d,%d,%f,%f\n", ALLOCATOR_AREA_SIZE, CF, MOD, WRITES + READS, WRITES, READS, r,
                wr_time / 128, restore_time / 128);
    }
    fclose(file);

    return EXIT_SUCCESS;
}