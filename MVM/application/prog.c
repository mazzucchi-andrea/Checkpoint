#include <emmintrin.h> // SSE2

#include <stdio.h>
#include <string.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <sys/types.h>

#include <time.h>

#ifndef MEM_SIZE
#define MEM_SIZE 0x400000UL
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

/* Initialize the area with the given quadword */
void init_area(int8_t *area, int64_t init_value) {
    for (int i = 0; i < (MEM_SIZE - 8); i += 8) {
        *(int64_t *)(area + i) = init_value;
    }
}

float function(int8_t *area, int64_t new_value) {
    int offset;
    int64_t read_value;
    clock_t begin, end;
    double time_spent;
    begin = clock();
    for (int i = 0; i < WRITES; i += 4) {
        offset = i % (MEM_SIZE - 8 + 1);
        *(int64_t *)(area + offset) = new_value;
    }
    for (int i = 0; i < READS; i++) {
        offset = i % (MEM_SIZE - 8 + 1);
        read_value = *(int64_t *)(area + offset);
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
    int64_t init_value, new_value;

    srand(42);
    init_value = rand() % (0xFFFFFFFFFFFFFFFF - 1 + 1) + 1;
    new_value = rand() % (0xFFFFFFFFFFFFFFFF - 1 + 1) + 1;

    size_t alignment = 8 * (1024 * MEM_SIZE);
    int8_t *area = (int8_t *)aligned_alloc(alignment, MEM_SIZE);
    if (area == NULL) {
        perror("aligned_alloc failed\n");
        return EXIT_FAILURE;
    }

    memset(area, 0, MEM_SIZE);

    init_area(area, init_value);
    clean_cache(area);

    for (int i = 0; i < 128; i++) {
#if CF == 1
        clean_cache(area);
#endif
        time += function(area, new_value);
    }

    FILE *file = fopen("test_results.csv", "a");
    if (file == NULL) {
        fprintf(stderr, "Error opening file!\n");
        return 1;
    }
    sprintf(buffer, "%d, %d, %d, %d, %f\n", CF, WRITES + READS, WRITES, READS, time);
    fprintf(file, "%s", buffer);
    fclose(file);

    return EXIT_SUCCESS;
}