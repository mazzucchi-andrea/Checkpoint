#include <emmintrin.h> // SSE2

#include <stdio.h>
#include <string.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <sys/types.h>

#include <time.h>

#ifndef MEM_SIZE
#define MEM_SIZE 0x100000UL
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

float __attribute__((optimize("unroll-loops"))) function(int8_t *area, int64_t value) {
    int offset = 0;
    int64_t read_value;
    clock_t begin, end;
    double time_spent;
    begin = clock();
    for (int i = 0; i < WRITES; i++) {
        offset %= (MEM_SIZE - 8 + 1);
        *(int64_t *)(area + offset) = value;
        offset += 4;
    }
    for (int i = 0; i < READS; i++) {
        offset %= (MEM_SIZE - 8 + 1);
        read_value = *(int64_t *)(area + offset);
        offset += 4;
    }
    end = clock();
    time_spent = (double)(end - begin) / CLOCKS_PER_SEC;
    return time_spent;
}

void clean_cache(int8_t *area) {
    int cache_line_size = __builtin_cpu_supports("sse2") ? 64 : 32;
    for (int i = 0; i < (2 * MEM_SIZE + MEM_SIZE); i += (cache_line_size / 8)) {
        _mm_clflush(area + i);
    }
}

int main(int argc, char **argv) {
    double time = 0;
    char buffer[1024];
    char *endptr;
    int ret;
    int64_t value;

    srand(42);
    value = rand() % (0xFFFFFFFFFFFFFFFF - 1 + 1) + 1;

    size_t alignment = 8 * (1024 * MEM_SIZE);
    int8_t *area = (int8_t *)aligned_alloc(alignment, MEM_SIZE * 2);
    if (area == NULL) {
        perror("aligned_alloc failed\n");
        return EXIT_FAILURE;
    }
    memset(area, 0, MEM_SIZE);

    for (int i = 0; i < 128; i++) {
#if CF == 1
        clean_cache(area);
#endif
        time += function(area, value);
    }

    FILE *file = fopen("mvm_test_results.csv", "a");
    if (file == NULL) {
        fprintf(stderr, "Error opening file!\n");
        return 1;
    }
    sprintf(buffer, "0x%lx,%d,%d,%d,%d,%f\n", MEM_SIZE, CF, WRITES + READS, WRITES, READS, time / 128);
    fprintf(file, "%s", buffer);
    fclose(file);

    return EXIT_SUCCESS;
}