#include <emmintrin.h> // SSE2
#include <errno.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <sys/mman.h>
#include <time.h>

#ifndef SIZE
#define SIZE 0x100000UL
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
double test_checkpoint(u_int8_t *area, u_int8_t *area_copy, int64_t value) {
    int offset = 0;
    int64_t read_value;
    clock_t begin, end;
    double time_spent;
    begin = clock();
    memcpy(area_copy, area, SIZE);
    for (int i = 0; i < WRITES; i++) {
        offset %= (SIZE - 8 + 1);
        *(int64_t *)(area + offset) = value;
        offset += 4;
    }
    offset = 0;
    for (int i = 0; i < READS; i++) {
        offset %= (SIZE - 8 + 1);
        read_value = *(int64_t *)(area + offset);
        offset += 4;
    }
    end = clock();
    time_spent = (double)(end - begin) / CLOCKS_PER_SEC;
    return time_spent;
}

double test_checkpoint_repeat(u_int8_t *area, u_int8_t *area_copy,
                              int64_t value, int rep) {
    int offset = 0;
    int64_t read_value;
    clock_t begin, end;
    double time_spent;
    begin = clock();
    memcpy(area_copy, area, SIZE);
    for (int r = 0; r < rep; r++) {
        offset = 0;
        for (int i = 0; i < WRITES; i++) {
            offset %= (SIZE - 8 + 1);
            *(int64_t *)(area + offset) = value;
            offset += 4;
        }
        offset = 0;
        for (int i = 0; i < READS; i++) {
            offset %= (SIZE - 8 + 1);
            read_value = *(int64_t *)(area + offset);
            offset += 4;
        }
    }
    end = clock();
    time_spent = (double)(end - begin) / CLOCKS_PER_SEC;
    return time_spent;
}

double restore_area(u_int8_t *area, u_int8_t *area_copy) {
    clock_t begin, end;
    double time_spent;
    begin = clock();
    memcpy(area, area_copy, SIZE);
    end = clock();
    time_spent = (double)(end - begin) / CLOCKS_PER_SEC;
    return time_spent;
}

void clean_cache(u_int8_t *area) {
    int cache_line_size = __builtin_cpu_supports("sse2") ? 64 : 32;
    for (int i = 0; i < SIZE; i += (cache_line_size / 8)) {
        _mm_clflush(area + i);
    }
}

int main(int argc, char *argv[]) {
    double wr_time = 0, restore_time = 0;
    char *endptr;
    FILE *file;
    int64_t value;

    srand(42);
    value = rand() % INT64_MAX;

    unsigned long base_addr = 8UL * 1024UL * SIZE;
    size_t size = 2 * SIZE;
    u_int8_t *area = mmap((void *)base_addr, size, PROT_READ | PROT_WRITE,
                          MAP_ANONYMOUS | MAP_PRIVATE | MAP_FIXED, 0, 0);
    if (area == MAP_FAILED) {
        perror("mmap failed");
        return errno;
    }
    u_int8_t *area_copy = (u_int8_t *)(area + SIZE);

    for (int i = 0; i < 128; i++) {
#if CF == 1
        clean_cache(area);
#endif
        wr_time += test_checkpoint(area, area_copy, value);
#if CF == 1
        clean_cache(area);
#endif
        restore_time += restore_area(area, area_copy);
    }

    file = fopen("simple_ckpt_test_results.csv", "a");
    if (file == NULL) {
        fprintf(stderr, "Error opening file!\n");
        return 1;
    }
    fprintf(file, "0x%lx,%d,%d,%d,%d,%f,%f\n", SIZE, CF, WRITES + READS, WRITES,
            READS, wr_time / 128, restore_time / 128);
    fclose(file);

    file = fopen("simple_ckpt_repeat_test_results.csv", "a");
    if (file == NULL) {
        fprintf(stderr, "Error opening file!\n");
        return 1;
    }

    for (int r = 2; r <= 10; r += 2) {
        for (int i = 0; i < 128; i++) {
#if CF == 1
            clean_cache(area);
#endif
            wr_time += test_checkpoint_repeat(area, area_copy, value, r);
#if CF == 1
            clean_cache(area);
#endif
            restore_time += restore_area(area, area_copy);
        }

        fprintf(file, "0x%lx,%d,%d,%d,%d,%d,%f,%f\n", SIZE, CF, WRITES + READS,
                WRITES, READS, r, wr_time / 128, restore_time / 128);
    }
    fclose(file);

    return EXIT_SUCCESS;
}