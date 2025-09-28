#include <asm/prctl.h>

#include <emmintrin.h> // SSE2

#include <linux/limits.h>

#include <immintrin.h> // For AVX2 intrinsics

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <sys/mman.h>
#include <sys/types.h>

#include <time.h>

#ifndef SIZE
#define SIZE 0x200000UL
#endif

#define BITMAP_SIZE SIZE / 8

#define STR1(x) #x
#define STR(x) STR1(x)

extern int arch_prctl(int code, unsigned long addr);

void checkpoint_qword(int8_t *int_ptr, int offset);

void *tls_setup() {
    unsigned long addr;
    addr = (unsigned long)mmap(NULL, 64, PROT_READ | PROT_WRITE, MAP_ANONYMOUS | MAP_PRIVATE, 0, 0);
    *(unsigned long *)addr = addr;
    if (arch_prctl(ARCH_SET_GS, addr)) {
        return NULL;
    }
    return (void *)addr;
}

inline void checkpoint_qword(int8_t *int_ptr, int offset) {
    int_ptr += offset;

    __asm__ __inline__(
        "mov %%rax, %%gs:0;"
        "mov %%rbx, %%gs:8;"
        "mov %%rcx, %%gs:16;"
        "mov %%rax, %%rcx;"
        "and $0xffffffffffc00000,%%rcx;"
        "and $0x3fffff, %%rax;"
        "test $7, %%rax;"
        "jz second_qword;"
        "and $0x7ffff8, %%rax;"
        "shr $3, %%rax;"
        "mov %%rax, %%rbx;"
        "and $15, %%rbx;"
        "shr $4, %%rax;"
        "bts %%bx, " STR(2 * SIZE) "(%%rcx, %%rax, 2);"
        "jc next_qword;"
        "shl $4, %%rax;"
        "add %%rbx, %%rax;"
        "mov (%%rcx, %%rax, 8), %%rbx;"
        "mov %%rbx, " STR(SIZE) "(%%rcx, %%rax, 8);"
        "jmp check_last;"
        "next_qword:"
        "shl $4, %%rax;"
        "add %%rbx, %%rax;"
        "check_last:"
        "shl $3, %%rax;"
        "add $8, %%rax;"
        "cmp $" STR(SIZE) ", %%rax;"
        "jge end;"
        "second_qword:"
        "shr $3, %%rax;"
        "mov %%rax, %%rbx;"
        "and $15, %%rbx;"
        "shr $4, %%rax;"
        "bts %%bx, " STR(2 * SIZE) "(%%rcx, %%rax, 2);"
        "jc end;"
        "shl $4, %%rax;"
        "add %%rbx, %%rax;"
        "mov (%%rcx, %%rax, 8), %%rbx;"
        "mov %%rbx, " STR(SIZE) "(%%rcx, %%rax, 8);"
        "end:"
        "mov %%gs:0, %%rax;"
        "mov %%gs:8, %%rbx;"
        "mov %%gs:16, %%rcx;"
        :
        : "a"(int_ptr)
        : "rbx", "rcx", "memory");
}

/* Initialize the area with the given quadword */
void init_area(int8_t *area, int64_t init_value) {
    for (int i = 0; i < (SIZE - 8); i++) {
        *(int64_t *)(area + i) = init_value;
    }
}

/* Save original values and set the bitmap bit before writing the new value */
void test_checkpoint(int8_t *area, int64_t new_value, int numberOfWrites, int numberOfReads) {
    int offset;
    int64_t read_value;
    clock_t begin, end;
    double time_spent;
    begin = clock();
    for (int i = 0; i < numberOfWrites; i++) {
        offset = i % (SIZE - 8 + 1);
        checkpoint_qword(area, offset);
        *(int64_t *)(area + offset) = new_value;
    }
    end = clock();

    time_spent = (double)(end - begin) / CLOCKS_PER_SEC;
    printf("Time spent by %d writes: %f s\n", numberOfWrites, time_spent);

    begin = clock();
    for (int i = 0; i < numberOfReads; i++) {
        offset = i % (SIZE - 8 + 1);
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
        offset = i % (SIZE - 8 + 1);
        *(int64_t *)(area + offset) = new_value;
    }
    end = clock();

    time_spent = (double)(end - begin) / CLOCKS_PER_SEC;
    printf("Time spent by %d writes: %f s\n", numberOfWrites, time_spent);

    begin = clock();
    for (int i = 0; i < numberOfReads; i++) {
        offset = i % (SIZE - 8 + 1);
        read_value = *(int64_t *)(area + offset);
    }
    end = clock();

    time_spent = (double)(end - begin) / CLOCKS_PER_SEC;
    printf("Time spent by %d reads: %f s\n", numberOfReads, time_spent);
}

/* Verify that the set bits correspond to the correctly saved quadwords. */
int verify_checkpoint(int8_t *area, int8_t *init_A_copy) {
    int8_t *bitmap = area + SIZE * 2;
    int8_t *areaS = area + SIZE;
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
    int8_t *bitmap = area + 2 * SIZE;
    int8_t *src = area + SIZE;
    int8_t *dst = area;
    int target_offset;
    for (int offset = 0; offset < BITMAP_SIZE; offset += 8) {
        if (*(int64_t *)(bitmap + offset) == 0) {
            continue;
        }
        for (int i = 0; i < 8; i += 4) {
            if (*(int32_t *)(bitmap + offset + i) != 0) {
                for (int j = 0; j < 4; j += 2) {
                    if (*(int16_t *)(bitmap + offset + i + j) != 0) {
                        for (int k = 0; k < 16; k++) {
                            if ((1 << k) & *(int16_t *)(bitmap + offset + i + j)) {
                                target_offset = (((offset + i + j) << 3) + k) << 3;
                                *(int64_t *)(dst + target_offset) = *(int64_t *)(src + target_offset);
                            }
                        }
                    }
                }
            }
        }
    }
    memset(bitmap, 0, BITMAP_SIZE);
}

