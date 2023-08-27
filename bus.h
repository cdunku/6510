#ifndef _6510_BUS
#define _6510_BUS

#include <stdint.h>

#include "cpu.h"

uint8_t rb(MOS_6510* const c, uint16_t addr);
uint16_t rw(MOS_6510* const c, uint16_t addr);

void wb(MOS_6510* const c, uint16_t addr, uint8_t value);

uint8_t fetch_byte(MOS_6510* const c);
uint16_t fetch_word(MOS_6510* const c);

uint8_t pop_byte(MOS_6510* const c);
uint16_t pop_word(MOS_6510* const c);

void push_byte(MOS_6510* const c, uint8_t byte);
void push_word(MOS_6510* const c, uint16_t word);

#endif // _6510_BUS
