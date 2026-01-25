#include <emmintrin.h> // SSE2
#include <errno.h>
#include <math.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
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

double test_checkpoint(uint8_t* area, int64_t value) {
    int offset;
    __attribute__((unused)) int64_t read_value;
    clock_t begin, end;

    begin = clock();
    set_ckpt(area);
    offset = 0;
    for (int i = 0; i < WRITES; i++) {
        offset %= (SIZE - 8);
        *(int64_t*)(area + offset) = value;
        offset += 4;
    }
    offset = 0;
    for (int i = 0; i < READS; i++) {
        offset %= (SIZE - 8);
        read_value = *(int64_t*)(area + offset);
        offset += 4;
    }
    end = clock();

    return (double)(end - begin) / CLOCKS_PER_SEC;
}

double test_checkpoint_repeat(uint8_t* area, int64_t value, int rep) {
    int offset = 0;
    __attribute__((unused)) int64_t read_value;
    clock_t begin, end;

    begin = clock();
    set_ckpt(area);
    for (int r = 0; r < rep; r++) {
        offset = 0;
        for (int i = 0; i < WRITES; i++) {
            offset %= (SIZE - 8);
            *(int64_t*)(area + offset) = value;
            offset += 4;
        }
        offset = 0;
        for (int i = 0; i < READS; i++) {
            offset %= (SIZE - 8);
            read_value = *(int64_t*)(area + offset);
            offset += 4;
        }
    }
    end = clock();

    return (double)(end - begin) / CLOCKS_PER_SEC;
}

void clean_cache(uint8_t* area) {
    int cache_line_size = __builtin_cpu_supports("sse2") ? 64 : 32;
    for (int i = 0; i < (SIZE * 2); i += (cache_line_size / 8)) {
        _mm_clflush((void*)(area + i));
    }
}

void mean_ci_95(double* samples, double* mean, double* ci) {
    double sum = 0.0;
    for (int i = 0; i < 50; i++) {
        sum += samples[i];
    }
    *mean = sum / 50;

    double var = 0.0;
    for (int i = 0; i < 50; i++) {
        double d = samples[i] - *mean;
        var += d * d;
    }

    double sd = sqrt(var / (50 - 1)); // sample SD
    double sem = sd / sqrt(50);       // standard error

    const double t95 = 2.009;

    *ci = t95 * sem;
}

int main(void) {
    double ckpt_samples[50], restore_samples[50];
    double ckpt_mean, ckpt_ci, restore_mean, restore_ci;
    unsigned long base_addr;
    clock_t begin, end;
    uint8_t* area;
    int64_t value;
    size_t size;
    FILE* file;

    srand(42);
    value = rand() % INT64_MAX;

    base_addr = 8UL * 1024UL * SIZE;
    size = 2UL * SIZE;
    area =
        (uint8_t*)mmap((void*)base_addr, size, PROT_READ | PROT_WRITE,
                       MAP_ANONYMOUS | MAP_PRIVATE | MAP_FIXED_NOREPLACE, 0, 0);
    if (area == MAP_FAILED) {
        perror("mmap failed");
        return errno;
    }

    for (int i = 0; i < 50; i++) {
#if CF == 1
        clean_cache(area);
#endif
        ckpt_samples[i] = test_checkpoint(area, value);
#if CF == 1
        clean_cache(area);
#endif
        begin = clock();
        restore_area(area);
        end = clock();
        restore_samples[i] = (double)(end - begin) / CLOCKS_PER_SEC;
    }

    file = fopen("simple_ckpt_test_results.csv", "a");
    if (file == NULL) {
        fprintf(stderr, "Error opening simple_ckpt_test_results.csv\n");
        return errno;
    }
    mean_ci_95(ckpt_samples, &ckpt_mean, &ckpt_ci);
    mean_ci_95(restore_samples, &restore_mean, &restore_ci);
    fprintf(file, "0x%x,%d,%d,%d,%d,%.10f,%.10f,%.10f,%.10f\n", SIZE, CF,
            WRITES + READS, WRITES, READS, ckpt_mean, ckpt_ci, restore_mean,
            restore_ci);
    fclose(file);

    file = fopen("simple_ckpt_repeat_test_results.csv", "a");
    if (file == NULL) {
        fprintf(stderr, "Error opening simple_ckpt_repeat_test_results.csv\n");
        return errno;
    }

    for (int r = 2; r <= 10; r += 2) {
        for (int i = 0; i < 50; i++) {
#if CF == 1
            clean_cache(area);
#endif
            ckpt_samples[i] = test_checkpoint_repeat(area, value, r);
#if CF == 1
            clean_cache(area);
#endif
            begin = clock();
            restore_area(area);
            end = clock();
            restore_samples[i] = (double)(end - begin) / CLOCKS_PER_SEC;
        }
        mean_ci_95(ckpt_samples, &ckpt_mean, &ckpt_ci);
        mean_ci_95(restore_samples, &restore_mean, &restore_ci);
        fprintf(file, "0x%x,%d,%d,%d,%d,%d,%.10f,%.10f,%.10f,%.10f\n", SIZE, CF,
                WRITES + READS, WRITES, READS, r, ckpt_mean, ckpt_ci,
                restore_mean, restore_ci);
    }
    fclose(file);

    return EXIT_SUCCESS;
}