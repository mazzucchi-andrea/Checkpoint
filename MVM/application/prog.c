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

extern int arch_prctl(int code, unsigned long addr);

void *tls_setup() {
    unsigned long addr;
    addr = (unsigned long)mmap(NULL, 64, PROT_READ | PROT_WRITE,
                               MAP_ANONYMOUS | MAP_PRIVATE, 0, 0);
    *(unsigned long *)addr = addr;
    arch_prctl(ARCH_SET_GS, addr);
    return (void *)addr;
}

/* Initialize the area at the generated offset with the given value */
void init_area(int seed, int8_t *area, int64_t init_value, int numberOfWrites) {
    int offset;
    srand(seed);
    printf("Init Memory with 0x%lx\n", init_value);
    for (int i = 0; i < numberOfWrites; i++) {
        offset = rand() % (0x200000 - 8 + 1);
        *(int64_t *)(area + offset) = init_value;
    }
}

/* Save original values and set the bitmap bit before writing the new value */
void test_checkpoint(int seed, int8_t *area, int64_t new_value,
                     int numberOfWrites) {
    int offset;
    srand(seed);
    printf("Set new value (0x%lx)\n", new_value);
    for (int i = 0; i < numberOfWrites; i++) {
        offset = rand() % (0x200000 - 8 + 1);
        *(int64_t *)(area + offset) = new_value;
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
            fprintf(
                stderr,
                "Bit for offset %d not set. (word offset %d, bit index %d)\n",
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

    printf("Seed: %d\n", seed);
    printf("Number of Writes: %d\n", numberOfWrites);

    if (tls_setup() == NULL) {
        return EXIT_FAILURE;
    }

    srand(seed);

    init_value = rand() % (0xFFFFFFFFFFFFFFFF - 1 + 1) + 1;
    first_value = rand() % (0xFFFFFFFFFFFFFFFF - 1 + 1) + 1;
    second_value = rand() % (0xFFFFFFFFFFFFFFFF - 1 + 1) + 1;

    printf("Initial Value 0x%lx\n", init_value);
    printf("First Value 0x%lx\n", first_value);
    printf("Second Value 0x%lx\n", second_value);

    unsigned long size = (1 << 21);
    size_t alignment = 8 * (1024 * size);

    int8_t *area = (int8_t *)aligned_alloc(alignment, size * 2 + 0x40000);
    memset(area, 0, size * 2 + 0x40000);

    if (area == NULL) {

        fprintf(stderr, "Cannot allocate aligned memory");
        return EXIT_FAILURE;
    };

    printf("BaseA: %p\n", area);
    printf("BaseS: %p\n", area + 0x200000);
    printf("BaseM: %p\n", area + 0x400000);

    init_area(seed, area, init_value, numberOfWrites);

    int8_t *init_A_copy = mmap(NULL, 0x200000, PROT_READ | PROT_WRITE,
                               MAP_ANONYMOUS | MAP_PRIVATE, 0, 0);

    memcpy(init_A_copy, area, 0x200000);

    ret = memcmp(area, init_A_copy, 0x200000);

    if (ret) {
        fprintf(stderr, "Area A check failed: %d\n", ret);
        return EXIT_FAILURE;
    }

    test_checkpoint(seed, area, first_value, numberOfWrites);

    ret = memcmp(area + 0x200000, init_A_copy, 0x200000);
    if (ret) {
        fprintf(stderr, "Area S first value check failed: %d\n", ret);
        return EXIT_FAILURE;
    }

    if (check_bitmap(seed, (int16_t *)(area + 0x400000), numberOfWrites)) {
        fprintf(stderr, "Bitmap first value check failed\n");
        return EXIT_FAILURE;
    }

    test_checkpoint(seed, area, second_value, numberOfWrites);

    ret = memcmp(area + 0x200000, init_A_copy, 0x200000);
    if (ret) {
        fprintf(stderr, "Area S second value check failed: %d\n", ret);
        return EXIT_FAILURE;
    }

    if (check_bitmap(seed, (int16_t *)(area + 0x400000), numberOfWrites)) {
        fprintf(stderr, "Bitmap first value check failed\n");
        return EXIT_FAILURE;
    }

    printf("Test Passed\n");
    return EXIT_SUCCESS;
}