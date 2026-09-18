#include <SDL3/SDL.h>
#include <SDL3/SDL_main.h>

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#define START_ADDRESS 0x200

static SDL_Window *window = NULL;
static SDL_Renderer *renderer = NULL;

typedef struct {
  uint8_t memory[4096];
  uint8_t V[16];
  uint8_t keyboard[16];
  uint32_t display[64][32];
  uint8_t delayTimer;
  uint8_t soundTimer;
  uint16_t programCounter; // Stores the current executing address
  uint8_t stackPointer;    // Point to topmost level of the stack
  uint16_t stack[16];
  uint16_t I;
} Chip8;

uint8_t sprite[80] = {
    0xF0, 0x90, 0x90, 0x90, 0xF0, 0x20, 0x60, 0x20, 0x20, 0x70, 0xF0, 0x10,
    0xF0, 0x80, 0xF0, 0xF0, 0x10, 0xF0, 0x10, 0xF0, 0x90, 0x90, 0xF0, 0x10,
    0x10, 0xF0, 0x80, 0xF0, 0x10, 0xF0, 0xF0, 0x80, 0xF0, 0x90, 0xF0, 0xF0,
    0x10, 0x20, 0x40, 0x04, 0xF0, 0x90, 0xF0, 0x90, 0xF0, 0xF0, 0x90, 0xF0,
    0x10, 0xF0, 0xF0, 0x90, 0xF0, 0x90, 0x90, 0xE0, 0x90, 0xE0, 0x90, 0xE0,
    0xF0, 0x90, 0x80, 0x80, 0xF0, 0xE0, 0x90, 0x90, 0x90, 0xE0, 0xF0, 0x80,
    0xF0, 0x80, 0xF0, 0xF0, 0x80, 0xF0, 0x80, 0x80};

typedef void (*opcodeFunction)(Chip8 *chip8, int16_t opcode);

// Initialize chip
void initChip(Chip8 *chip8) {
  chip8->programCounter = START_ADDRESS;
  for (int i = 0; i < sizeof(sprite); i++) {
    chip8->memory[i] = sprite[i];
  }
  memset(chip8->display, 0, sizeof(chip8->display));
}

// Loads ROM from given path into Chip8
void loadROM(Chip8 *chip8, char *fileName) {
  FILE *rom = fopen(fileName, "rb");
  if (!rom) {
    printf("Error loading ROM: %s\n", fileName);
    exit(1);
  }
  fread(&chip8->memory[START_ADDRESS], 1, sizeof(chip8->memory) - START_ADDRESS,
        rom);
  fclose(rom);
}

// Clears the display
void opcode_00E0(Chip8 *chip8, int16_t opcode) {
  memset(chip8->display, 0, sizeof(chip8->display));
}

// Returns from subroutine
void opcode_00EE(Chip8 *chip8, int16_t opcode) {
  chip8->programCounter = chip8->stack[chip8->stackPointer];
  chip8->stackPointer--;
}

// Jump to location nnn
void opcode_1nnn(Chip8 *chip8, int16_t opcode) {
  uint16_t address = opcode & 0xFFF;
  chip8->programCounter = address;
}

// Call subroutine at nnn
void opcode_2nnn(Chip8 *chip8, int16_t opcode) {
  chip8->stackPointer++;
  chip8->stack[chip8->stackPointer] = chip8->programCounter;
  chip8->programCounter = opcode & 0xFFF;
}

// Skip next instruction if Vx == kk
void opcode_3xkk(Chip8 *chip8, int16_t opcode) {
  uint16_t x = (opcode >> 8) & 0xF;
  uint16_t kk = opcode & 0xFF;
  if (chip8->V[x] == kk) {
    chip8->programCounter += 2;
  }
}

// Skip next instruction if Vx != kk
void opcode_4xkk(Chip8 *chip8, int16_t opcode) {
  uint16_t x = (opcode >> 8) & 0xF;
  uint16_t kk = opcode & 0xFF;
  if (chip8->V[x] != kk) {
    chip8->programCounter += 2;
  }
}

// Skip next instruction if Vx == Vy
void opcode_5xy0(Chip8 *chip8, int16_t opcode) {
  uint16_t x = (opcode >> 8) & 0xF;
  uint16_t y = (opcode >> 4) & 0xF;
  if (chip8->V[x] == chip8->V[y]) {
    chip8->programCounter += 2;
  }
}

// Set Vx = kk
void opcode_6xkk(Chip8 *chip8, int16_t opcode) {
  uint16_t x = (opcode >> 8) & 0xF;
  uint16_t kk = opcode & 0xFF;
  chip8->V[x] = kk;
}

// Set Vx = Vx + kk
void opcode_7xkk(Chip8 *chip8, int16_t opcode) {
  uint16_t x = (opcode >> 8) & 0xF;
  uint16_t kk = opcode & 0xFF;
  chip8->V[x] += kk;
}

