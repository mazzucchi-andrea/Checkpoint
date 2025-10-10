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
#endif

/* Initialize the area with the given quadword */
void init_area(int8_t *area, int64_t init_value) {
    for (int i = 0; i < (ALLOCATOR_AREA_SIZE - 8); i += 8) {
        *(int64_t *)(area + i) = init_value;
    }
}

/* Save original values and set the bitarray bit before writing the new value and read */
float test_checkpoint(int8_t *area, int64_t new_value) {
    int offset;
    int64_t read_value;
    clock_t begin, end;
    double time_spent;
    begin = clock();
    for (int i = 0; i < WRITES; i += 4) {
        offset = i % (ALLOCATOR_AREA_SIZE - 8 + 1);
        *(int64_t *)(area + offset) = new_value;
    }
    for (int i = 0; i < READS; i++) {
        offset = i % (ALLOCATOR_AREA_SIZE - 8 + 1);
        read_value = *(int64_t *)(area + offset);
    }
    end = clock();
    time_spent = (double)(end - begin) / CLOCKS_PER_SEC;
    return time_spent;
}

/* Verify that the set bits correspond to the correctly saved quadwords. */
int verify_checkpoint(int8_t *area, int8_t *init_A_copy) {
    int8_t *bitarray = area + ALLOCATOR_AREA_SIZE * 2;
    int8_t *areaS = area + ALLOCATOR_AREA_SIZE;
    for (int offset = 0; offset < BITARRAY_SIZE; offset += 2) {
        u_int16_t current_word = *(u_int16_t *)(bitarray + offset);
        if (current_word == 0) {
            continue;
        }
        for (int k = 0; k < 16; k++) {
            if ((current_word >> k) & 1) {
#if MOD == 64
                int target_offset = (offset * 8 + k) * 8;
                if (*(u_int64_t *)(init_A_copy + target_offset) != *(u_int64_t *)(areaS + target_offset)) {
                    fprintf(stderr,
                            "Checkpoint verify failed:\n"
                            "Offset 0x%x\n"
                            "Area S value: 0x%lx\n"
                            "Area A init Value: 0x%lx\n",
                            offset, *(u_int64_t *)(areaS + target_offset), *(u_int64_t *)(init_A_copy + target_offset));
                    return -1;
#elif MOD == 128
                int target_offset = (offset * 8 + k) * 16;
                if (*(__int128 *)(init_A_copy + target_offset) != *(__int128 *)(areaS + target_offset)) {
                    fprintf(stderr,
                            "Checkpoint verify failed:\n"
                            "BitArray Word Offset: 0x%x\n"
                            "Bit: %d\n"
                            "Target Offset: %d\n"
                            "Area S Value: First qword: 0x%lx Second qword: 0x%lx\n"
                            "Area A init Value First qword: 0x%lx Second qword: 0x%lx\n",
                            offset, k, target_offset, *(int64_t *)(areaS + target_offset),
                            *(int64_t *)(areaS + target_offset + 8), *(int64_t *)(init_A_copy + target_offset),
                            *(int64_t *)(init_A_copy + target_offset + 8));
                    return -1;
#elif MOD == 256
                int target_offset = (offset * 8 + k) * 32;
                if (memcmp(init_A_copy + target_offset, areaS + target_offset, 32)) {
                    fprintf(
                        stderr,
                        "Checkpoint verify failed:\n"
                        "BitArray Word Offset: 0x%x\n"
                        "Bit: %d\n"
                        "Target Offset: %d\n"
                        "Area S Value: First qword: 0x%lx Second qword: 0x%lx Third qword: 0x%lx Fourth qword: 0x%lx\n"
                        "Area A init Value First qword: 0x%lx Second qword: 0x%lx Third qword: 0x%lx Fourth qword: "
                        "0x%lx\n",
                        offset, k, target_offset, *(int64_t *)(areaS + target_offset),
                        *(int64_t *)(areaS + target_offset + 8), *(int64_t *)(areaS + target_offset + 16),
                        *(int64_t *)(areaS + target_offset + 24), *(int64_t *)(init_A_copy + target_offset),
                        *(int64_t *)(init_A_copy + target_offset + 8), *(int64_t *)(init_A_copy + target_offset + 16),
                        *(int64_t *)(init_A_copy + target_offset + 24));
                    return -1;
#endif
                }
            }
        }
    }
    return 0;
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

    for (int offset = 0; offset < BITARRAY_SIZE; offset += 8) {
        if (*(u_int64_t *)(bitarray + offset) == 0) {
            continue;
        }
        for (int i = 0; i < 8; i += 2) {
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
    int64_t init_value, first_value, second_value;

    srand(42);
    init_value = rand() % (0xFFFFFFFFFFFFFFFF - 1 + 1) + 1;
    first_value = rand() % (0xFFFFFFFFFFFFFFFF - 1 + 1) + 1;
    second_value = rand() % (0xFFFFFFFFFFFFFFFF - 1 + 1) + 1;

    size_t alignment = 8 * (1024 * ALLOCATOR_AREA_SIZE);
    int8_t *area = (int8_t *)aligned_alloc(alignment, ALLOCATOR_AREA_SIZE * 2 + BITARRAY_SIZE);
    if (area == NULL) {
        perror("aligned_alloc failed\n");
        return EXIT_FAILURE;
    }

    memset(area, 0, ALLOCATOR_AREA_SIZE * 2 + BITARRAY_SIZE);

    init_area(area, init_value);
    int8_t *init_A_copy =
        (int8_t *)mmap(NULL, ALLOCATOR_AREA_SIZE, PROT_READ | PROT_WRITE, MAP_ANONYMOUS | MAP_PRIVATE, 0, 0);
    if (init_A_copy == MAP_FAILED) {
        perror("mmap init area copy");
        return EXIT_FAILURE;
    }

    memcpy(init_A_copy, area, ALLOCATOR_AREA_SIZE);

    ret = memcmp(area, init_A_copy, ALLOCATOR_AREA_SIZE);
    if (ret) {
        fprintf(stderr, "Area A check failed: %d\n", ret);
        return EXIT_FAILURE;
    }
    clean_cache(area);

    for (int i = 0; i < 128; i++) {
#if CF == 1
        clean_cache(area);
#endif
        wr_time += test_checkpoint(area, first_value);
        if (verify_checkpoint(area, init_A_copy)) {
            return EXIT_FAILURE;
        }

#if CF == 1
        clean_cache(area);
#endif
        restore_time += restore_area(area);
        ret = memcmp(area, init_A_copy, ALLOCATOR_AREA_SIZE);
        if (ret) {
            fprintf(stderr, "Area A restore check failed: 0x%x\n", ret);
            return EXIT_FAILURE;
        }
    }

    FILE *file = fopen("test_results.csv", "a");
    if (file == NULL) {
        fprintf(stderr, "Error opening file!\n");
        return 1;
    }
    sprintf(buffer, "%ld, %d, %d, %d, %d, %d, %f, %f\n", ALLOCATOR_AREA_SIZE, CF, MOD, WRITES + READS, WRITES, READS,
            wr_time, restore_time);
    fprintf(file, "%s", buffer);
    fclose(file);

    return EXIT_SUCCESS;
}