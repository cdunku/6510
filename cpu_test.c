#include <time.h>
#include <string.h>

#include "cpu.h"
#include "bus.h"
#include "debug.h"

int 
load_file(MOS_6510* const c, const char* file_to_load, uint16_t addr)
{
  FILE *f = fopen(file_to_load, "rb");

  if(f == NULL) 
  {
    fprintf(stderr, "**" RED " Error " RESET "** " "file couldn't be opened"); 
    fclose(f); 
    return 1;
  }

  fseek(f, 0, SEEK_END);
  size_t file_size = ftell(f);
  rewind(f);

  if(file_size + addr > 0x10000) 
  {
    fprintf(stderr, "\n**" RED "Error" RESET "** " "file size to large\n"); 
    fclose(f); 
    return 1;
  }

  size_t file_read = fread(&c->ram[addr], sizeof(uint8_t), file_size, f);

  if(file_read != file_size) 
  {
    fprintf(stderr, "**" RED " Error " RESET "** " "file \"%s\" couldn't be read into memory" , file_to_load); 
    fclose(f); 
    return 1;
  }

  fclose(f);
  return 0;
}


static int 
execute_allsuiteasm(MOS_6510* const c, const char* file_to_load)
{
  memset(c->ram, 0, 0x10000);
  load_file(c, file_to_load, 0x4000);
  initialise(c);

  printf("\n** file loaded: " BOLD "%s" RESET " **\n", file_to_load);

  while(1)
  {
    mnemonics(c);
    if (c->pc == 0x45C0) {
      if (rb(c, 0x0210) == 0xFF) {
        printf(GREEN "✓" RESET " - test passed!\n");
      }
      else {
        printf(RED "✘" RESET " - test failed!\n");
      }
      break;
    }
  }

  return 0;
}
static int
execute_6502_decimal_test(MOS_6510* const c, const char* file_to_load)
{
  memset(c->ram, 0, 0x10000);
  if(load_file(c, file_to_load, 0x200) != 0) return 1;
  initialise(c);

  printf("\n** file loaded: " BOLD "%s" RESET " **\n", file_to_load);

  c->pc = 0x200;

  while(true)
  {
    mnemonics(c);
    if(c->pc == 0x024B)
    {
      printf("%s", c->a == 0 ? GREEN "✓" RESET " - test passed!\n" : RED "✘" RESET " - test failed!\n");
      break;
    }
  }
  return 0;
}

static int
execute_6502_interrupt_test(MOS_6510* const c, const char* file_to_load)
{
  memset(c->ram, 0, 0x10000);
  load_file(c, file_to_load, 0xA);
  initialise(c);

  printf("\n** file loaded: " BOLD "%s" RESET " **\n", file_to_load);
  
  c->pc = 0x400;

  uint16_t previous_pc = 0;
  wb(c, 0xBFFC, 0);
  while(true) 
  {
    mnemonics(c);
    // cpu_debug(c);

    c->irq_status = rb(c, 0xBFFC);
    interrupt_handler(c);
    wb(c, 0xBFFC, c->irq_status);
    if(previous_pc == c->pc)
    {
      if(c->pc == 0x06F5)
      {
        printf(GREEN "✓" RESET " - test passed!\n");
        break;
      }
      printf(RED "✘" RESET " - test failed! (trapped at " BOLD "0x%04X" RESET ")\n", c->pc);
      break;
    }
    previous_pc = c->pc;
  }
  return 0;
}

static int
execute_6502_functional_test(MOS_6510* const c, const char *file_to_load)
{
  memset(c->ram, 0, 0x10000);
  if(load_file(c, file_to_load, 0) != 0) return 1;
  initialise(c);

  printf("\n** file loaded: " BOLD "%s" RESET " **\n", file_to_load);

  c->pc = 0x400;

  uint16_t previous_pc = 0;

  while(true) 
  {
    mnemonics(c);
    if(previous_pc == c->pc)
    {
      if(c->pc == 0x3469)
      {
        printf(GREEN "✓" RESET " - test passed!\n");
        break;
      }
      printf(RED "✘" RESET " - test failed! (trapped at " BOLD "0x%04X" RESET ")\n", c->pc);
      break;
    }
    previous_pc = c->pc;
  }
  return 0;
}

static int 
execute_timingtest(MOS_6510* const c, const char* file_to_load)
{
  memset(c->ram, 0, 0x10000);
  if(load_file(c, file_to_load, 0x1000) != 0) return 1;
  initialise(c);

  printf("\n** file loaded: " BOLD "%s" RESET " **\n", file_to_load);

  c->pc = 0x1000;

  while(true)
  {
    mnemonics(c);
    // cpu_debug(c);
    if(c->pc == 0x1269)
    {
      printf("%s", c->cyc == 1141 ? GREEN "✓" RESET " - test passed!\n" : RED "✘" RESET " - test failed!\n");
      break;
    }
  }
  return 0;
}

int 
main(void)
{
  char* array[8];
  array[0] = " ________    ________    ______       ________ \n";
  array[1] = "|\\   ____\\  |\\   ____\\  /___   \\     |\\   __  \\ \n";
  array[2] = "\\ \\  \\      \\ \\ \\ ___/  \\___ \\  \\    \\ \\  \\|\\  \\  \n";
  array[3] = " \\ \\  \\_____ \\ \\ \\_____     \\ \\  \\    \\ \\  \\|\\  \\\n";
  array[4] = "  \\ \\    _  \\ \\ \\_______\\    \\ \\  \\    \\ \\  \\|\\  \\ \n";
  array[5] = "   \\ \\  \\|\\  \\ \\/______\\ \\    \\ \\  \\    \\ \\  \\|\\  \\  \n";
  array[6] = "    \\ \\_______\\ ________\\ \\  __\\_\\  \\___ \\ \\_______\\  \n";
  array[7] = "     \\|_______| \\_\\________\\ \\_\\________\\ \\|_______|         \n";

  for(int i = 0; i < 8; i++) {
    printf("%s", array[i]);
  }

  MOS_6510 c;

  const time_t time_start = time(NULL);

  execute_allsuiteasm(&c, "test_files/AllSuiteA.bin");
  execute_6502_decimal_test(&c, "test_files/6502_decimal_test.bin");
  execute_6502_interrupt_test(&c, "test_files/6502_interrupt_test.bin");
  execute_6502_functional_test(&c, "test_files/6502_functional_test.bin");
  execute_timingtest(&c, "test_files/timingtest-1.bin");
  
  const time_t time_end = time(NULL);

  printf("\nProgram executed in %ld seconds\n", (time_end - time_start) / 1000);
	return 0;
}
