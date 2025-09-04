#include <asm/prctl.h>

#include <linux/limits.h>

#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <sys/mman.h>
#include <sys/types.h>

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
  __asm__ __inline__("mov %%rax, %%gs:0;"
                     "mov %%rbx, %%gs:8;"
                     "mov %%rcx, %%gs:16;"
                     "mov %%rax, %%rcx;"
                     "and $0xffffffffffe00000,%%rcx;"
                     "and $0x00000000001fffff, %%rax;"
                     "test $7, %%rax;"
                     "jz second_qword;"
                     "and $0xfffffffffffffff8, %%rax;"
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

/* Save original values and set the bitmap bit before writing the new value */
void test_checkpoint(int8_t *area, int64_t new_value, int numberOfOps) {
  int offset = 0;
  for (int i = 0; i < numberOfOps; i++) {
    checkpoint_qword(area + offset);
    *(int64_t *)(area + offset) = new_value;
    offset = (offset + 1) % (0x200000 - 8);
  }
}

/* Write the new values */
void test_only_write(int8_t *area, int64_t new_value, int numberOfOps) {
  int offset = 0;
  for (int i = 0; i < numberOfOps; i++) {
    *(int64_t *)(area + offset) = new_value;
    offset = (offset + 1) % (0x200000 - 8);
  }
}

/* One parameter is needed to run the tests:
 * - numberOfOps: the number of operations to perform;
 */
int main(int argc, char *argv[]) {
  char *endptr;
  int numberOfOps;
  int64_t new_value = 42;

  if (argc < 2) {
    fprintf(stderr, "Usage: %s <numberOfOps>\n", argv[0]);
    return EXIT_FAILURE;
  }

  // Convert and validate numberOfOps
  numberOfOps = strtol(argv[1], &endptr, 10);
  if (*endptr != '\0' || (numberOfOps <= 0)) {
    fprintf(stderr, "Number of writes must be a positive integer\n");
    return EXIT_FAILURE;
  }

  if (tls_setup() == NULL) {
    return EXIT_FAILURE;
  }

  unsigned long size = (1 << 21);
  size_t alignment = 8 * (1024 * size);

  int8_t *area = (int8_t *)aligned_alloc(alignment, size * 2 + 0x40000);
  if (area == NULL) {
    fprintf(stderr, "Cannot allocate aligned memory");
    return EXIT_FAILURE;
  }

  if (CHECKPOINT) {
    test_checkpoint(area, new_value, numberOfOps);
  } else {
    test_only_write(area, new_value, numberOfOps);
  }

  return EXIT_SUCCESS;
}