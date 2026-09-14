#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

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

void init(Chip8* chip) {
  chip->programCounter = 0x200;

}

// Loads ROM from given path into Chip8
void loadROM(Chip8* chip, char* fileName) {
  FILE* rom = fopen(fileName, "rb"); 
  if (!rom) {
    printf("Error loading ROM: %s\n", fileName);
    exit(1);
  }
  fread(&chip->memory[0x200], 1, sizeof(chip->memory), rom);
  fclose(rom);
}

// Debug
void printROM(Chip8* chip) {
  for (int i = 0; i < sizeof(chip->memory); i++) {
    printf("%d", chip->memory[i+0x200]);
  }
}

int main(int argc, char *argv[])
{
  if (argc != 2 ) {
    printf("Error: Usage: ./emulator <ROM path>\n");
    exit(1);
  }

  Chip8 c;
  loadROM(&c, argv[1]);
  return 1;
}
