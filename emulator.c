#include <stdint.h>

typedef struct {
  uint8_t memory[4096];
  uint8_t registers[16];
  uint8_t keyboard[16];
  uint32_t display[64][32];
  uint8_t delayTimer;
  uint8_t soundTimer;
  uint16_t programCounter; // Stores the current executing address
  uint8_t stackPointer; // Point to topmost level of the stack
  uint16_t stack[16];
  uint16_t registerI;
} Chip8;
