#include <stdint.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>

#define START_ADDRESS 0x200

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

typedef void (*opcodeFunction) (Chip8* chip8, int16_t opcode);

// Initialize chip
void initChip(Chip8* chip) {
  chip->programCounter = START_ADDRESS;
}

// Loads ROM from given path into Chip8
void loadROM(Chip8* chip8, char* fileName) {
  FILE* rom = fopen(fileName, "rb"); 
  if (!rom) {
    printf("Error loading ROM: %s\n", fileName);
    exit(1);
  }
  fread(&chip8->memory[START_ADDRESS], 1, sizeof(chip8->memory) - START_ADDRESS, rom);
  fclose(rom);
}

// Clears the display
void opcode_00E0(Chip8* chip8, int16_t opcode) {
  memset(chip8->display, 0, sizeof(chip8->display)); 
}

void opcode_00EE(Chip8* chip8, int16_t opcode) {
}

// Emulate a CPU cycle
void cycle(Chip8* chip8) {
  // Fetch
  uint16_t opcode = (chip8->memory[chip8->programCounter] << 8) | chip8->memory[(chip8->programCounter) + 1];
  chip8->programCounter += 2;
  // Decode
  // Execute
}

int main(int argc, char *argv[])
{
  if (argc != 2 ) {
    printf("Error: Usage: ./emulator <ROM path>\n");
    exit(1);
  }

  Chip8 chip8;
  initChip(&chip8);
  loadROM(&chip8, argv[1]);
  cycle(&chip8);
  return 1;
}
