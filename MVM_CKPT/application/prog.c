#include <emmintrin.h> // SSE2
#include <errno.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/mman.h>
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

double test_checkpoint(uint8_t *area, int64_t value) {
    int offset;
    __attribute__((unused)) int64_t read_value;
    clock_t begin, end;

    begin = clock();
    _set_ckpt(area);
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
    end = clock();

    return (double)(end - begin) / CLOCKS_PER_SEC;
}

double test_checkpoint_repeat(uint8_t *area, int64_t value, int rep) {
    int offset;
    __attribute__((unused)) int64_t read_value;
    clock_t begin, end;

    begin = clock();
    _set_ckpt(area);
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

    return (double)(end - begin) / CLOCKS_PER_SEC;
}

void clean_cache(uint8_t *area) {
    int cache_line_size = __builtin_cpu_supports("sse2") ? 64 : 32;
    for (int i = 0; i < (2 * ALLOCATOR_AREA_SIZE + BITMAP_SIZE);
         i += (cache_line_size / 8)) {
        _mm_clflush(area + i);
    }
}

int main(void) {
    double wr_time, restore_time;
    unsigned long base_addr;
    clock_t begin, end;
    uint8_t *area;
    int64_t value;
    size_t size;
    FILE *file;

    _tls_setup();

    srand(42);
    value = rand() % INT64_MAX;

    base_addr = 8UL * 1024UL * ALLOCATOR_AREA_SIZE;
    size = 2UL * ALLOCATOR_AREA_SIZE + BITMAP_SIZE;
    area = (uint8_t *)mmap((void *)base_addr, size, PROT_READ | PROT_WRITE,
                           MAP_ANONYMOUS | MAP_PRIVATE | MAP_FIXED, -1, 0);
    if (area == MAP_FAILED) {
        perror("mmap failed");
        return errno;
    }

    wr_time = 0.0;
    restore_time = 0.0;
    for (int i = 0; i < 128; i++) {
#if CF == 1
        clean_cache(area);
#endif
        wr_time += test_checkpoint(area, value);
#if CF == 1
        clean_cache(area);
#endif
        begin = clock();
        _restore_area(area);
        end = clock();
        restore_time += (double)(end - begin) / CLOCKS_PER_SEC;
    }

    file = fopen("ckpt_test_results.csv", "a");
    if (file == NULL) {
        fprintf(stderr, "Error opening ckpt_test_results.csv\n");
        return errno;
    }
    fprintf(file, "0x%x,%d,%d,%d,%d,%d,%f,%f\n", ALLOCATOR_AREA_SIZE, CF, MOD,
            WRITES + READS, WRITES, READS, wr_time / 128, restore_time / 128);
    fclose(file);

    file = fopen("ckpt_repeat_test_results.csv", "a");
    if (file == NULL) {
        fprintf(stderr, "Error opening ckpt_repeat_test_results.csv\n");
        return errno;
    }

    for (int r = 2; r <= 10; r += 2) {
        wr_time = 0.0;
        restore_time = 0.0;
        for (int i = 0; i < 128; i++) {
#if CF == 1
            clean_cache(area);
#endif
            wr_time += test_checkpoint_repeat(area, value, r);
#if CF == 1
            clean_cache(area);
#endif
            begin = clock();
            _restore_area(area);
            end = clock();
            restore_time += (double)(end - begin) / CLOCKS_PER_SEC;
        }
        fprintf(file, "0x%x,%d,%d,%d,%d,%d,%d,%f,%f\n", ALLOCATOR_AREA_SIZE, CF,
                MOD, WRITES + READS, WRITES, READS, r, wr_time / 128,
                restore_time / 128);
    }
    fclose(file);

    return EXIT_SUCCESS;
}