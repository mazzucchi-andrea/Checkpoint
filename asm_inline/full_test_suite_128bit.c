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

#ifndef SIZE
#define SIZE 0x200000UL
#endif

#define BITARRAY_SIZE SIZE / 8

#define STR1(x) #x
#define STR(x) STR1(x)

extern int arch_prctl(int code, unsigned long addr);

void checkpoint_dqword(int8_t *int_ptr, int offset);

void *tls_setup() {
    unsigned long addr;
    addr = (unsigned long)mmap(NULL, 1024, PROT_READ | PROT_WRITE, MAP_ANONYMOUS | MAP_PRIVATE, 0, 0);
    *(unsigned long *)addr = addr;
    if (arch_prctl(ARCH_SET_GS, addr)) {
        return NULL;
    }
    return (void *)addr;
}

inline void checkpoint_dqword(int8_t *int_ptr, int offset) {
    int_ptr += offset;

    __asm__ __inline__(
        "mov %%rax, %%gs:0;"
        "mov %%rbx, %%gs:8;"
        "mov %%rcx, %%gs:16;"
        "movdqu %%xmm1, %%gs:24;"
        "mov %%rax, %%rcx;"
        "and $0xffffffffffc00000,%%rcx;"
        "and $0x3fffff, %%rax;"
        "test $15, %%rax;"
        "jz second_dqword;"
        "and $0x7ffff0, %%rax;"
        "shr $4, %%rax;"
        "mov %%rax, %%rbx;"
        "and $15, %%rbx;"
        "shr $4, %%rax;"
        "bts %%bx, " STR(2 * SIZE) "(%%rcx, %%rax, 2);"
        "jc next_qword;"
        "shl $4, %%rax;"
        "add %%rbx, %%rax;"
        "shl $4, %%rax;"
        "movdqu (%%rcx, %%rax,), %%xmm1;"
        "movdqu %%xmm1, " STR(SIZE) "(%%rcx, %%rax,);"
        "jmp check_last;"
        "next_qword:"
        "shl $4, %%rax;"
        "add %%rbx, %%rax;"
        "check_last:"
        "shl $4, %%rax;"
        "add $16, %%rax;"
        "cmp $" STR(SIZE) ", %%rax;"
        "jge end;"
        "second_dqword:"
        "shr $4, %%rax;"
        "mov %%rax, %%rbx;"
        "and $15, %%rbx;"
        "shr $4, %%rax;"
        "bts %%bx, " STR(2 * SIZE) "(%%rcx, %%rax, 2);"
        "jc end;"
        "shl $4, %%rax;"
        "add %%rbx, %%rax;"
        "shl $4, %%rax;"
        "movdqu (%%rcx, %%rax,), %%xmm1;"
        "movdqu %%xmm1, " STR(SIZE) "(%%rcx, %%rax,);"
        "end:"
        "mov %%gs:0, %%rax;"
        "mov %%gs:8, %%rbx;"
        "mov %%gs:16, %%rcx;"
        "movdqu %%gs:24, %%xmm1;"
        :
        : "a"(int_ptr)
        : "rbx", "rcx", "memory");
}

/* Initialize the area with the given quadword */
void init_area(int8_t *area, int64_t init_value) {
    for (int i = 0; i < (SIZE - 8); i += 8) {
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
    for (int i = 0; i < numberOfWrites; i += 4) {
        offset = i % (SIZE - 8 + 1);
        checkpoint_dqword(area, offset);
        *(int64_t *)(area + offset) = new_value;
    }
    for (int i = 0; i < numberOfReads; i++) {
        offset = i % (SIZE - 8 + 1);
        read_value = *(int64_t *)(area + offset);
    }
    end = clock();
    time_spent = (double)(end - begin) / CLOCKS_PER_SEC;
    printf("Time spent by %d writes and %d reads: %f s\n", numberOfWrites, numberOfReads, time_spent);
}

void test_no_checkpoint(int8_t *area, int64_t new_value, int numberOfWrites, int numberOfReads) {
    int offset;
    int64_t read_value;
    clock_t begin, end;
    double time_spent;
    begin = clock();
    for (int i = 0; i < numberOfWrites; i += 4) {
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
    for (int offset = 0; offset < BITARRAY_SIZE; offset += 2) {
        int16_t current_word = bitmap[offset];
        if (current_word == 0) {
            continue;
        }
        for (int k = 0; k < 16; k++) {
            if ((current_word >> k) & 1) {
                int target_offset = (offset * 8 + k) * 16;
                if (*(__int128 *)(init_A_copy + target_offset) != *(__int128 *)(areaS + target_offset)) {
                    fprintf(stderr,
                            "Checkpoint verify failed:\n"
                            "BitArray Word Offset: %d\n"
                            "Bit: %d\n"
                            "Target Offset: %d\n"
                            "Area S Value: First qword: 0x%lx Second qword: 0x%lx\n"
                            "Area A init Value First qword: 0x%lx Second qword: 0x%lx\n",
                            offset,
                            k,
                            target_offset, 
                            *(int64_t *)(areaS + target_offset),
                            *(int64_t *)(areaS + target_offset + 8),
                            *(int64_t *)(init_A_copy + target_offset),
                            *(int64_t *)(init_A_copy + target_offset + 8));
                    return -1;
                }
            }
        }
    }
    return 0;
}

void restore_area(int8_t *area) {
    int8_t *bitarray = area + 2 * SIZE;
    int8_t *src = area + SIZE;
    int8_t *dst = area;
    int16_t current_word;
    int target_offset;
    clock_t begin, end;
    double time_spent;
    begin = clock();

    for (int offset = 0; offset < BITARRAY_SIZE; offset += 8) {
        if (*(int64_t *)(bitarray + offset) == 0) {
            continue;
        }
        for (int j = 0; j < 8; j += 2) {
            current_word = *(int16_t *)(bitarray + offset + j);
            if (current_word == 0) {
                continue;
            }
            for (int k = 0; k < 16; k++) {
                    if ((current_word >> k) & 1) {
                        target_offset = ((offset + j) * 8 + k) * 16;
                        *(__int128 *)(dst + target_offset) = *(__int128 *)(src + target_offset);
                    }
                }
        }
    }
    memset(bitarray, 0, BITARRAY_SIZE);

    end = clock();
    time_spent = (double)(end - begin) / CLOCKS_PER_SEC;
    printf("Time spent by restore: %f s\n", time_spent);
}

void clean_cache(int8_t *area) {
    int cache_line_size = __builtin_cpu_supports("sse2") ? 64 : 32;
    for (int i = 0; i < (2 * SIZE + BITARRAY_SIZE); i += (cache_line_size / 8)) {
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
    int8_t *area = (int8_t *)aligned_alloc(alignment, SIZE * 2 + BITARRAY_SIZE);
    if (area == NULL) {
        perror("aligned_alloc failed\n");
        return EXIT_FAILURE;
    }

    memset(area, 0, SIZE * 2 + BITARRAY_SIZE);

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

    restore_area(area);

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