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

void checkpoint_qword(int8_t *int_ptr);

void *tls_setup() {
  unsigned long addr;

  addr = (unsigned long)mmap(NULL, 64, PROT_READ | PROT_WRITE,
                             MAP_ANONYMOUS | MAP_PRIVATE, 0, 0);

  *(unsigned long *)addr = addr;

  arch_prctl(ARCH_SET_GS, addr);

  return (void *)addr;
}

inline void checkpoint_qword(int8_t *int_ptr) {
  __asm__ __inline__("mov %0, %%rax;"
                     "mov %%rax, %%gs:0;"
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

/* Save original values and set the bitmap bit before writing the new value */
void test_checkpoint(int8_t *area, int64_t new_value, int numberOfOps,
                     double writePercentage) {
  int offset = 0;
  int64_t value_read;
  for (int i = 0; i < numberOfOps; i++) {
    if (((double)rand() / RAND_MAX) < writePercentage) {
      checkpoint_qword(area + offset);
      *(int64_t *)(area + offset) = new_value;
    } else {
      value_read = *(int64_t *)(area + offset);
      (void) value_read;
    }
    offset = (offset + 1) % (4096 - 8);
  }
}

/* Write the new values */
void test_only_write(int8_t *area, int64_t new_value, int numberOfOps,
                     double writePercentage) {
  int offset = 0;
  int64_t value_read;
  for (int i = 0; i < numberOfOps; i++) {
    if (((double)rand() / RAND_MAX) < writePercentage) {
      *(int64_t *)(area + offset) = new_value;
    } else {
      value_read = *(int64_t *)(area + offset);
      (void) value_read;
    }
    offset = (offset + 1) % (4096 - 8);
  }
}

/* Three parameters are needed to run the tests:
 * - seed:
 * - numberOfOps: the number of operations to perform;
 * - writePercentage:
 */
int main(int argc, char *argv[]) {
  char *endptr;
  int seed, numberOfOps;
  double writePercentage;
  int64_t new_value = 42;

  if (argc < 4) {
    fprintf(stderr, "Usage: %s <seed> <numberOfOps> <writePercentage>\n",
            argv[0]);
    return EXIT_FAILURE;
  }

  // Convert and validate seed
  seed = strtol(argv[1], &endptr, 10);
  if (*endptr != '\0' || seed <= 0) {
    fprintf(stderr, "Seed must be a positive integer.\n");
    return EXIT_FAILURE;
  }

  // Convert and validate numberOfOps
  numberOfOps = strtol(argv[2], &endptr, 10);
  if (*endptr != '\0' || (numberOfOps <= 0)) {
    fprintf(stderr, "Number of writes must be a positive integer\n");
    return EXIT_FAILURE;
  }
  DEBUG printf("Number of Operations: %d\n", numberOfOps);

  // Convert and validate writePercentage
  writePercentage = atof(argv[3]);
  if (*endptr != '\0' || numberOfOps < 0 || (1 - numberOfOps) > 0.0001) {
    fprintf(stderr, "Write Percentage must be a double between 0 and 1.0, actual %f.\n", writePercentage);
    return EXIT_FAILURE;
  }
  DEBUG printf("Percentage of Writes: %f\n", writePercentage);

  if (tls_setup() == NULL) {
    return EXIT_FAILURE;
  }

  int8_t *area = mmap(NULL, 8256, PROT_READ | PROT_WRITE,
                      MAP_ANONYMOUS | MAP_PRIVATE, 0, 0);
  DEBUG {
    printf("BaseA: %p\n", area);
    printf("BaseS: %p\n", area + 4096);
    printf("BaseM: %p\n", area + 8192);
  }

  srand(42);
  if (CHECKPOINT) {
    test_checkpoint(area, new_value, numberOfOps, writePercentage);
  } else {
    test_only_write(area, new_value, numberOfOps, writePercentage);
  }

  return EXIT_SUCCESS;
}