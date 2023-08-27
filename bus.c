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
  if(addr >= 0xA000 && addr <= 0xBFFF)
  {
    if((c->ram[0x0001] & HIRAM) && (c->ram[0x0001] & LORAM))
    {
      return c->basic_rom[addr & 0x1FFF];
    }
  }

	/*
	 * NOTE: Why did I include this very specific IF statement?
	 *
	 * The reason is that CHAR_ROM and the I/O needs to be accessible at all times regardless of the values of LORAM and HIRAM,
	 * The memory locations for LORAM are (0x8000 - 0x9FFF) where input/output operations take place. 
	 * Specifically the registers of various hardware components, such as the VIC-II graphics chip and the SID sound chip, are located here.
	 *
	 * Whilst HIRAM also holds a similar purpose, with the locations being (0xD000 - 0xDFFF) having the same purpose as well.
	 *
	 */

  if(addr >= 0xD000 && addr <= 0xDFFF)
  {
    if((c->ram[0x0001] & LORAM) || (c->ram[0x0001] & HIRAM))
    {

			/*
			 * CHAREN is a signal thay either enables or disables the CHAR ROM.
			 * When CHAREN is (logic) 1 it is not stored into RAM but rather the contents of the I/O section.
			 * The RAM being the char_rom array.
			 */

      if(c->ram[0x0001] & CHAREN)
        {
          // Returns IO_READ
        }
      else 
      {
        return c->char_rom[addr & 0xFFF];
			}
    }
	}
  if(addr >= 0xE000 && addr <= 0xFFFF)
  {
    if(c->ram[0x0001] & HIRAM)
    {
      return c->kernal_rom[addr & 0x1FFF];
    }
  }
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
  if(addr >= 0xD000 && addr <= 0xDFFF)
  {
    if((c->ram[0x0001] & HIRAM || c->ram[0x0002] & LORAM) && (c->ram[0x0001] & CHAREN))
    {
      // IO_WRITE        
    }
  }
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
