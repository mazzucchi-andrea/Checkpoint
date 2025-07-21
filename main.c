#include <asm/prctl.h>

#include <linux/limits.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <sys/mman.h>
#include <sys/types.h>

#define DEBUG if (0)

extern int arch_prctl(int code, unsigned long addr);

void *tls_setup() {
  unsigned long addr;

  addr = (unsigned long)mmap(NULL, 64, PROT_READ | PROT_WRITE,
                             MAP_ANONYMOUS | MAP_PRIVATE, 0, 0);

  *(unsigned long *)addr = addr;

  arch_prctl(ARCH_SET_GS, addr);

  return (void *)addr;
}

void checkpoint_dword(int8_t *int_ptr, int64_t offset) {
  int_ptr += offset;

  __asm__ __inline__("mov %%rax, %%gs:0;"
                     "mov %0, %%rax;"
                     "mov %%rbx, %%gs:8;"
                     "mov %%rcx, %%gs:16;"
                     "mov %%rax, %%rcx;"
                     "and $-4096, %%rcx;"
                     "and $0xFFF, %%rax;"
                     "test $7, %%rax;"
                     "jz second_qword;"
                     "first_qword:"
                     "and $-8, %%rax;"
                     "shr $3, %%rax;"
                     "mov %%rax, %%rbx;"
                     "and $15, %%rbx;"
                     "shr $4, %%rax;"
                     "bts %%bx, 8192(%%rcx, %%rax, 2);"
                     "jc next_qword;"
                     "shl $4, %%rax;"
                     "add %%rbx, %%rax;"
                     "mov (%%rcx, %%rax, 8), %%rbx;"
                     "mov %%rbx, 4096(%%rcx, %%rax, 8);"
                     "next_qword:"
                     "mov %%gs:0, %%rax;"
                     "and $0xFF8, %%rax;"
                     "add $8, %%rax;"
                     "cmp $4096, %%rax;"
                     "jge end;"
                     "second_qword:"
                     "shr $3, %%rax;"
                     "mov %%rax, %%rbx;"
                     "and $15, %%rbx;"
                     "shr $4, %%rax;"
                     "bts %%bx, 8192(%%rcx, %%rax, 2);"
                     "jc end;"
                     "shl $4, %%rax;"
                     "add %%rbx, %%rax;"
                     "mov (%%rcx, %%rax, 8), %%rbx;"
                     "mov %%rbx, 4096(%%rcx, %%rax, 8);"
                     "end:"
                     "mov %%gs:0, %%rax;"
                     "mov %%gs:8, %%rbx;"
                     "mov %%gs:16, %%rcx;"
                     :
                     : "r"(int_ptr)
                     : "rax", "rbx", "rcx", "memory");
}

/* Initialize the area at the generated offset with the given value */
void init_area(int seed, int8_t *area, int64_t init_value, int numberOfWrites) {
  int offset;
  srand(seed);
  DEBUG printf("Init Memory with %ld\n", init_value);
  DEBUG printf("Writing at ");
  for (int i = 0; i < numberOfWrites; i++) {
    offset = rand() % (4091 - 0 + 1) + 0;
    DEBUG printf("%d, ", offset);
    *(area + offset) = init_value;
  }
  DEBUG printf("\n");
}

/* Save original values and set the bitmap bit before writing the new value */
void test_checkpoint(int seed, int8_t *area, int64_t new_value,
                     int numberOfWrites) {
  int offset;
  srand(seed);
  DEBUG printf("Set new value (%ld) and start checkpoint operations\n",
               new_value);
  DEBUG printf("Writing at ");
  for (int i = 0; i < numberOfWrites; i++) {
    offset = rand() % (4091 - 0 + 1) + 0;
    DEBUG printf("%d, ", offset);
    checkpoint_dword(area, offset);
    *(area + offset) = new_value;
  }
  DEBUG printf("\n");
}

/* Check if the area at the generated offset contains the given value */
int check_area(int seed, int8_t *area, int8_t value, int numberOfWrites) {
  int offset;
  srand(seed);
  for (int i = 0; i < numberOfWrites; i++) {
    offset = rand() % (4091 - 0 + 1) + 0;
    if (*(area + offset) != value) {
      return -1;
    }
  }
  return 0;
}

/* Check if the bits are set */
int check_bitmap(int seed, int16_t *area, int numberOfWrites) {
  int offset, word_offset, bit_index;
  bool aligned;
  srand(seed);
  for (int i = 0; i < numberOfWrites; i++) {
    offset = rand() % (4091 - 0 + 1) + 0;
    DEBUG printf("offset: %d\n", offset);
    aligned = (offset % 8) == 0;
    offset -= offset % 8;
  not_aligned:
    word_offset = offset >> 7;
    DEBUG printf("word_offset: %d\n", word_offset);
    bit_index = (offset >> 3) & 15;
    DEBUG printf("bit_index: %d\n", bit_index);
    if (!((1 << bit_index) & *(area + word_offset))) {
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

/* Three parameters are needed to run the tests:
 * - seed: the initial value for random number generation;
 * - numberOfWrites: the number of write operations to perform;
 */
int main(int argc, char *argv[]) {
  char *endptr;
  int seed, numberOfWrites;
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
  if (*endptr != '\0' || numberOfWrites <= 0) {
    fprintf(stderr,
            "Number of writes must be an integer between 1 and 2048.\n");
    return EXIT_FAILURE;
  }

  printf("Seed: %d\n", seed);
  printf("Number of Writes: %d\n", numberOfWrites);

  if (tls_setup() == NULL) {
    return EXIT_FAILURE;
  }

  srand(seed);

  init_value = rand() % (0xFFFFFFFFFFFFFFFF - 1 + 1) + 1;
  DEBUG printf("Initial Value %ld\n", init_value);
  first_value = rand() % (0xFFFFFFFFFFFFFFFF - 1 + 1) + 1;
  DEBUG printf("First Value %ld\n", first_value);
  second_value = rand() % (0xFFFFFFFFFFFFFFFF - 1 + 1) + 1;
  DEBUG printf("Second Value %ld\n", first_value);

  int8_t *area = mmap(NULL, 8256, PROT_READ | PROT_WRITE,
                      MAP_ANONYMOUS | MAP_PRIVATE, 0, 0);
  DEBUG {
    printf("BaseA: %p\n", area);
    printf("BaseS: %p\n", area + 4096);
    printf("BaseM: %p\n", area + 8192);
  }

  init_area(seed, area, init_value, numberOfWrites);

  if (check_area(seed, area, init_value, numberOfWrites)) {
    return EXIT_FAILURE;
  }

  test_checkpoint(seed, area, first_value, numberOfWrites);

  if (check_area(seed, area, first_value, numberOfWrites)) {
    return EXIT_FAILURE;
  }

  if (check_area(seed, area + 4096, init_value, numberOfWrites)) {
    return EXIT_FAILURE;
  }

  if (check_bitmap(seed, (int16_t *)(area + 8192), numberOfWrites)) {
    return EXIT_FAILURE;
  }

  test_checkpoint(seed, area, second_value, numberOfWrites);

  if (check_area(seed, area, second_value, numberOfWrites)) {
    return EXIT_FAILURE;
  }

  if (check_area(seed, area + 4096, init_value, numberOfWrites)) {
    return EXIT_FAILURE;
  }

  if (check_bitmap(seed, (int16_t *)(area + 8192), numberOfWrites)) {
    return EXIT_FAILURE;
  }

  return EXIT_SUCCESS;
}