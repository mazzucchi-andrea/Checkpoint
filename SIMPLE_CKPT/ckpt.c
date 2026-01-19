#include <immintrin.h> // AVX
#include <string.h>

#include "ckpt.h"

void set_ckpt(uint8_t *area) { memcpy(area + SIZE, area, SIZE); }

void restore_area(uint8_t *area) { memcpy(area, area + SIZE, SIZE); }