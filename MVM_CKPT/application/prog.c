#include <emmintrin.h> // SSE2

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

#ifndef ALLOCATOR_AREA_SIZE
#define ALLOCATOR_AREA_SIZE 0x100000UL
#endif

#ifndef WRITES
#define WRITES 950
#endif

#ifndef READS
#define READS 50
#endif

#ifndef CF
#define CF 0
#endif

#ifndef MOD
#define MOD 64
#endif

#if MOD == 64
#define BITARRAY_SIZE ALLOCATOR_AREA_SIZE / 8
#elif MOD == 128
#define BITARRAY_SIZE ALLOCATOR_AREA_SIZE / 16
#elif MOD == 256
#define BITARRAY_SIZE ALLOCATOR_AREA_SIZE / 32
#else
#define BITARRAY_SIZE ALLOCATOR_AREA_SIZE / 64
#endif

/* Save original values and set the bitarray bit before writing the new value and read */
float __attribute__((optimize("unroll-loops"))) test_checkpoint(int8_t *area, int64_t value) {
    int offset;
    int64_t read_value;
    clock_t begin, end;
    double time_spent;
    begin = clock();
    for (int i = 0; i < WRITES; i += 4) {
        offset = i % (ALLOCATOR_AREA_SIZE - 8 + 1);
        *(int64_t *)(area + offset) = value;
    }
    for (int i = 0; i < READS; i++) {
        offset = i % (ALLOCATOR_AREA_SIZE - 8 + 1);
        read_value = *(int64_t *)(area + offset);
    }
    end = clock();
    time_spent = (double)(end - begin) / CLOCKS_PER_SEC;
    return time_spent;
}

double restore_area(int8_t *area) {
    int8_t *bitarray = area + 2 * ALLOCATOR_AREA_SIZE;
    int8_t *src = area + ALLOCATOR_AREA_SIZE;
    int8_t *dst = area;
    u_int16_t current_word;
    int target_offset;
    clock_t begin, end;
    double time_spent;
    begin = clock();

    for (int offset = 0; offset < BITARRAY_SIZE; offset += 32) {
        __m256i bitarray_vec = _mm256_loadu_si256((__m256i *)(bitarray + offset));
        if (_mm256_testz_si256(bitarray_vec, bitarray_vec)) {
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
    char buffer[1024];
    char *endptr;
    int ret;
    int64_t value;

    srand(42);
    value = rand() % (0xFFFFFFFFFFFFFFFF - 1 + 1) + 1;

    size_t alignment = 8 * (1024 * ALLOCATOR_AREA_SIZE);
    int8_t *area = (int8_t *)aligned_alloc(alignment, ALLOCATOR_AREA_SIZE * 2 + BITARRAY_SIZE);
    if (area == NULL) {
        perror("aligned_alloc failed\n");
        return EXIT_FAILURE;
    }
    memset(area, 0, ALLOCATOR_AREA_SIZE * 2 + BITARRAY_SIZE);
    clean_cache(area);

    for (int i = 0; i < 128; i++) {
#if CF == 1
        clean_cache(area);
#endif
        wr_time += test_checkpoint(area, value);
#if CF == 1
        clean_cache(area);
#endif
        restore_time += restore_area(area);
    }

    FILE *file = fopen("ckpt_test_results.csv", "a");
    if (file == NULL) {
        fprintf(stderr, "Error opening file!\n");
        return 1;
    }
    sprintf(buffer, "0x%lx,%d,%d,%d,%d,%d,%f,%f\n", ALLOCATOR_AREA_SIZE, CF, MOD, WRITES + READS, WRITES, READS,
            wr_time / 128, restore_time / 128);
    fprintf(file, "%s", buffer);
    fclose(file);

    return EXIT_SUCCESS;
}