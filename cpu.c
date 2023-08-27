#include <stdbool.h>
#include <stdlib.h>

#include "cpu.h"
#include "bus.h"
#include "debug.h"

static inline bool
page_crossed(uint16_t addr_1, uint16_t addr_2)
{
  return (addr_1 & 0xFF00) != (addr_2 & 0xFF00);
}

static inline void 
set_zn(MOS_6510* const c, uint8_t value)
{
  c->zf = value == 0;
  c->nf = value >> 7;
}

uint8_t 
get_flags(MOS_6510* const c) 
{
  uint8_t p = 0;

  p |= c->nf << 7;
  p |= c->vf << 6;
  p |= 1 << 5;
  p |= c->bf << 4;
  p |= c->df << 3;
  p |= c->idf << 2;
  p |= c->zf << 1;
  p |= c->cf;

  return p;
}

void
set_flags(MOS_6510* const c, uint8_t value)
{
  c->nf = (value >> 7) & 1;
  c->vf = (value >> 6) & 1;
  c->df = (value >> 3) & 1;
  c->bf = (value >> 4) & 1;
  c->idf = (value >> 2) & 1;
  c->zf = (value >> 1) & 1;
  c->cf = value & 1;
}

/* Addressing modes */

static inline void
address_mode(MOS_6510* const c, uint8_t mode)
{
  switch (mode) {

    case IMPLIED:
    case ACCUMULATOR:
      c->addr_ptr = 0;
      break;

    case IMMEDIATE:
      c->addr_ptr = c->pc++;
      break;

    case ABSOLUTE:
      c->addr_ptr = fetch_word(c);
      break;

    case ABSOLUTE_X:
      c->addr_ptr = fetch_word(c) + c->x;
      if(page_crossed(c->addr_ptr, c->addr_ptr - c->x)) c->page_crossed = 1;
      break;

    case ABSOLUTE_Y:
      c->addr_ptr = fetch_word(c) + c->y;
      if(page_crossed(c->addr_ptr, c->addr_ptr - c->y)) c->page_crossed = 1;
      break;

    case ZEROPAGE:
      c->addr_ptr = fetch_byte(c) & 0xFF;
      break;

    case ZEROPAGE_X:
      c->addr_ptr = (fetch_byte(c) + c->x) & 0xFF;
      break;

    case ZEROPAGE_Y:
      c->addr_ptr = (fetch_byte(c) + c->y) & 0xFF;
      break;

    case RELATIVE:
      c->addr_rel = (int8_t)fetch_byte(c);
      break; 

    case INDIRECT:
      c->addr_ptr = fetch_word(c);
      c->addr_ptr = rw(c, c->addr_ptr);
      break;

    case INDIRECT_Y:
      c->addr_ptr = rw(c, fetch_byte(c)) + c->y;
      if(page_crossed(c->addr_ptr, c->addr_ptr - c->y)) c->page_crossed = 1;
      break;

    case INDIRECT_X:
      c->addr_ptr = (fetch_byte(c) + c->x) & 0xFF;
      c->addr_ptr = rw(c, c->addr_ptr);
      break;

    default:
      fprintf(stderr, "\n**" RED " Error " RESET "**" " invalid addressing mode\n");
      exit(1);
  }
}

/* Documented opcodes */

/* Load, Store, Transfer instructions */

static inline void
LDA(MOS_6510* const c)
{
  c->a = rb(c, c->addr_ptr);
  set_zn(c, c->a);
}

static inline void
LDX(MOS_6510* const c)
{
  c->x = rb(c, c->addr_ptr);
  set_zn(c, c->x);
}

static inline void
LDY(MOS_6510* const c)
{
  c->y = rb(c, c->addr_ptr);
  set_zn(c, c->y);
}

static inline void
STA(MOS_6510* const c)
{
  wb(c, c->addr_ptr, c->a);
}

static inline void
STX(MOS_6510* const c)
{
  wb(c, c->addr_ptr, c->x);
}

static inline void
STY(MOS_6510* const c)
{
  wb(c, c->addr_ptr, c->y);
}

static inline void
TAX(MOS_6510* const c)
{
  c->x = c->a;
  set_zn(c, c->x);
}

static inline void
TAY(MOS_6510* const c)
{
  c->y = c->a;
  set_zn(c, c->y);
}

static inline void
TXA(MOS_6510* const c) 
{
  c->a = c->x;
  set_zn(c, c->a);
}

static inline void
TYA(MOS_6510* const c)
{
  c->a = c->y;
  set_zn(c, c->a);
}

static inline void
TSX(MOS_6510* const c) 
{
  c->x = c->sp;
  set_zn(c, c->x);
}

static inline void
TXS(MOS_6510* const c) 
{
  c->sp = c->x;
}


/* Arithmethic instructions */

