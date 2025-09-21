#include <asm/prctl.h>

#include <emmintrin.h> // SSE2

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

#ifndef CKPT_SIZE
#define CKPT_SIZE 0x200000UL
#endif

#define BITMAP_SIZE CKPT_SIZE / 8

extern int arch_prctl(int code, unsigned long addr);

void *tls_setup() {
    unsigned long addr;
    addr = (unsigned long)mmap(NULL, 64, PROT_READ | PROT_WRITE, MAP_ANONYMOUS | MAP_PRIVATE, 0, 0);
    *(unsigned long *)addr = addr;
    if (arch_prctl(ARCH_SET_GS, addr)) {
        return NULL;
    }
    return (void *)addr;
}

/* Initialize the area with the given quadword */
void init_area(int8_t *area, int64_t init_value) {
    for (int i = 0; i < (CKPT_SIZE - 8); i++) {
        *(int64_t *)(area + i) = init_value;
    }
}

/* Save original values and set the bitmap bit before writing the new value and read */
void test_checkpoint(int8_t *area, int64_t new_value, int numberOfWrites, int numberOfReads) {
    int offset;
    int64_t read_value;
    clock_t begin, end;
    double time_spent;
    begin = clock();
    for (int i = 0; i < numberOfWrites; i++) {
        offset = i % (CKPT_SIZE - 8 + 1);
        *(int64_t *)(area + offset) = new_value;
    }
    end = clock();

    time_spent = (double)(end - begin) / CLOCKS_PER_SEC;
    printf("Time spent by %d writes: %f s\n", numberOfWrites, time_spent);

    begin = clock();
    for (int i = 0; i < numberOfReads; i++) {
        offset = i % (CKPT_SIZE - 8 + 1);
        read_value = *(int64_t *)(area + offset);
    }
    end = clock();

    time_spent = (double)(end - begin) / CLOCKS_PER_SEC;
    printf("Time spent by %d reads: %f s\n", numberOfReads, time_spent);
}

void test_no_checkpoint(int8_t *area, int64_t new_value, int numberOfWrites, int numberOfReads) {
    int offset;
    int64_t read_value;
    clock_t begin, end;
    double time_spent;
    begin = clock();
    for (int i = 0; i < numberOfWrites; i++) {
        offset = i % (CKPT_SIZE - 8 + 1);
        *(int64_t *)(area + offset) = new_value;
    }
    end = clock();

    time_spent = (double)(end - begin) / CLOCKS_PER_SEC;
    printf("Time spent by %d writes: %f s\n", numberOfWrites, time_spent);

    begin = clock();
    for (int i = 0; i < numberOfReads; i++) {
        offset = i % (CKPT_SIZE - 8 + 1);
        read_value = *(int64_t *)(area + offset);
    }
    end = clock();

    time_spent = (double)(end - begin) / CLOCKS_PER_SEC;
    printf("Time spent by %d reads: %f s\n", numberOfReads, time_spent);
}

/* Verify that the set bits correspond to the correctly saved quadwords. */
int verify_checkpoint(int8_t *area, int8_t *init_A_copy) {
    int8_t *bitmap = area + CKPT_SIZE * 2;
    int8_t *areaS = area + CKPT_SIZE;
    int8_t *areaAcopy = init_A_copy;
    for (int offset = 0; offset < BITMAP_SIZE; offset += 8) {
        if (*(int64_t *)(bitmap + offset) == 0) {
            continue;
        }
        for (int i = 0; i < 8; i++) {
            int8_t current_byte = bitmap[offset + i];
            if (current_byte == 0) {
                continue;
            }

            for (int k = 0; k < 8; k++) {
                if ((current_byte >> k) & 1) {
                    int target_offset = ((offset + i) * 8 + k) * 8;
                    if (*(int64_t *)(areaAcopy + target_offset) != *(int64_t *)(areaS + target_offset)) {
                        fprintf(stderr,
                                "Checkpoint verufy failed:\n"
                                "Offset 0x%x\n"
                                "Area S value: 0x%lx\n"
                                "Area A init Value: 0x%lx\n",
                                offset, *(int64_t *)(areaS + target_offset), *(int64_t *)(areaAcopy + target_offset));
                        return -1;
                    }
                }
            }
        }
    }
    return 0;
}

void restore_area(int8_t *area) {
    int8_t *bitmap = area + CKPT_SIZE * 2;
    int8_t *src = area + CKPT_SIZE;
    int8_t *dst = area;
    for (int offset = 0; offset < BITMAP_SIZE; offset += 8) {
        if (*(int64_t *)(bitmap + offset) == 0) {
            continue;
        }
        for (int i = 0; i < 8; i++) {
            int8_t current_byte = bitmap[offset + i];
            if (current_byte == 0) {
                continue;
            }

            for (int k = 0; k < 8; k++) {
                if ((current_byte >> k) & 1) {
                    int target_offset = ((offset + i) * 8 + k) * 8;
                    *(int64_t *)(dst + target_offset) = *(int64_t *)(src + target_offset);
                }
            }
        }
    }
    memset(bitmap, 0, BITMAP_SIZE);
}

