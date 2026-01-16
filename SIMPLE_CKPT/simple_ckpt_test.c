#include <emmintrin.h> // SSE2
#include <errno.h>
#include <stdint.h>
#include <stdio.h>
#include <sys/mman.h>
#include <time.h>

#include "ckpt.h"

#ifndef SIZE
#define SIZE 0x100000
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

/* Save original values and set the bitarray bit before writing the new value
 * and read */
double test_checkpoint(uint8_t *area, int64_t value) {
    int offset = 0;
    int64_t read_value;
    clock_t begin, end;

    begin = clock();
    set_ckpt(area);
    for (int i = 0; i < WRITES; i++) {
        offset %= (SIZE - 8);
        *(int64_t *)(area + offset) = value;
        offset += 4;
    }
    offset = 0;
    for (int i = 0; i < READS; i++) {
        offset %= (SIZE - 8);
        read_value = *(int64_t *)(area + offset);
        offset += 4;
    }
    end = clock();

    return (double)(end - begin) / CLOCKS_PER_SEC;
}

double test_checkpoint_repeat(uint8_t *area, int64_t value, int rep) {
    int offset = 0;
    int64_t read_value;
    clock_t begin, end;

    begin = clock();
    set_ckpt(area);
    for (int r = 0; r < rep; r++) {
        offset = 0;
        for (int i = 0; i < WRITES; i++) {
            offset %= (SIZE - 8);
            *(int64_t *)(area + offset) = value;
            offset += 4;
        }
        offset = 0;
        for (int i = 0; i < READS; i++) {
            offset %= (SIZE - 8);
            read_value = *(int64_t *)(area + offset);
            offset += 4;
        }
    }
    end = clock();

    return (double)(end - begin) / CLOCKS_PER_SEC;
}

void clean_cache(uint8_t *area) {
    int cache_line_size = __builtin_cpu_supports("sse2") ? 64 : 32;
    for (int i = 0; i < SIZE * 2; i += (cache_line_size / 8)) {
        _mm_clflush(area + i);
    }
}

int main(int argc, char *argv[]) {
    double wr_time, restore_time;
    unsigned long base_addr;
    clock_t begin, end;
    uint8_t *area;
    int64_t value;
    size_t size;
    FILE *file;

    srand(42);
    value = rand() % INT64_MAX;

    base_addr = 8UL * 1024UL * SIZE;
    size = 2 * SIZE;
    area = (uint8_t *)mmap((void *)base_addr, size, PROT_READ | PROT_WRITE,
                           MAP_ANONYMOUS | MAP_PRIVATE | MAP_FIXED, 0, 0);
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
        restore_area(area);
        end = clock();
        restore_time += (double)(end - begin) / CLOCKS_PER_SEC;
    }

    file = fopen("simple_ckpt_test_results.csv", "a");
    if (file == NULL) {
        fprintf(stderr, "Error opening file!\n");
        return 1;
    }
    fprintf(file, "0x%x,%d,%d,%d,%d,%f,%f\n", SIZE, CF, WRITES + READS, WRITES,
            READS, wr_time / 128, restore_time / 128);
    fclose(file);

    file = fopen("simple_ckpt_repeat_test_results.csv", "a");
    if (file == NULL) {
        fprintf(stderr, "Error opening file!\n");
        return 1;
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
            restore_area(area);
            end = clock();
            restore_time += (double)(end - begin) / CLOCKS_PER_SEC;
        }

        fprintf(file, "0x%x,%d,%d,%d,%d,%d,%f,%f\n", SIZE, CF, WRITES + READS,
                WRITES, READS, r, wr_time / 128, restore_time / 128);
    }
    fclose(file);

    return EXIT_SUCCESS;
}