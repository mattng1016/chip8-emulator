#include <stdint.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>

#define START_ADDRESS 0x200

typedef struct {
  uint8_t memory[4096];
  uint8_t V[16];
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

// Returns from subroutine
void opcode_00EE(Chip8* chip8, int16_t opcode) {
  chip8->programCounter = chip8->stack[chip8->stackPointer];
  chip8->stackPointer--;
}

// Jump to location nnn
void opcode_1nnn(Chip8* chip8, int16_t opcode) {
  uint16_t address = opcode & 0xFFF;
  chip8->programCounter = address;
}

// Call subroutine at nnn
void opcode_2nnn(Chip8* chip8, int16_t opcode) {
  chip8->stackPointer++;
  chip8->stack[chip8->stackPointer] = chip8->programCounter;
  chip8->programCounter = opcode & 0xFFF;
}

// Skip next instruction if Vx == kk
void opcode_3xkk(Chip8* chip8, int16_t opcode) {
  uint16_t x = (opcode >> 8) & 0xF;
  uint16_t kk = opcode & 0xFF;
  if (chip8->V[x] == kk) {
    chip8->programCounter += 2;
  }
}

// Skip next instruction if Vx != kk
void opcode_4xkk(Chip8* chip8, int16_t opcode) {
  uint16_t x = (opcode >> 8) & 0xF;
  uint16_t kk = opcode & 0xFF;
  if (chip8->V[x] != kk) {
    chip8->programCounter += 2;
  }
}

// Skip next instruction if Vx == Vy
void opcode_5xy0(Chip8* chip8, int16_t opcode) {
  uint16_t x = (opcode >> 8) & 0xF;
  uint16_t y = (opcode >> 4) & 0xF;
  if (chip8->V[x] == chip8->V[y]) {
    chip8->programCounter += 2;
  }
}

void opcode_6xkk(Chip8* chip8, int16_t opcode) {

}

void opcode_7xkk(Chip8* chip8, int16_t opcode) {

}

void opcode_8xy0(Chip8* chip8, int16_t opcode) {

}

void opcode_8xy1(Chip8* chip8, int16_t opcode) {

}


void opcode_8xy2(Chip8* chip8, int16_t opcode) {

}

void opcode_8xy3(Chip8* chip8, int16_t opcode) {

}

void opcode_8xy4(Chip8* chip8, int16_t opcode) {

}

void opcode_8xy5(Chip8* chip8, int16_t opcode) {

}

void opcode_8xy6(Chip8* chip8, int16_t opcode) {

}

void opcode_8xy7(Chip8* chip8, int16_t opcode) {

}

void opcode_8xyE(Chip8* chip8, int16_t opcode) {

}

void opcode_9xy0(Chip8* chip8, int16_t opcode) {

}

void opcode_Annn(Chip8* chip8, int16_t opcode) {

}

void opcode_Bnnn(Chip8* chip8, int16_t opcode) {

}

void opcode_Cxkk(Chip8* chip8, int16_t opcode) {

}


void opcode_Dxyn(Chip8* chip8, int16_t opcode) {

}


void opcode_Ex9E(Chip8* chip8, int16_t opcode) {

}


void opcode_ExA1(Chip8* chip8, int16_t opcode) {

}


void opcode_Fx07(Chip8* chip8, int16_t opcode) {

}


void opcode_Fx0A(Chip8* chip8, int16_t opcode) {

}


void opcode_Fx15(Chip8* chip8, int16_t opcode) {

}


void opcode_Fx18(Chip8* chip8, int16_t opcode) {

}


void opcode_Fx1E(Chip8* chip8, int16_t opcode) {

}


void opcode_Fx29(Chip8* chip8, int16_t opcode) {

}


void opcode_Fx33(Chip8* chip8, int16_t opcode) {

}


void opcode_Fx55(Chip8* chip8, int16_t opcode) {

}


void opcode_Fx65(Chip8* chip8, int16_t opcode) {

}

// Emulate a CPU cycle
void cycle(Chip8* chip8) {
  // Fetch
  uint16_t opcode = (chip8->memory[chip8->programCounter] << 8) | chip8->memory[(chip8->programCounter) + 1];
  chip8->programCounter += 2;
  // Decode
  printf("%x\n", chip8->programCounter);
  chip8->V[0xB] = 0xA;
  chip8->V[0xC] = 0xA;
  opcode_5xy0(chip8, 0xABA0);
  printf("%x", chip8->programCounter);
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
