#include <stdint.h>
#include <string.h>
// #include <emmintrin.h>

#include "ckpt.h"

// CKPT ASM CODE
#define LOG2_32 5
#define LOG2_64 6
#define LOG2_128 7
#define LOG2_256 8
#define LOG2_512 9
#define LOG2_1024 10
#define LOG2_2048 11
#define LOG2_4096 12

// Helper macro to concatenate and evaluate
#define LOG2_EVAL(x) LOG2_##x
#define LOG2(x) LOG2_EVAL(x)

void set_ckpt(uint8_t *area) {
    memset(area + ALLOCATOR_AREA_SIZE * 2, 0, BITMAP_SIZE);
}

void ckpt_chunk(uint8_t *ptr) {
    int bitmap_offset, chunk, index;
    uint8_t bitmask, bit_index;

    uint8_t *base =
        (uint8_t *)((uintptr_t)ptr & (uintptr_t)(~(ALLOCATOR_AREA_SIZE - 1UL)));
    uint64_t chunk_offset = (uint64_t)ptr & (ALLOCATOR_AREA_SIZE - CHUNK);
    chunk = chunk_offset >> LOG2(CHUNK);
    bit_index = chunk & 7;
    bitmap_offset = chunk >> 3;
    bitmask = 1 << bit_index;
    *(uint8_t *)(base + 2 * ALLOCATOR_AREA_SIZE + bitmap_offset) |= bitmask;
}

void restore_chunks(uint8_t *area) {
    uint8_t current_byte;

    for (int j = 0; j < BITMAP_SIZE; j++) {
        current_byte = *(uint8_t *)(area + 2 * ALLOCATOR_AREA_SIZE + j);
        if (current_byte == 0) {
            continue;
        }
        for (int k = 0; k < 8; k++) {
            if (((current_byte >> k) & 1) == 1) {
                memcpy(
                    (void *)(area + (j * 8 + k) * CHUNK),
                    (void *)(area + (j * 8 + k) * CHUNK + ALLOCATOR_AREA_SIZE),
                    CHUNK);
            }
        }
    }
    memset(area + ALLOCATOR_AREA_SIZE * 2, 0, BITMAP_SIZE);
}