void clean_cache(int8_t *area) {
    int cache_line_size = __builtin_cpu_supports("sse2") ? 64 : 32;
    for (int i = 0; i < (2 * CKPT_SIZE + BITMAP_SIZE); i += (cache_line_size / 8)) {
        _mm_clflush(area + i);
    }
}

/* Two parameters are needed to run the tests:
 * - numberOfWrites: the number of write operations to perform;
 * - numberOfReads: the number of read operations to perform;
 */
int main(int argc, char *argv[]) {
    char *endptr;
    int numberOfWrites, numberOfReads, ret;
    clock_t begin, end;
    int64_t init_value, first_value, second_value;

    if (argc < 3) {
        fprintf(stderr, "Usage: %s <numberOfWrites> <<numberOfReads>\n", argv[0]);
        return EXIT_FAILURE;
    }

    numberOfWrites = strtol(argv[1], &endptr, 10);
    if (*endptr != '\0' || numberOfWrites <= 0) {
        fprintf(stderr, "Number of writes must be an integer greater or eqaul to 1.\n");
        return EXIT_FAILURE;
    }

    numberOfReads = strtol(argv[2], &endptr, 10);
    if (*endptr != '\0' || numberOfReads < 0) {
        fprintf(stderr, "Number of reads must be an integer greater or eqaul to 0.\n");
        return EXIT_FAILURE;
    }

    printf("Number of Writes: %d\n", numberOfWrites);
    printf("Number of Reads: %d\n\n", numberOfReads);

    if (tls_setup() == NULL) {
        fprintf(stderr, "tls_setup failed\n");
        return EXIT_FAILURE;
    }

    srand(42);
    init_value = rand() % (0xFFFFFFFFFFFFFFFF - 1 + 1) + 1;
    first_value = rand() % (0xFFFFFFFFFFFFFFFF - 1 + 1) + 1;
    second_value = rand() % (0xFFFFFFFFFFFFFFFF - 1 + 1) + 1;

    printf("Initial Value 0x%lx\n", init_value);
    printf("First Value 0x%lx\n", first_value);
    printf("Second Value 0x%lx\n\n", second_value);

    size_t alignment = 8 * (1024 * CKPT_SIZE);
    int8_t *area = (int8_t *)aligned_alloc(alignment, CKPT_SIZE * 2 + BITMAP_SIZE);
    if (area == NULL) {
        perror("aligned_alloc failed\n");
        return EXIT_FAILURE;
    }

    memset(area, 0, CKPT_SIZE * 2 + BITMAP_SIZE);

    printf("BaseA: %p\n", area);
    printf("BaseS: %p\n", area + CKPT_SIZE);
    printf("BaseM: %p\n\n", area + (CKPT_SIZE * 2) + BITMAP_SIZE);

    init_area(area, init_value);
    int8_t *init_A_copy = (int8_t *)mmap(NULL, CKPT_SIZE, PROT_READ | PROT_WRITE, MAP_ANONYMOUS | MAP_PRIVATE, 0, 0);
    if (init_A_copy == MAP_FAILED) {
        perror("mmap init area copy");
        return EXIT_FAILURE;
    }

    memcpy(init_A_copy, area, CKPT_SIZE);

    ret = memcmp(area, init_A_copy, CKPT_SIZE);
    if (ret) {
        fprintf(stderr, "Area A check failed: %d\n", ret);
        return EXIT_FAILURE;
    }

    printf("Start Tests\n");
    clean_cache(area);
    test_checkpoint(area, first_value, numberOfWrites, numberOfReads);

    if (verify_checkpoint(area, init_A_copy)) {
        return EXIT_FAILURE;
    }

    printf("\nRepeat writes and reads to verify the time spent on already saved areas.\n");
    clean_cache(area);
    test_checkpoint(area, second_value, numberOfWrites, numberOfReads);

    if (verify_checkpoint(area, init_A_copy)) {
        return EXIT_FAILURE;
    }

    printf("\nTest Restore Function\n");
    clean_cache(area);
    begin = clock();
    restore_area(area);
    end = clock();
    double time_spent = (double)(end - begin) / CLOCKS_PER_SEC;
    printf("Time spent by restore: %f s\n", time_spent);

    ret = memcmp(area, init_A_copy, CKPT_SIZE);
    if (ret) {
        fprintf(stderr, "Area A restore check failed: 0x%x\n", ret);
        return EXIT_FAILURE;
    }

    printf("\nRepeat the writes without checkpoint to measure the overhead\n");
    clean_cache(area);
    test_no_checkpoint(area, first_value, numberOfWrites, numberOfReads);

    printf("\nTest Passed\n");
    return EXIT_SUCCESS;
}