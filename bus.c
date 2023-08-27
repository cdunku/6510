#include "cpu.h"
#include "bus.h"
#include "debug.h"

/* R/W, Stack and I/O functions */

#define LORAM 0x1 // (BIT 0, WEIGHT 1)
#define HIRAM 0x2 // (BIT 1, WEIGHT 2)
#define CHAREN 0x4 // (BIT 2, WEIGHT 4)

/*
 * Memory configuration and organisation:
 *
 * http://unusedino.de/ec64/technical/aay/c64/memcfg.htm
 * http://sta.c64.org/cbm64mem.html
 *
 */

uint8_t 
rb(MOS_6510* const c, uint16_t addr)
{
  return c->ram[addr & 0xFFFF];
}

uint16_t
rw(MOS_6510* const c, uint16_t addr)
{
  return rb(c, addr + 1) << 8 | rb(c, addr);
}

void 
wb(MOS_6510* const c, uint16_t addr, uint8_t value)
{
  c->ram[addr & 0xFFFF] = value;
}

void
ww(MOS_6510* const c, uint16_t addr, uint16_t value)
{
  wb(c, addr, value & 0xFF);
  wb(c, addr + 1, (value & 0xFF) >> 8);
}

uint8_t 
fetch_byte(MOS_6510* const c)
{
	uint8_t byte = rb(c, c->pc++);
	return byte;
}

uint16_t
fetch_word(MOS_6510* const c)
{
  uint16_t word = rw(c, c->pc);
  c->pc += 2;
  return word;
}

uint8_t 
pop_byte(MOS_6510* const c)
{
  c->sp++;
  uint16_t stack_addr = 0x100 + c->sp;
  return rb(c, stack_addr);
}

uint16_t 
pop_word(MOS_6510* const c)
{
  return (pop_byte(c)) | (pop_byte(c) << 8);
}

void 
push_byte(MOS_6510* const c, uint8_t byte)
{
  uint16_t stack_addr = 0x100 + c->sp--;
  wb(c, stack_addr, byte);
}

void 
push_word(MOS_6510* const c, uint16_t word)
{
  uint16_t addr = 0x100 + c->sp;
  wb(c, addr, word >> 8);
  wb(c, addr - 1, word & 0xFF);
  c->sp -= 2;
}