void restore_area_2(int8_t *area) {
    int8_t *bitmap = area + 2 * SIZE;
    int8_t *src = area + SIZE;
    int8_t *dst = area;
    for (int offset = 0; offset < BITMAP_SIZE; offset += 8) {
        if (*(int64_t *)(bitmap + offset) == 0) {
            continue;
        }
        for (int i = 0; i < 8; i += 2) {
            int16_t current_word = *(int16_t *)(bitmap + offset + i);
            if (current_word == 0) {
                continue;
            }
            for (int k = 0; k < 16; k++) {
                if (((current_word >> k) & 1) == 1) {
                    int target_offset = ((offset + i) * 8 + k) * 8;
                    // printf("Offset 0x%x. Bit 0x%x. Target Offset 0x%x.\n", offset, k, target_offset);
                    *(int64_t *)(dst + target_offset) = *(int64_t *)(src + target_offset);
                }
            }
        }
    }
    memset(bitmap, 0, BITMAP_SIZE);
}

void restore_area_3(int8_t *area) {
    int8_t *bitmap = area + 2 * SIZE;
    int8_t *src = area + SIZE;
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

void restore_area_avx_8(int8_t *area) {
    int8_t *bitmap = area + 2 * SIZE;
    int8_t *src = area + SIZE;
    int8_t *dst = area;

    for (int offset = 0; offset < BITMAP_SIZE; offset += 32) {
        __m256i bitmap_vec = _mm256_loadu_si256((__m256i *)(bitmap + offset));

        if (_mm256_testz_si256(bitmap_vec, bitmap_vec)) {
            continue;
        }

        // Use a loop for the 32 bytes to handle different bit patterns
        for (int i = 0; i < 32; i++) {
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

void restore_area_avx_16(int8_t *area) {
    int8_t *bitmap = area + 2 * SIZE;
    int8_t *src = area + SIZE;
    int8_t *dst = area;

    for (int offset = 0; offset < BITMAP_SIZE; offset += 32) {
        __m256i bitmap_vec = _mm256_loadu_si256((__m256i *)(bitmap + offset));

        if (_mm256_testz_si256(bitmap_vec, bitmap_vec)) {
            continue;
        }

        // Use a loop for the 32 bytes to handle different bit patterns
        for (int i = 0; i < 32; i += 2) {
            int16_t current_word = *(int16_t *)(bitmap + offset + i);
            if (current_word == 0) {
                continue;
            }

            for (int k = 0; k < 16; k++) {
                if ((current_word >> k) & 1) {
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
    for (int i = 0; i < (2 * SIZE + BITMAP_SIZE); i += (cache_line_size / 8)) {
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
        return EXIT_FAILURE;
    }

    srand(42);
    init_value = rand() % (0xFFFFFFFFFFFFFFFF - 1 + 1) + 1;
    first_value = rand() % (0xFFFFFFFFFFFFFFFF - 1 + 1) + 1;
    second_value = rand() % (0xFFFFFFFFFFFFFFFF - 1 + 1) + 1;

    printf("Initial Value 0x%lx\n", init_value);
    printf("First Value 0x%lx\n", first_value);
    printf("Second Value 0x%lx\n\n", second_value);

    size_t alignment = 8 * (1024 * SIZE);
    int8_t *area = (int8_t *)aligned_alloc(alignment, SIZE * 2 + BITMAP_SIZE);
    if (area == NULL) {
        perror("aligned_alloc failed\n");
        return EXIT_FAILURE;
    }

    memset(area, 0, SIZE * 2 + BITMAP_SIZE);

    printf("BaseA: %p\n", area);
    printf("BaseS: %p\n", area + SIZE);
    printf("BaseM: %p\n\n", area + 2 * SIZE);

    init_area(area, init_value);
    int8_t *init_A_copy = (int8_t *)mmap(NULL, SIZE, PROT_READ | PROT_WRITE, MAP_ANONYMOUS | MAP_PRIVATE, 0, 0);
    if (init_A_copy == MAP_FAILED) {
        perror("mmap init area copy");
        return EXIT_FAILURE;
    }

    memcpy(init_A_copy, area, SIZE);

    ret = memcmp(area, init_A_copy, SIZE);
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

    clean_cache(area);
    printf("\nTest Restore Function\n");
    begin = clock();

    #if SIMPLE
        restore_area(area);
    #endif

    #if SIMPLE2
        restore_area_2(area);
    #endif

    #if SIMPLE3
        restore_area_3(area);
    #endif

    #if AVX8
        restore_area_avx_8(area);
    #endif

    #if AVX16
        restore_area_avx_16(area);
    #endif

    end = clock();

    double time_spent = (double)(end - begin) / CLOCKS_PER_SEC;
    printf("Time spent by restore: %f s\n", time_spent);

    ret = memcmp(area, init_A_copy, SIZE);
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