static inline void
ADC(MOS_6510* const c)
{
  const uint8_t byte = rb(c, c->addr_ptr);
  const bool carry = c->cf;

  if(c->df)
  {
    /* Decimal mode ADC */

    uint8_t al = (c->a & 0x0F) + (byte & 0x0F) + carry;
    if(al > 0x09) al += 0x06;

    uint8_t ah = (c->a >> 4) + (byte >> 4) + (al > 0xF);


    c->vf = ((ah << 4) ^ c->a) & ~(c->a ^ byte) & 0x80; 

    if(ah > 0x09) ah += 0x06;

    c->nf = ah & 0x8;
    c->cf = ah > 0xF;

    c->a = (ah << 4) | (al & 0xF);
    c->zf = c->a == 0;
  }
  else 
  {
    const uint16_t result = c->a + byte + carry;

    c->vf = ~(c->a ^ byte) & (c->a ^ result) & 0x80;  

    c->cf = result > 0xFF;
    set_zn(c, result & 0xFF);

    c->a = result & 0xFF;
  }
}

static inline void
SBC(MOS_6510* const c)
{
  const uint8_t byte = rb(c, c->addr_ptr);
  const bool com_carry = !c->cf;

  if(c->df)
  {
    /* Decimal mode SBC */

    uint8_t al = (c->a & 0x0F) - (byte & 0x0F) - com_carry;
    const uint16_t dec_result = c->a - byte - com_carry;

    if(al >> 7) al -= 0x06;

    uint8_t ah = (c->a >> 4) - (byte >> 4) - (al >> 7);

    c->vf = (dec_result ^ c->a) & (c->a ^ byte) & 0x80;
    c->cf = !(dec_result > 255);

    if(ah >> 7) ah -= 0x06;

    c->a = (ah << 4) | (al & 0xF);

    set_zn(c, c->a);
  }
  else
  {
    const uint16_t result = c->a - byte - com_carry;

    c->vf = (result ^ c->a) & (c->a ^ byte) & 0x80; 

    c->cf = !(result > 255);
    set_zn(c, result & 0xFF);

    c->a = result & 0xFF;
  }
}

static inline void
DEC(MOS_6510* const c) 
{
  uint8_t value = rb(c, c->addr_ptr) - 1;
  wb(c, c->addr_ptr, value);
  set_zn(c, value);
}

static inline void
DEX(MOS_6510* const c)
{
  c->x--;
  set_zn(c, c->x);
}
static inline void
DEY(MOS_6510* const c)
{
  c->y--;
  set_zn(c, c->y);
}

static inline void
INX(MOS_6510* const c)
{
  c->x++;
  set_zn(c, c->x);
}

static inline void
INY(MOS_6510* const c) 
{
  c->y++;
  set_zn(c, c->y);
}

static inline void 
INC(MOS_6510* const c) 
{
  uint8_t value = rb(c, c->addr_ptr) + 1;
  wb(c, c->addr_ptr, value);
  set_zn(c, value);
}

/* Logical instructions */

static inline void
AND(MOS_6510* const c)
{
  uint8_t value = rb(c, c->addr_ptr);
  c->a &= value;
  set_zn(c, c->a);
}

static inline void
ORA(MOS_6510* const c)
{
  uint8_t value = rb(c, c->addr_ptr);
  c->a |= value;
  set_zn(c, c->a);
}

static inline void
EOR(MOS_6510* const c)
{
  uint8_t value = rb(c, c->addr_ptr);
  c->a ^= value;
  set_zn(c, c->a);
}

static inline void
BIT(MOS_6510* const c)
{
  uint8_t byte = rb(c, c->addr_ptr);
  uint8_t value = c->a & byte;

  c->vf = (byte >> 6) & 1;
  c->zf = value == 0;
  c->nf = byte >> 7;
}

static inline void
CMP(MOS_6510* const c)
{
  uint8_t byte = rb(c, c->addr_ptr);
  uint8_t result = c->a - byte;

  c->cf = c->a >= byte;
  set_zn(c, result);
}

static inline void
CPX(MOS_6510* const c)
{
  uint8_t byte = rb(c, c->addr_ptr);
  uint8_t result = c->x - byte;

  c->cf = c->x >= byte;
  set_zn(c, result);
}

static inline void
CPY(MOS_6510* const c)
{
  uint8_t byte = rb(c, c->addr_ptr);
  uint8_t result = c->y - byte;

  c->cf = c->y >= byte;
  set_zn(c, result);
}


/* Rotation instructions */

static inline void
ASL(MOS_6510* const c)
{
  uint8_t value = c->a << 1;

  set_zn(c, value);
  c->cf = c->a >> 7;

  c->a = value;
}

static inline void
ASL_MEM(MOS_6510* const c)
{
  uint8_t byte = rb(c, c->addr_ptr);
  uint8_t value = byte << 1;

  set_zn(c, value);
  c->cf = byte >> 7;

  wb(c, c->addr_ptr, value);
}

static inline void
LSR(MOS_6510* const c)
{
  uint8_t value = c->a >> 1;

  set_zn(c, value);
  c->cf = c->a & 1;

  c->a = value;
}

static inline void
LSR_MEM(MOS_6510* const c)
{
  uint8_t byte = rb(c, c->addr_ptr);
  uint8_t value = byte >> 1;

  set_zn(c, value);
  c->cf = byte & 1;

  wb(c, c->addr_ptr, value);
}

