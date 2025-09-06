#include <asm/prctl.h>

#include <linux/limits.h>

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <sys/mman.h>
#include <sys/types.h>

#define DEBUG if (0)

extern int arch_prctl(int code, unsigned long addr);

void checkpoint_qword(int8_t *int_ptr, int offset);

void *tls_setup() {
    unsigned long addr;

    addr = (unsigned long)mmap(NULL, 64, PROT_READ | PROT_WRITE,
                               MAP_ANONYMOUS | MAP_PRIVATE, 0, 0);

    *(unsigned long *)addr = addr;

    arch_prctl(ARCH_SET_GS, addr);

    return (void *)addr;
}

inline void checkpoint_qword(int8_t *int_ptr, int offset) {
    int_ptr += offset;

    __asm__ __inline__("mov %%rax, %%gs:0;"
                       "mov %%rbx, %%gs:8;"
                       "mov %%rcx, %%gs:16;"
                       "mov %%rax, %%rcx;"
                       "and $0xffffffffffe00000,%%rcx;"
                       "and $0x1fffff, %%rax;"
                       "test $7, %%rax;"
                       "jz second_qword;"
                       "and $0x3ffff8, %%rax;"
                       "shr $3, %%rax;"
                       "mov %%rax, %%rbx;"
                       "and $15, %%rbx;"
                       "shr $4, %%rax;"
                       "bts %%bx, 0x400000(%%rcx, %%rax, 2);"
                       "jc next_qword;"
                       "shl $4, %%rax;"
                       "add %%rbx, %%rax;"
                       "mov (%%rcx, %%rax, 8), %%rbx;"
                       "mov %%rbx, 0x200000(%%rcx, %%rax, 8);"
                       "jmp check_last;"
                       "next_qword:"
                       "shl $4, %%rax;"
                       "add %%rbx, %%rax;"
                       "check_last:"
                       "shl $3, %%rax;"
                       "add $8, %%rax;"
                       "cmp $0x200000, %%rax;"
                       "jge end;"
                       "second_qword:"
                       "shr $3, %%rax;"
                       "mov %%rax, %%rbx;"
                       "and $15, %%rbx;"
                       "shr $4, %%rax;"
                       "bts %%bx, 0x400000(%%rcx, %%rax, 2);"
                       "jc end;"
                       "shl $4, %%rax;"
                       "add %%rbx, %%rax;"
                       "mov (%%rcx, %%rax, 8), %%rbx;"
                       "mov %%rbx, 0x200000(%%rcx, %%rax, 8);"
                       "end:"
                       "mov %%gs:0, %%rax;"
                       "mov %%gs:8, %%rbx;"
                       "mov %%gs:16, %%rcx;"
                       :
                       : "a"(int_ptr)
                       : "rbx", "rcx", "memory");
}

/* Initialize the area at the generated offset with the given value */
void init_area(int seed, int8_t *area, int64_t init_value, int numberOfWrites) {
    int offset;
    srand(seed);
    DEBUG printf("Init Memory with 0x%lx\n", init_value);
    for (int i = 0; i < numberOfWrites; i++) {
        offset = rand() % (0x200000 - 8 + 1);
        DEBUG printf("Writing 0x%lx at 0x%x\n", *(int64_t *)(area + offset),
                     offset);
        *(int64_t *)(area + offset) = init_value;
    }
}

void audit_values(int8_t *area, int offset) {
    printf("Value written at 0x%x in A: 0x%lx\n", offset,
           *(int64_t *)(area + offset));
    printf("Value written at 0x%x in S: 0x%lx\n", offset,
           *(int64_t *)(area + 0x200000 + offset));
}

/* Save original values and set the bitmap bit before writing the new value */
void test_checkpoint(int seed, int8_t *area, int64_t new_value,
                     int numberOfWrites) {
    int offset;
    srand(seed);
    DEBUG printf("Set new value (0x%lx) and start checkpoint operations\n",
                 new_value);
    for (int i = 0; i < numberOfWrites; i++) {
        offset = rand() % (0x200000 - 8 + 1);
        DEBUG printf("Writing 0x%lx at 0x%x\n", new_value, offset);
        checkpoint_qword(area, offset);
        *(int64_t *)(area + offset) = new_value;
        DEBUG audit_values(area, offset);
    }
}

