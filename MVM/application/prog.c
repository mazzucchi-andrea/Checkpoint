#include <emmintrin.h> // SSE2
#include <errno.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/mman.h>
#include <sys/types.h>
#include <time.h>

#ifndef MEM_SIZE
#define MEM_SIZE 0x100000
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

double function(u_int8_t *area, int64_t value) {
    int offset = 0;
    int64_t read_value;
    clock_t begin, end;

    begin = clock();
    for (int i = 0; i < WRITES; i++) {
        offset %= (MEM_SIZE - 8);
        *(int64_t *)(area + offset) = value;
        offset += 4;
    }
    offset = 0;
    for (int i = 0; i < READS; i++) {
        offset %= (MEM_SIZE - 8);
        read_value = *(int64_t *)(area + offset);
        offset += 4;
    }
    end = clock();

    return (double)(end - begin) / CLOCKS_PER_SEC;
}

void clean_cache(u_int8_t *area) {
    int cache_line_size = __builtin_cpu_supports("sse2") ? 64 : 32;
    for (int i = 0; i < (2 * MEM_SIZE + MEM_SIZE); i += (cache_line_size / 8)) {
        _mm_clflush(area + i);
    }
}

int main(int argc, char **argv) {
    double time = 0.0;
    int64_t value;

    srand(42);
    value = rand() % INT64_MAX;

    unsigned long base_addr = 8UL * 1024UL * MEM_SIZE;
    size_t size = 2 * MEM_SIZE;
    u_int8_t *area = (u_int8_t *)mmap((void *)base_addr, size, PROT_READ | PROT_WRITE,
                          MAP_ANONYMOUS | MAP_PRIVATE | MAP_FIXED, 0, 0);
    if (area == MAP_FAILED) {
        perror("mmap failed");
        return errno;
    }

    for (int i = 0; i < 128; i++) {
#if CF == 1
        clean_cache(area);
#endif
        time += function(area, value);
    }

    FILE *file = fopen("mvm_test_results.csv", "a");
    if (file == NULL) {
        fprintf(stderr, "Error opening file!\n");
        return EXIT_FAILURE;
    }
    fprintf(file, "0x%x,%d,%d,%d,%d,%f\n", MEM_SIZE, CF, WRITES + READS, WRITES,
            READS, time / 128);
    fclose(file);

    return EXIT_SUCCESS;
}