static inline void
ROL(MOS_6510* const c)
{
  uint8_t value = c->a << 1;

  value |= c->cf;  
  set_zn(c, value);
  c->cf = c->a >> 7; 

  c->a = value;
}

static inline void
ROL_MEM(MOS_6510* const c)
{
  uint8_t byte = rb(c, c->addr_ptr);
  uint8_t value = byte << 1;

  value |= c->cf;  
  set_zn(c, value);
  c->cf = byte >> 7;

  wb(c, c->addr_ptr, value);
}

static inline void
ROR(MOS_6510* const c)
{
  uint8_t value = c->a >> 1;

  value |= c->cf << 7; 
  set_zn(c, value);
  c->cf = c->a & 1;

  c->a = value;
}

static inline void
ROR_MEM(MOS_6510* const c)
{
  uint8_t byte = rb(c, c->addr_ptr);
  uint8_t value = byte >> 1;

  value |= c->cf << 7; 
  set_zn(c, value);
  c->cf = byte & 1;
 
  wb(c, c->addr_ptr, value);
}


/* Branching instructions */

static inline void
BPL(MOS_6510* const c)
{
  if(!c->nf)
  {
    if(page_crossed(c->pc, c->pc + c->addr_rel)) c->page_crossed = 1;
    c->pc += c->addr_rel;
    c->cyc++;
  }
}

static inline void
BMI(MOS_6510* const c)
{
  if(c->nf)
  {
    if(page_crossed(c->pc, c->pc + c->addr_rel)) c->page_crossed = 1;
    c->pc += c->addr_rel;
    c->cyc++;
  }
}

static inline void
BVC(MOS_6510* const c)
{
  if(!c->vf)
  {
    if(page_crossed(c->pc, c->pc + c->addr_rel)) c->page_crossed = 1;
    c->pc += c->addr_rel;
    c->cyc++;
  }
}

static inline void
BVS(MOS_6510* const c)
{
  if(c->vf)
  {
    if(page_crossed(c->pc, c->pc + c->addr_rel)) c->page_crossed = 1;
    c->pc += c->addr_rel;
    c->cyc++;
  }
}

static inline void
BCC(MOS_6510* const c)
{
  if(!c->cf)
  {
    if(page_crossed(c->pc, c->pc + c->addr_rel)) c->page_crossed = 1;
    c->pc += c->addr_rel;
    c->cyc++;
  }
}

static inline void
BCS(MOS_6510* const c)
{
  if(c->cf)
  {
    if(page_crossed(c->pc, c->pc + c->addr_rel)) c->page_crossed = 1;
    c->pc += c->addr_rel;
    c->cyc++;
  }
}

static inline void
BNE(MOS_6510* const c)
{
  if(!c->zf)
  {
    if(page_crossed(c->pc, c->pc + c->addr_rel)) c->page_crossed = 1;
    c->pc += c->addr_rel;
    c->cyc++;
  }
}

static inline void
BEQ(MOS_6510* const c)
{
  if(c->zf)
  {
    if(page_crossed(c->pc, c->pc + c->addr_rel)) c->page_crossed = 1;
    c->pc += c->addr_rel;
    c->cyc++;
  }
}

/* Stack instructions */

static inline void
PHA(MOS_6510* const c)
{
  push_byte(c, c->a);
}

static inline void
PHP(MOS_6510* const c)
{
  c->bf = 1;
  push_byte(c, get_flags(c));
}

static inline void
PLA(MOS_6510* const c)
{
  c->a = pop_byte(c);
  set_zn(c, c->a);
}

static inline void
PLP(MOS_6510* const c)
{
  set_flags(c, pop_byte(c));
}

/* System instructions */

static inline void
BRK(MOS_6510* const c)
{
  c->bf = 1;

  c->pc++;

  push_word(c, c->pc);
  push_byte(c, get_flags(c));

  c->idf = 1;

  c->pc = rw(c, INTERRUPT_VECTOR);
}

void
IRQ(MOS_6510* const c)
{
  if(c->idf) return;

  push_word(c, c->pc);
  c->bf = 0;
  push_byte(c, get_flags(c));
  c->idf = 1;

  c->pc = rw(c, INTERRUPT_VECTOR);

  c->cyc += 7;
}

void
NMI(MOS_6510* const c)
{
  push_word(c, c->pc);
  c->bf = 0;
  push_byte(c, get_flags(c));
  c->idf = 1;

  c->pc = rw(c, NMI_VECTOR);

  c->cyc += 7;
}

static inline void
RTS(MOS_6510* const c)
{
  c->pc = pop_word(c);
  c->pc++;
}

static inline void
JMP(MOS_6510* const c)
{
  c->pc = c->addr_ptr;
}

static inline void
JSR(MOS_6510* const c)
{
  push_word(c, c->pc - 1);
  c->pc = c->addr_ptr;
}

static inline void
NOP(MOS_6510* const c) 
{
  (void) c;
}