// Set Vx = Vy
void opcode_8xy0(Chip8 *chip8, int16_t opcode) {
  uint16_t x = (opcode >> 8) & 0xF;
  uint16_t y = (opcode >> 4) & 0xF;
  chip8->V[x] = chip8->V[y];
}

// Set Vx = Vx OR Vy
void opcode_8xy1(Chip8 *chip8, int16_t opcode) {
  uint16_t x = (opcode >> 8) & 0xF;
  uint16_t y = (opcode >> 4) & 0xF;
  chip8->V[x] |= chip8->V[y];
}

// Set Vx = Vx AND Vy
void opcode_8xy2(Chip8 *chip8, int16_t opcode) {
  uint16_t x = (opcode >> 8) & 0xF;
  uint16_t y = (opcode >> 4) & 0xF;
  chip8->V[x] &= chip8->V[y];
}

// Set Vx = Vx XOR Vy
void opcode_8xy3(Chip8 *chip8, int16_t opcode) {
  uint16_t x = (opcode >> 8) & 0xF;
  uint16_t y = (opcode >> 4) & 0xF;
  chip8->V[x] ^= chip8->V[y];
}

// Set Vx = Vx + Vy, set VF = carry
void opcode_8xy4(Chip8 *chip8, int16_t opcode) {
  uint16_t x = (opcode >> 8) & 0xF;
  uint16_t y = (opcode >> 4) & 0xF;
  if ((chip8->V[x] + chip8->V[y]) > 255) {
    chip8->V[0xF] = 1;
  } else
    chip8->V[0xF] = 0;
  chip8->V[x] += chip8->V[y];
}

// Set Vx = Vx - Vy, set VF = NOT borrow
void opcode_8xy5(Chip8 *chip8, int16_t opcode) {
  uint16_t x = (opcode >> 8) & 0xF;
  uint16_t y = (opcode >> 4) & 0xF;
  if (chip8->V[x] > chip8->V[y]) {
    chip8->V[0xF] = 1;
  } else
    chip8->V[0xF] = 0;
  chip8->V[x] = chip8->V[x] - chip8->V[y];
}

// Set Vx = Vx SHR 1
void opcode_8xy6(Chip8 *chip8, int16_t opcode) {
  uint16_t x = (opcode >> 8) & 0xF;
  if (chip8->V[x] & 1) {
    chip8->V[0xF] = 1;
  } else
    chip8->V[0xf] = 0;
  chip8->V[x] /= 2;
}

// Set Vx = Vy - Vx, set VF = NOT borrow
void opcode_8xy7(Chip8 *chip8, int16_t opcode) {
  uint16_t x = (opcode >> 8) & 0xF;
  uint16_t y = (opcode >> 4) & 0xF;
  if (chip8->V[y] > chip8->V[x]) {
    chip8->V[0xF] = 1;
  } else
    chip8->V[0xF] = 0;
  chip8->V[x] = chip8->V[y] - chip8->V[x];
}

// Set Vx = Vx SHL 1
void opcode_8xyE(Chip8 *chip8, int16_t opcode) {
  uint16_t x = (opcode >> 8) & 0xF;
  if ((chip8->V[x] >> 15) & 1) {
    chip8->V[0xF] = 1;
  } else
    chip8->V[0xf] = 0;
  chip8->V[x] *= 2;
}

// Skip next instruction if Vx != Vy
void opcode_9xy0(Chip8 *chip8, int16_t opcode) {
  uint16_t x = (opcode >> 8) & 0xF;
  uint16_t y = (opcode >> 4) & 0xF;
  if (chip8->V[x] != chip8->V[y]) {
    chip8->programCounter += 2;
  }
}

// Set I to nnn
void opcode_Annn(Chip8 *chip8, int16_t opcode) { chip8->I = opcode & 0x0FFF; }

// Jump to location nnn + V0
void opcode_Bnnn(Chip8 *chip8, int16_t opcode) {
  chip8->programCounter = (opcode & 0x0FFF) + chip8->V[0];
}

// Set Vx = random byte AND kk
void opcode_Cxkk(Chip8 *chip8, int16_t opcode) {
  srand(time(NULL));
  uint8_t r = rand() % 255;
  uint16_t x = (opcode >> 8) & 0xF;
  uint16_t kk = opcode & 0xFF;
  chip8->V[x] = r & kk;
}

// Display n-byte sprite starting at memory location I at (Vx, Vy), set VF =
// collision
void opcode_Dxyn(Chip8 *chip8, int16_t opcode) {
  uint8_t n = opcode & 0xF;
  uint16_t x = (opcode >> 8) & 0xF;
  uint16_t y = (opcode >> 4) & 0xF;
  uint8_t xCoord = chip8->V[x];
  uint8_t yCoord = chip8->V[y];

  chip8->V[0xF] = 0;
  for (int i = 0; i < n; i++) { // Each row
    uint8_t spriteRow = chip8->memory[chip8->I + i];
    for (int j = 0; j < 8; j++) { // Each bit
      uint8_t spriteBit = spriteRow & (0x80 >> j);
      uint8_t xBit = (xCoord + j) % 64;
      uint8_t yBit = (yCoord + i) % 32;
      if ((chip8->display[yBit][xBit] ^= spriteBit) == 0 && spriteBit != 0) {
        chip8->V[0xF] = 1;
      }
    }
  }
}