/* Check if the bits are set */
int check_bitmap(int seed, int16_t *area, int numberOfWrites) {
    int offset, word_offset, bit_index;
    bool aligned;
    srand(seed);
    for (int i = 0; i < numberOfWrites; i++) {
        offset = rand() % (0x200000 - 8 + 1);
        aligned = (offset % 8) == 0;
        offset -= offset % 8;
    not_aligned:
        word_offset = offset >> 7;
        bit_index = (offset >> 3) & 15;
        if (!((1 << bit_index) & *(area + word_offset))) {
            DEBUG fprintf(stderr,
                          "Bit for offset 0x%x not set. (word offset 0x%x, bit "
                          "index 0x%x)\n",
                          offset, word_offset, bit_index);
            return -1;
        }
        if (!aligned) {
            offset += 8;
            aligned = true;
            goto not_aligned;
        }
    }
    return 0;
}

/* Two parameters are needed to run the tests:
 * - seed: the initial value for random number generation;
 * - numberOfWrites: the number of write operations to perform;
 */
int main(int argc, char *argv[]) {
    char *endptr;
    int seed, numberOfWrites, ret;
    int64_t init_value, first_value, second_value;

    if (argc < 3) {
        fprintf(stderr, "Usage: %s <seed> <numberOfWrites>\n", argv[0]);
        return EXIT_FAILURE;
    }

    // Convert and validate seed
    seed = strtol(argv[1], &endptr, 10);
    if (*endptr != '\0' || seed <= 0) {
        fprintf(stderr, "Seed must be a positive integer.\n");
        return EXIT_FAILURE;
    }

    // Convert and validate numberOfWrites
    numberOfWrites = strtol(argv[2], &endptr, 10);
    if (*endptr != '\0' || (numberOfWrites <= 0 && numberOfWrites <= 512)) {
        fprintf(stderr,
                "Number of writes must be an integer between 1 and 512.\n");
        return EXIT_FAILURE;
    }

    printf("Seed: 0x%x\n", seed);
    printf("Number of Writes: 0x%x\n", numberOfWrites);

    if (tls_setup() == NULL) {
        return EXIT_FAILURE;
    }

    srand(seed);

    init_value = rand() % (0xFFFFFFFFFFFFFFFF - 1 + 1) + 1;
    first_value = rand() % (0xFFFFFFFFFFFFFFFF - 1 + 1) + 1;
    second_value = rand() % (0xFFFFFFFFFFFFFFFF - 1 + 1) + 1;

    DEBUG printf("Initial Value 0x%lx\n", init_value);
    DEBUG printf("First Value 0x%lx\n", first_value);
    DEBUG printf("Second Value 0x%lx\n", second_value);

    unsigned long size = (1 << 21);
    size_t alignment = 8 * (1024 * size);

    int8_t *area = (int8_t *)aligned_alloc(alignment, size * 2 + 0x40000);
    if (area == NULL) {
        fprintf(stderr, "Cannot allocate aligned memory");
        return EXIT_FAILURE;
    }
    DEBUG {
        printf("BaseA: %p\n", area);
        printf("BaseS: %p\n", area + 0x200000);
        printf("BaseM: %p\n", area + 0x400000);
    }

    int8_t *init_A_copy = mmap(NULL, (1 << 21), PROT_READ | PROT_WRITE,
                               MAP_ANONYMOUS | MAP_PRIVATE, 0, 0);

    init_area(seed, area, init_value, numberOfWrites);

    memcpy(init_A_copy, area, 0x200000);

    ret = memcmp(area, init_A_copy, 0x200000);
    if (ret) {
        fprintf(stderr, "Area A check failed: 0x%x\n", ret);
        return EXIT_FAILURE;
    }

    test_checkpoint(seed, area, first_value, numberOfWrites);

    ret = memcmp(area + 0x200000, init_A_copy, 0x200000);
    if (ret) {
        fprintf(stderr, "Area S check failed: 0x%x\n", ret);
        return EXIT_FAILURE;
    }

    if (check_bitmap(seed, (int16_t *)(area + 0x400000), numberOfWrites)) {
        fprintf(stderr, "Bitmap check failed\n");
        return EXIT_FAILURE;
    }

    test_checkpoint(seed, area, second_value, numberOfWrites);

    ret = memcmp(area + 0x200000, init_A_copy, 0x200000);
    if (ret) {
        fprintf(stderr, "Area S check failed: 0x%x\n", ret);
        return EXIT_FAILURE;
    }

    if (check_bitmap(seed, (int16_t *)(area + 0x400000), numberOfWrites)) {
        fprintf(stderr, "Bitmap check failed\n");
        return EXIT_FAILURE;
    }

    printf("Test Passed\n");
    return EXIT_SUCCESS;
}