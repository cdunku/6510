#ifndef _6510_DEBUG
#define _6510_DEBUG

#define GREEN "\033[32m"
#define RED "\x1b[31m"
#define RESET "\x1b[m"
#define BOLD "\x1B[1m" 

#include <stdio.h>
#include <stdint.h>

struct debug {
  char *mnemonics;
  char *address_mode;
};

void cpu_debug(MOS_6510* const c);

#endif // _CPU_DEBUG