static inline void
RTI(MOS_6510* const c)
{
  set_flags(c, pop_byte(c));
  c->pc = pop_word(c);
}

/* Clear, Set instructions */

static inline void
CLC(MOS_6510* const c) 
{
  c->cf = 0;
}

static inline void
CLD(MOS_6510* const c) 
{
  c->df = 0;
}

static inline void
CLI(MOS_6510* const c) 
{
  c->idf = 0;
}

static inline void
CLV(MOS_6510* const c) 
{
  c->vf = 0;
}

static inline void
SEC(MOS_6510* const c) 
{
  c->cf = 1;
}

static inline void
SED(MOS_6510* const c) 
{
  c->df = 1;
}

static inline void
SEI(MOS_6510* const c) 
{
  c->idf = 1;
}

/* Undocumented opcodes */

static inline void
JAM(MOS_6510* const c)
{
  (void) c;
  while(1)
  {
    /* Does nothing */
  }
}

static inline void 
SLO(MOS_6510* const c)
{
  ASL_MEM(c);
  ORA(c);
}

static inline void
ANC(MOS_6510* const c)
{
  AND(c);
  uint8_t value = c->a << 1;

  set_zn(c, value);
  c->cf = c->a >> 7;
}

static inline void
RLA(MOS_6510* const c)
{
  ROL_MEM(c);
  AND(c);
}

static inline void
SRE(MOS_6510* const c)
{
  LSR_MEM(c);
  EOR(c);
}

static inline void
ALR(MOS_6510* const c)
{
  AND(c);
  LSR(c);
}

static inline void
RRA(MOS_6510* const c)
{
  ROR_MEM(c);
  ADC(c);
}

static inline void
SAX(MOS_6510* const c)
{
  wb(c, c->addr_ptr, c->a & c->x);
}

static inline void
LAX(MOS_6510* const c)
{
  set_zn(c, c->a = c->x = rb(c, c->addr_ptr));
}

static inline void
DCP(MOS_6510* const c)
{
  DEC(c);
  CMP(c);
}

static inline void
ARR(MOS_6510* const c)
{
  AND(c); 
  ROR(c);
}

static inline void
TAS(MOS_6510* const c)
{
  c->sp = c->a & c->x;
  wb(c, c->addr_ptr, c->a & c->x & ((c->addr_ptr >> 8) + 1));
}

static inline void
LAS(MOS_6510* const c)
{
  c->a = c->x = c->sp = c->addr_ptr & c->sp;
}

static inline void
USBC(MOS_6510* const c)
{
  SBC(c);
  NOP(c);
}

static inline void
XAA(MOS_6510* const c) 
{
  c->a = (c->a | UNSTABLE_CONST) & c->x;
  c->a &= c->addr_ptr;
  set_zn(c, c->a);
}

static inline void
AHX(MOS_6510* const c) 
{
  wb(c, c->addr_ptr, c->a & c->x & ((c->addr_ptr >> 8) + 1));
}

static inline void
SHY(MOS_6510* const c) 
{
  wb(c, c->addr_ptr, c->y & ((c->addr_ptr >> 8) + 1));
}

static inline void
SHX(MOS_6510* const c) 
{
  wb(c, c->addr_ptr, c->x & ((c->addr_ptr >> 8) + 1));
}

static inline void
AXS(MOS_6510* const c) 
{
  wb(c, c->addr_ptr, c->x & c->a);
}

static inline void
ISC(MOS_6510* const c) 
{
  INC(c);
  SBC(c);
}

void 
interrupt_handler(MOS_6510* const c)
{
  if((c->irq_status & 0x2) == 0x2)
  {
    NMI(c);
    c->irq_status &= ~0x2;
  }
  if(!c->idf && (c->irq_status & 0x1) == 0x1)
  {
    IRQ(c);
    c->irq_status &= ~0x1;
  }
}


/* Opcode execution array */

