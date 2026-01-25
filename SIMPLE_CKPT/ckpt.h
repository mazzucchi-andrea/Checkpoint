#ifndef _CKPT_
#define _CKPT_

#include <stdint.h>

#ifndef SIZE
#define SIZE 0x100000
#endif

void set_ckpt(uint8_t*);

void restore_area(uint8_t*);

#endif