// Skip next instruction if key with value of Vx is pressed
void opcode_Ex9E(Chip8 *chip8, int16_t opcode) {
  uint8_t x = (opcode >> 8) & 0xF;
  uint8_t key = chip8->V[x];
  if (chip8->keyboard[key]) {
    chip8->programCounter += 2;
  }
}

// Skip next instruction if key with the value of Vx is not pressed
void opcode_ExA1(Chip8 *chip8, int16_t opcode) {
  uint8_t x = (opcode >> 8) & 0xF;
  uint8_t key = chip8->V[x];
  if (!chip8->keyboard[key]) {
    chip8->programCounter += 2;
  }
}

// Set Vx = delay timer value
void opcode_Fx07(Chip8 *chip8, int16_t opcode) {
  uint8_t x = (opcode >> 8) & 0xF;
  chip8->V[x] = chip8->delayTimer;
}

// Wait for a key press, store the value of the key in Vx
void opcode_Fx0A(Chip8 *chip8, int16_t opcode) {}

// Set delay timer = Vx
void opcode_Fx15(Chip8 *chip8, int16_t opcode) {
  uint8_t x = (opcode >> 8) & 0xF;
  chip8->delayTimer = chip8->V[x];
}

// Set sound timer = Vx
void opcode_Fx18(Chip8 *chip8, int16_t opcode) {
  uint8_t x = (opcode >> 8) & 0xF;
  chip8->soundTimer = chip8->V[x];
}

// Set I = I + Vx
void opcode_Fx1E(Chip8 *chip8, int16_t opcode) {
  uint8_t x = (opcode >> 8) & 0xF;
  chip8->I += chip8->V[x];
}

// Set I = location of sprite for digit Vx
void opcode_Fx29(Chip8 *chip8, int16_t opcode) {
  uint8_t x = (opcode >> 8) & 0xF;
  uint8_t sprite = chip8->V[x];
  chip8->I = 5 * sprite;
}

// Store BCD representation of Vx in memory location I, I + 1, and I + 2
void opcode_Fx33(Chip8 *chip8, int16_t opcode) {}

// Store register V0 through Vx in memory starting at location I
void opcode_Fx55(Chip8 *chip8, int16_t opcode) {
  uint8_t x = (opcode >> 8) & 0xF;
  for (int i = 0; i < x; i++) {
    chip8->memory[(chip8->I) + i] = chip8->V[i];
  }
}

// Read register V0 through Vx from memory starting at location I
void opcode_Fx65(Chip8 *chip8, int16_t opcode) {
  uint8_t x = (opcode >> 8) & 0xF;
  for (int i = 0; i < x; i++) {
    chip8->V[i] = chip8->memory[(chip8->I) + i];
  }
}

void test(Chip8 *chip8) {
  for (int i = 0; i < 32; i++) {
    for (int j = 0; j < 64; j++) {
      if (chip8->display[i][j] != 0) {
        printf("#");
      } else
        printf("-");
    }
    printf("\n");
  }
}

// Emulate a CPU cycle
void cycle(Chip8 *chip8) {
  // Fetch
  uint16_t opcode = (chip8->memory[chip8->programCounter] << 8) |
                    chip8->memory[(chip8->programCounter) + 1];
  chip8->programCounter += 2;
  // Decode
  chip8->V[0] = 0x03;
  chip8->V[1] = 0x04;
  chip8->I = 0x208;
  for (int i = 0; i < 5; i++) {
    chip8->memory[0x208 + i] = sprite[i + 5];
  }
  opcode_Dxyn(chip8, 0xD015);
  test(chip8);
  // Execute
}

int main(int argc, char *argv[]) {
  if (argc != 2) {
    printf("Error: Usage: ./emulator <ROM path>\n");
    exit(1);
  }

  Chip8 chip8;
  initChip(&chip8);
  loadROM(&chip8, argv[1]);

  SDL_Init(SDL_INIT_VIDEO);
  SDL_Window *window =
      SDL_CreateWindow("Chip-8 Emulator", 900, 640, SDL_WINDOW_RESIZABLE);
  SDL_Renderer *renderer = SDL_CreateRenderer(window, NULL);

  cycle(&chip8);

  bool running = true;
  while (running) {
    SDL_Event event;
    while (SDL_PollEvent(&event)) {
      switch (event.type) {
      case SDL_EVENT_QUIT:
        running = false;
        break;
      }
    }
  }


  SDL_DestroyRenderer(renderer);
  SDL_DestroyWindow(window);
  return 1;
}