struct instruction opcodes[256] = 
{
  {BRK, 7, IMPLIED, 0}, /* 0x00 */
  {ORA, 6, INDIRECT_X, 0}, /* 0x01 */
  {JAM, 2, IMPLIED, 0}, /* 0x02 */
  {SLO, 8, INDIRECT_X, 0}, /* 0x03 */
  {NOP, 3, ZEROPAGE, 0}, /* 0x04 */
  {ORA, 3, ZEROPAGE, 0}, /* 0x05 */
  {ASL_MEM, 5, ZEROPAGE, 0}, /* 0x06 */
  {SLO, 5, ZEROPAGE, 0}, /* 0x07 */
  {PHP, 3, IMPLIED, 0}, /* 0x08 */
  {ORA, 2, IMMEDIATE, 0}, /* 0x09 */
  {ASL, 2, ACCUMULATOR, 0}, /* 0x0A */
  {ANC, 2, IMMEDIATE, 0}, /* 0x0B */
  {NOP, 4, ABSOLUTE, 0}, /* 0x0C */
  {ORA, 4, ABSOLUTE, 0}, /* 0x0D */
  {ASL_MEM, 6, ABSOLUTE, 0}, /* 0x0E */
  {SLO, 6, ABSOLUTE, 0}, /* 0x0F */
  {BPL, 2, RELATIVE, 1}, /* 0x10 */
  {ORA, 5, INDIRECT_Y, 1}, /* 0x11 */
  {JAM, 0, IMPLIED, 0}, /* 0x12 */
  {SLO, 8, INDIRECT_X, 0}, /* 0x13 */
  {NOP, 4, ZEROPAGE_X, 0}, /* 0x14 */
  {ORA, 4, ZEROPAGE_X, 0}, /* 0x15 */
  {ASL_MEM, 6, ZEROPAGE_X, 0}, /* 0x16 */
  {SLO, 6, ZEROPAGE, 0}, /* 0x17 */
  {CLC, 2, IMPLIED, 0}, /* 0x18 */
  {ORA, 4, ABSOLUTE_Y, 1}, /* 0x19 */
  {NOP, 2, IMPLIED, 0}, /* 0x1A */
  {SLO, 7, ABSOLUTE_Y, 0}, /* 0x1B */
  {NOP, 4, ABSOLUTE_X, 1}, /* 0x1C */
  {ORA, 4, ABSOLUTE_X, 1}, /* 0x1D */
  {ASL_MEM, 7, ABSOLUTE_X, 0}, /* 0x1E */
  {SLO, 7, ABSOLUTE_X, 0}, /* 0x1F */
  {JSR, 6, ABSOLUTE, 0}, /* 0x20 */
  {AND, 6, INDIRECT_X, 0}, /* 0x21 */
  {JAM, 0, IMPLIED, 0}, /* .0x22 */
  {RLA, 8, INDIRECT_X, 0}, /* 0x23 */
  {BIT, 3, ZEROPAGE, 0}, /* 0x24 */
  {AND, 3, ZEROPAGE, 0}, /* 0x25 */
  {ROL_MEM, 5, ZEROPAGE, 0}, /* 0x26 */
  {RLA, 5, ZEROPAGE, 0}, /* 0x27 */
  {PLP, 4, IMPLIED, 0}, /* 0x28 */
  {AND, 2, IMMEDIATE, 0}, /* 0x29 */
  {ROL, 2, ACCUMULATOR, 0}, /* 0x2A */
  {ANC, 2, IMMEDIATE, 0}, /* 0x2B */
  {BIT, 4, ABSOLUTE, 0}, /* 0x2C */
  {AND, 4, ABSOLUTE, 0}, /* 0x2D */
  {ROL_MEM, 6, ABSOLUTE, 0}, /* 0x2E */
  {RLA, 6, ABSOLUTE, 0}, /* 0x2F */
  {BMI, 2, RELATIVE, 1}, /* 0x30 */
  {AND, 5, INDIRECT_Y, 1}, /* 0x31 */
  {JAM, 0, IMPLIED, 0}, /* 0x32 */
  {RLA, 8, INDIRECT_Y, 0}, /* 0x33 */
  {NOP, 4, ZEROPAGE_X, 0}, /* 0x34 */
  {AND, 4, ZEROPAGE_X, 0}, /* 0x35 */
  {ROL_MEM, 6, ZEROPAGE_X, 0}, /* 0x36 */
  {RLA, 6, ZEROPAGE_X, 0}, /* 0x37 */
  {SEC, 2, IMPLIED, 0}, /* 0x38 */
  {AND, 4, ABSOLUTE_Y, 1}, /* 0x39 */
  {NOP, 2, IMPLIED, 0}, /* 0x3A */
  {RLA, 7, ABSOLUTE_Y, 0}, /* 0x3B */
  {NOP, 4, ABSOLUTE_Y, 1}, /* 0x3C */
  {AND, 4, ABSOLUTE_X, 1}, /* 0x3D */
  {ROL_MEM, 7, ABSOLUTE_X, 0}, /* 0x3E */
  {RLA, 7, ABSOLUTE_X, 0}, /* 0x3F */
  {RTI, 6, IMPLIED, 0}, /* 0x40 */
  {EOR, 6, INDIRECT_X, 0}, /* 0x41 */
  {JAM, 0, IMPLIED, 0}, /* 0x42 */
  {SRE, 8, INDIRECT_X, 0}, /* 0x43 */
  {NOP, 3, ZEROPAGE, 0}, /* 0x44 */
  {EOR, 3, ZEROPAGE, 0}, /* 0x45 */
  {LSR_MEM, 5, ZEROPAGE, 0}, /* 0x46 */
  {SRE, 5, ZEROPAGE, 0}, /* 0x47 */
  {PHA, 3, IMPLIED, 0}, /* 0x48 */
  {EOR, 2, IMMEDIATE, 0}, /* 0x49 */
  {LSR, 2, ACCUMULATOR, 0}, /* 0x4A */
  {ALR, 2, IMMEDIATE, 0}, /* 0x4B */
  {JMP, 3, ABSOLUTE, 0}, /* 0x4C */
  {EOR, 4, ABSOLUTE, 0}, /* 0x4D */
  {LSR_MEM, 6, ABSOLUTE, 0}, /* 0x4E */
  {SRE, 6, ABSOLUTE, 0}, /* 0x4F */
  {BVC, 2, RELATIVE, 1}, /* 0x50 */
  {EOR, 5, INDIRECT_Y, 1}, /* 0x51 */
  {JAM, 0, IMPLIED, 0}, /* 0x52 */
  {SRE, 8, INDIRECT_Y, 0}, /* 0x53 */
  {NOP, 4, ZEROPAGE_X, 0}, /* 0x54 */
  {EOR, 4, ZEROPAGE_X, 0}, /* 0x55 */
  {LSR_MEM, 6, ZEROPAGE_X, 0}, /* 0x56 */
  {SRE, 6, ZEROPAGE_X, 0}, /* 0x57 */
  {CLI, 2, IMPLIED, 0}, /* 0x58 */
  {EOR, 4, ABSOLUTE_Y, 1}, /* 0x59 */
  {NOP, 2, IMPLIED, 0}, /* 0x5A */
  {SRE, 7, ABSOLUTE_Y, 0}, /* 0x5B */
  {NOP, 4, ABSOLUTE_X, 1}, /* 0x5C */
  {EOR, 4, ABSOLUTE_X, 1}, /* 0x5D */
  {LSR_MEM, 7, ABSOLUTE_X, 0}, /* 0x5E */
  {SRE, 7, ABSOLUTE_X, 0}, /* 0x5F */
  {RTS, 6, IMPLIED, 0}, /* 0x60 */
  {ADC, 6, INDIRECT_X, 0}, /* 0x61 */
  {JAM, 0, IMPLIED, 0}, /* 0x62 */
  {RRA, 8, INDIRECT_X, 0}, /* 0x63 */
  {NOP, 3, ZEROPAGE, 0}, /* 0x64 */
  {ADC, 3, ZEROPAGE, 0}, /* 0x65 */
  {ROR_MEM, 5, ZEROPAGE, 0}, /* 0x66 */
  {RRA, 5, ZEROPAGE, 0}, /* 0x67 */
  {PLA, 4, IMPLIED, 0}, /* 0x68 */
  {ADC, 2, IMMEDIATE, 0}, /* 0x69 */
  {ROR, 2, ACCUMULATOR, 0}, /* 0x6A */
  {ARR, 2, IMMEDIATE, 0}, /* 0x6B */
  {JMP, 5, INDIRECT, 0}, /* 0x6C */
  {ADC, 4, ABSOLUTE, 0}, /* 0x6D */
  {ROR_MEM, 6, ABSOLUTE, 0}, /* 0x6E */
  {RRA, 6, ABSOLUTE, 0}, /* 0x6F */
  {BVS, 2, RELATIVE, 1}, /* 0x70 */
  {ADC, 5, INDIRECT_Y, 1}, /* 0x71 */
  {JAM, 0, IMPLIED, 0}, /* 0x72 */
  {RRA, 8, INDIRECT_Y, 0}, /* 0x73 */
  {NOP, 4, ZEROPAGE_X, 0}, /* 0x74 */
  {ADC, 4, ZEROPAGE_X, 0}, /* 0x75 */
  {ROR_MEM, 6, ZEROPAGE_X, 0}, /* 0x76 */
  {RRA, 6, ZEROPAGE_X, 0}, /* 0x77 */
  {SEI, 2, IMPLIED, 0}, /* 0x78 */
  {ADC, 4, ABSOLUTE_Y, 1}, /* 0x79 */
  {NOP, 2, IMPLIED, 0}, /* 0x7A */
  {RRA, 7, ABSOLUTE_Y, 0}, /* 0x7B */
  {NOP, 4, ABSOLUTE_X, 1}, /* 0x7C */
  {ADC, 4, ABSOLUTE_X, 1}, /* 0x7D */
  {ROR_MEM, 7, ABSOLUTE_X, 0}, /* 0x7E */
  {RRA, 7, ABSOLUTE_X, 0}, /* 0x7F */ 
  {NOP, 2, IMMEDIATE, 0}, /* 0x80 */
  {STA, 6, INDIRECT_X, 0}, /* 0x81 */
  {NOP, 2, IMMEDIATE, 0}, /* 0x82 */
  {SAX, 6, INDIRECT_X, 0}, /* 0x83 */
  {STY, 3, ZEROPAGE, 0}, /* 0x84 */
  {STA, 3, ZEROPAGE, 0}, /* 0x85 */
  {STX, 3, ZEROPAGE, 0}, /* 0x86 */
  {SAX, 3, ZEROPAGE, 0}, /* 0x87 */
  {DEY, 2, IMPLIED, 0}, /* 0x88 */
  {NOP, 2, IMMEDIATE, 0}, /* 0x89 */
  {TXA, 2, IMPLIED, 0}, /* 0x8A */
  {XAA, 2, IMMEDIATE, 0}, /* 0x8B */
  {STY, 4, ABSOLUTE, 0}, /* 0x8C */
  {STA, 4, ABSOLUTE, 0}, /* 0x8D */
  {STX, 4, ABSOLUTE, 0}, /* 0x8E */
  {SAX, 4, ABSOLUTE, 0}, /* 0x8F */
  {BCC, 2, RELATIVE, 1}, /* 0x90 */
  {STA, 6, INDIRECT_Y, 0}, /* 0x91 */
  {JAM, 0, IMPLIED, 0}, /* 0x92 */
  {AHX, 6, INDIRECT_Y, 0}, /* 0x93 */
  {STY, 4, ZEROPAGE_X, 0}, /* 0x94 */
  {STA, 4, ZEROPAGE_X, 0}, /* 0x95 */
  {STX, 4, ZEROPAGE_Y, 0}, /* 0x96 */
  {SAX, 4, ZEROPAGE_Y, 0}, /* 0x97 */
  {TYA, 2, IMPLIED, 0}, /* 0x98 */
  {STA, 5, ABSOLUTE_Y, 0}, /* 0x99 */
  {TXS, 2, IMPLIED, 0}, /* 0x9A */
  {TAS, 5, ABSOLUTE_Y, 0}, /* 0x9B */
  {SHY, 5, ABSOLUTE_X, 0}, /* 0x9C */
  {STA, 5, ABSOLUTE_X, 0}, /* 0x9D */
  {SHX, 5, ABSOLUTE_Y, 0}, /* 0x9E */
  {AHX, 5, ABSOLUTE_Y, 0}, /* 0x9F */ 
  {LDY, 2, IMMEDIATE, 0}, /* 0xA0 */
  {LDA, 6, INDIRECT_X, 0}, /* 0xA1 */
  {LDX, 2, IMMEDIATE, 0}, /* 0xA2 */
  {LAX, 6, INDIRECT_X, 0}, /* 0xA3 */
  {LDY, 3, ZEROPAGE, 0}, /* 0xA4 */
  {LDA, 3, ZEROPAGE, 0}, /* 0xA5 */
  {LDX, 3, ZEROPAGE, 0}, /* 0xA6 */
  {LAX, 3, ZEROPAGE, 0}, /* 0xA7 */
  {TAY, 2, IMPLIED, 0}, /* 0xA8 */
  {LDA, 2, IMMEDIATE, 0}, /* 0xA9 */
  {TAX, 2, IMPLIED, 0}, /* 0xAA */
  {LAX, 2, IMMEDIATE, 0}, /* 0xAB */
  {LDY, 4, ABSOLUTE, 0}, /* 0xAC */
  {LDA, 4, ABSOLUTE, 0}, /* 0xAD */
  {LDX, 4, ABSOLUTE, 0}, /* 0xAE */
  {LAX, 4, ABSOLUTE, 0}, /* 0xAF */
  {BCS, 2, RELATIVE, 1}, /* 0xB0 */
  {LDA, 5, INDIRECT_Y, 1}, /* 0xB1 */
  {JAM, 0, IMPLIED, 0}, /* 0xB2 */
  {LAX, 5, INDIRECT_Y, 1}, /* 0xB3 */
  {LDY, 4, ZEROPAGE_X, 0}, /* 0xB4 */
  {LDA, 4, ZEROPAGE_X, 0}, /* 0xB5 */
  {LDX, 4, ZEROPAGE_Y, 0}, /* 0xB6 */
  {LAX, 4, ZEROPAGE_Y, 0}, /* 0xB7 */
  {CLV, 2, IMPLIED, 0}, /* 0xB8 */
  {LDA, 4, ABSOLUTE_Y, 1}, /* 0xB9 */
  {TSX, 2, IMPLIED, 0}, /* 0xBA */
  {LAS, 4, ABSOLUTE_Y, 1}, /* 0xBB */
  {LDY, 4, ABSOLUTE_X, 1}, /* 0xBC */
  {LDA, 4, ABSOLUTE_X, 1}, /* 0xBD */
  {LDX, 4, ABSOLUTE_Y, 1}, /* 0xBE */
  {LAX, 4, ABSOLUTE_Y, 1}, /* 0xBF */ 
  {CPY, 2, IMMEDIATE, 0}, /* 0xC0 */
  {CMP, 6, INDIRECT_X, 0}, /* 0xC1 */
  {NOP, 2, IMMEDIATE, 0}, /* 0xC2 */
  {DCP, 8, INDIRECT_X, 0}, /* 0xC3 */
  {CPY, 3, ZEROPAGE, 0}, /* 0xC4 */
  {CMP, 3, ZEROPAGE, 0}, /* 0xC5 */
  {DEC, 5, ZEROPAGE, 0}, /* 0xC6 */
  {DCP, 5, ZEROPAGE, 0}, /* 0xC7 */
  {INY, 2, IMPLIED, 0}, /* 0xC8 */
  {CMP, 2, IMMEDIATE, 0}, /* 0xC9 */
  {DEX, 2, IMPLIED, 0}, /* 0xCA */
  {AXS, 2, IMMEDIATE, 0}, /* 0xCB */
  {CPY, 4, ABSOLUTE, 0}, /* 0xCC */
  {CMP, 4, ABSOLUTE, 0}, /* 0xCD */
  {DEC, 6, ABSOLUTE, 0}, /* 0xCE */
  {DCP, 6, ABSOLUTE_X, 0}, /* 0xCF */
  {BNE, 2, RELATIVE, 1}, /* 0xD0 */
  {CMP, 5, INDIRECT_Y, 1}, /* 0xD1 */
  {JAM, 0, IMPLIED, 0}, /* 0xD2 */
  {DCP, 8, INDIRECT_Y, 0}, /* 0xD3 */
  {NOP, 4, ZEROPAGE_X, 0}, /* 0xD4 */
  {CMP, 4, ZEROPAGE_X, 0}, /* 0xD5 */
  {DEC, 6, ZEROPAGE_X, 0}, /* 0xD6 */
  {DCP, 6, ZEROPAGE_X, 0}, /* 0xD7 */
  {CLD, 2, IMPLIED, 0}, /* 0xD8 */
  {CMP, 4, ABSOLUTE_Y, 1}, /* 0xD9 */
  {NOP, 2, IMPLIED, 0}, /* 0xDA */
  {DCP, 7, ABSOLUTE_Y, 0}, /* 0xDB */
  {NOP, 4, ABSOLUTE_X, 1}, /* 0xDC */
  {CMP, 4, ABSOLUTE_X, 1}, /* 0xDD */
  {DEC, 7, ABSOLUTE_X, 0}, /* 0xDE */
  {DCP, 7, ABSOLUTE_X, 0}, /* 0xDF */ 
  {CPX, 2, IMMEDIATE, 0}, /* 0xE0 */
  {SBC, 6, INDIRECT_X, 0}, /* 0xE1 */
  {NOP, 2, IMMEDIATE, 0}, /* 0xE2 */
  {ISC, 8, INDIRECT_X, 0}, /* 0xE3 */
  {CPX, 3, ZEROPAGE, 0}, /* 0xE4 */
  {SBC, 3, ZEROPAGE, 0}, /* 0xE5 */
  {INC, 5, ZEROPAGE, 0}, /* 0xE6 */
  {ISC, 5, ZEROPAGE, 0}, /* 0xE7 */
  {INX, 2, IMPLIED, 0}, /* 0xE8 */
  {SBC, 2, IMMEDIATE, 0}, /* 0xE9 */
  {NOP, 2, IMPLIED, 0}, /* 0xEA */
  {USBC, 2, IMMEDIATE, 0}, /* 0xEB */
  {CPX, 4, ABSOLUTE, 0}, /* 0xEC */
  {SBC, 4, ABSOLUTE, 0}, /* 0xED */
  {INC, 6, ABSOLUTE, 0}, /* 0xEE */
  {ISC, 6, ABSOLUTE, 0}, /* 0xEF */
  {BEQ, 2, RELATIVE, 1}, /* 0xF0 */
  {SBC, 5, INDIRECT_Y, 1}, /* 0xF1 */
  {JAM, 0, IMPLIED, 0}, /* 0xF2 */
  {ISC, 8, INDIRECT_Y, 0}, /* 0xF3 */
  {NOP, 4, ZEROPAGE_X, 0}, /* 0xF4 */
  {SBC, 4, ZEROPAGE_X, 0}, /* 0xF5 */
  {INC, 6, ZEROPAGE_X, 0}, /* 0xF6 */
  {ISC, 6, ZEROPAGE_X, 0}, /* 0xF7 */
  {SED, 2, IMPLIED, 0}, /* 0xF8 */
  {SBC, 4, ABSOLUTE_Y, 1}, /* 0xF9 */
  {NOP, 2, IMPLIED, 0}, /* 0xFA */
  {ISC, 7, ABSOLUTE_Y, 0}, /* 0xFB */
  {NOP, 4, ABSOLUTE_X, 1}, /* 0xFC */
  {SBC, 4, ABSOLUTE_X, 1}, /* 0xFD */
  {INC, 7, ABSOLUTE_X, 0}, /* 0xFE */
  {ISC, 7, IMPLIED, 0}, /* 0xFF */ 
};

void initialise(MOS_6510* const c)
{
  c->cyc = 0;

  c->a = 0;
  c->x = 0;
  c->y = 0;

  c->nf = 0;
  c->vf = 0;
  c->bf = 0;
  c->df = 0;
  c->idf = 0;
  c->zf = 0;
  c->cf = 0;

  c->sp = 0xFD;
  c->pc = rw(c, RESET_VECTOR);

  c->addr_ptr = 0;
  c->addr_rel = 0;

  c->irq_status = 0;
  // c->ram[0x0000] = 0x2F; /* All inputs! */
  // c->ram[0x0001] = 0x37;
}  

void
mnemonics(MOS_6510* const c)
{
  const uint8_t opcode = fetch_byte(c);

  c->cyc += opcodes[opcode].cycle;
  c->page_crossed = 0;  

  address_mode(c, opcodes[opcode].address_mode);

  opcodes[opcode].func(c);

  if(c->page_crossed)
  {
    c->cyc += opcodes[opcode].crossed_cycles;
  }
}
