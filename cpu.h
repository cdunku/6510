#ifndef _6510_CPU
#define _6510_CPU

#include <stdint.h>
#include <stdbool.h>

#define NMI_VECTOR 0xFFFA
#define RESET_VECTOR 0xFFFC
#define INTERRUPT_VECTOR 0xFFFE
#define UNSTABLE_CONST 0xEE // Common values beeing 0x00, 0xEE, 0xFF 

enum ADDR_MODE {
  IMPLIED,
  ACCUMULATOR,
  RELATIVE,
  IMMEDIATE,
  ZEROPAGE,
  ZEROPAGE_X,
  ZEROPAGE_Y,
  ABSOLUTE,
  ABSOLUTE_X,
  ABSOLUTE_Y,
  INDIRECT,
  INDIRECT_X,
  INDIRECT_Y,
};

typedef struct MOS_6510 
{
  uint8_t a, 
          x, 
          y; 

  uint8_t sp; 
  uint16_t pc; 

  bool nf, vf, bf, df, idf, zf, cf;
  uint64_t cyc;
  bool page_crossed;

  uint8_t ram[65536]; // 64KB

  uint16_t addr_ptr; // For normal addressing modes
  int8_t addr_rel; // For relative mode (Branching)

  uint8_t irq_status;

} MOS_6510;

struct instruction 
{
  void (*func)(MOS_6510* const c);
  uint64_t cycle;
  enum ADDR_MODE address_mode;
  uint8_t crossed_cycles;
};

void initialise(MOS_6510* const c);
void mnemonics(MOS_6510* const c);

uint8_t get_flags(MOS_6510* const c);
void set_flags(MOS_6510* const c, uint8_t value); 

void interrupt_handler(MOS_6510* const c);

#endif // _6510_CPU
