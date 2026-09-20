#include <SDL3/SDL.h>
#include <SDL3/SDL_main.h>

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#define START_ADDRESS 0x200
#define CYCLE_INTERVAL (1000.0 / 540.0)
#define TIMER_INTERVAL (1000.0 / 60.0)

static SDL_Window *window = NULL;
static SDL_Renderer *renderer = NULL;

typedef struct {
  uint8_t memory[4096];
  uint8_t V[16];
  uint8_t keyboard[16];
  uint32_t display[32][64];
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

// Function pointer stuff
typedef void (*opcodeFunction)(Chip8 *chip8, uint16_t opcode);
opcodeFunction table[16];
opcodeFunction table0[0x100];
opcodeFunction table8[16];
opcodeFunction tableE[0x100];
opcodeFunction tableF[0x100];

// Initialize chip
void initChip(Chip8 *chip8) {
  memset(chip8, 0, sizeof(Chip8));

  chip8->programCounter = START_ADDRESS;
  for (int i = 0; i < sizeof(sprite); i++) {
    chip8->memory[i] = sprite[i];
  }
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
void opcode_00E0(Chip8 *chip8, uint16_t opcode) {
  memset(chip8->display, 0, sizeof(chip8->display));
}

// Returns from subroutine
void opcode_00EE(Chip8 *chip8, uint16_t opcode) {
  chip8->programCounter = chip8->stack[chip8->stackPointer];
  chip8->stackPointer--;
}

// Jump to location nnn
void opcode_1nnn(Chip8 *chip8, uint16_t opcode) {
  uint16_t address = opcode & 0xFFF;
  chip8->programCounter = address;
}

// Call subroutine at nnn
void opcode_2nnn(Chip8 *chip8, uint16_t opcode) {
  chip8->stackPointer++;
  chip8->stack[chip8->stackPointer] = chip8->programCounter;
  chip8->programCounter = opcode & 0xFFF;
}

// Skip next instruction if Vx == kk
void opcode_3xkk(Chip8 *chip8, uint16_t opcode) {
  uint16_t x = (opcode >> 8) & 0xF;
  uint16_t kk = opcode & 0xFF;
  if (chip8->V[x] == kk) {
    chip8->programCounter += 2;
  }
}

// Skip next instruction if Vx != kk
void opcode_4xkk(Chip8 *chip8, uint16_t opcode) {
  uint16_t x = (opcode >> 8) & 0xF;
  uint16_t kk = opcode & 0xFF;
  if (chip8->V[x] != kk) {
    chip8->programCounter += 2;
  }
}

// Skip next instruction if Vx == Vy
void opcode_5xy0(Chip8 *chip8, uint16_t opcode) {
  uint16_t x = (opcode >> 8) & 0xF;
  uint16_t y = (opcode >> 4) & 0xF;
  if (chip8->V[x] == chip8->V[y]) {
    chip8->programCounter += 2;
  }
}

// Set Vx = kk
void opcode_6xkk(Chip8 *chip8, uint16_t opcode) {
  uint16_t x = (opcode >> 8) & 0xF;
  uint16_t kk = opcode & 0xFF;
  chip8->V[x] = kk;
}

// Set Vx = Vx + kk
void opcode_7xkk(Chip8 *chip8, uint16_t opcode) {
  uint16_t x = (opcode >> 8) & 0xF;
  uint16_t kk = opcode & 0xFF;
  chip8->V[x] += kk;
}

// Set Vx = Vy
void opcode_8xy0(Chip8 *chip8, uint16_t opcode) {
  uint16_t x = (opcode >> 8) & 0xF;
  uint16_t y = (opcode >> 4) & 0xF;
  chip8->V[x] = chip8->V[y];
}

// Set Vx = Vx OR Vy
void opcode_8xy1(Chip8 *chip8, uint16_t opcode) {
  uint16_t x = (opcode >> 8) & 0xF;
  uint16_t y = (opcode >> 4) & 0xF;
  chip8->V[x] |= chip8->V[y];
  chip8->V[0xF] = 0;
}

// Set Vx = Vx AND Vy
void opcode_8xy2(Chip8 *chip8, uint16_t opcode) {
  uint16_t x = (opcode >> 8) & 0xF;
  uint16_t y = (opcode >> 4) & 0xF;
  chip8->V[x] &= chip8->V[y];
  chip8->V[0xF] = 0;
}

// Set Vx = Vx XOR Vy
void opcode_8xy3(Chip8 *chip8, uint16_t opcode) {
  uint16_t x = (opcode >> 8) & 0xF;
  uint16_t y = (opcode >> 4) & 0xF;
  chip8->V[x] ^= chip8->V[y];
  chip8->V[0xF] = 0;
}

// Set Vx = Vx + Vy, set VF = carry
void opcode_8xy4(Chip8 *chip8, uint16_t opcode) {
  uint16_t x = (opcode >> 8) & 0xF;
  uint16_t y = (opcode >> 4) & 0xF;
  uint16_t temp = chip8->V[x] + chip8->V[y];
  chip8->V[x] = temp & 0xFF;
  if (temp > 255) {
    chip8->V[0xF] = 1;
  } else
    chip8->V[0xF] = 0;
}

// Set Vx = Vx - Vy, set VF = NOT borrow
void opcode_8xy5(Chip8 *chip8, uint16_t opcode) {
  uint16_t x = (opcode >> 8) & 0xF;
  uint16_t y = (opcode >> 4) & 0xF;
  uint16_t temp = chip8->V[x] - chip8->V[y];
  chip8->V[x] = temp;
  if (chip8->V[x] >= chip8->V[y]) {
    chip8->V[0xF] = 1;
  } else
    chip8->V[0xF] = 0;
}

// Set Vx = Vx SHR 1
void opcode_8xy6(Chip8 *chip8, uint16_t opcode) {
  uint16_t x = (opcode >> 8) & 0xF;
  if (chip8->V[x] & 1) {
    chip8->V[0xF] = 1;
  } else
    chip8->V[0xF] = 0;
  chip8->V[x] /= 2;
}

// Set Vx = Vy - Vx, set VF = NOT borrow
void opcode_8xy7(Chip8 *chip8, uint16_t opcode) {
  uint16_t x = (opcode >> 8) & 0xF;
  uint16_t y = (opcode >> 4) & 0xF;
  uint16_t temp = chip8->V[y] - chip8->V[x];
  chip8->V[x] = temp;
  if (temp >= 0) {
    chip8->V[0xF] = 1;
  } else
    chip8->V[0xF] = 0;
}

// Set Vx = Vx SHL 1
void opcode_8xyE(Chip8 *chip8, uint16_t opcode) {
  uint16_t x = (opcode >> 8) & 0xF;
  chip8->V[0xF] = (chip8->V[x] & 0x80) >> 7;
  chip8->V[x] <<= 1;
}

// Skip next instruction if Vx != Vy
void opcode_9xy0(Chip8 *chip8, uint16_t opcode) {
  uint16_t x = (opcode >> 8) & 0xF;
  uint16_t y = (opcode >> 4) & 0xF;
  if (chip8->V[x] != chip8->V[y]) {
    chip8->programCounter += 2;
  }
}

// Set I to nnn
void opcode_Annn(Chip8 *chip8, uint16_t opcode) { chip8->I = opcode & 0x0FFF; }

// Jump to location nnn + V0
void opcode_Bnnn(Chip8 *chip8, uint16_t opcode) {
  chip8->programCounter = (opcode & 0x0FFF) + chip8->V[0];
}

// Set Vx = random byte AND kk
void opcode_Cxkk(Chip8 *chip8, uint16_t opcode) {
  srand(time(NULL));
  uint8_t r = rand() % 255;
  uint16_t x = (opcode >> 8) & 0xF;
  uint16_t kk = opcode & 0xFF;
  chip8->V[x] = r & kk;
}

// Display n-byte sprite starting at memory location I at (Vx, Vy), set VF =
// collision
void opcode_Dxyn(Chip8 *chip8, uint16_t opcode) {
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
void opcode_Ex9E(Chip8 *chip8, uint16_t opcode) {
  uint8_t x = (opcode >> 8) & 0xF;
  uint8_t key = chip8->V[x];
  if (chip8->keyboard[key]) {
    chip8->programCounter += 2;
  }
}

// Skip next instruction if key with the value of Vx is not pressed
void opcode_ExA1(Chip8 *chip8, uint16_t opcode) {
  uint8_t x = (opcode >> 8) & 0xF;
  uint8_t key = chip8->V[x];
  if (!chip8->keyboard[key]) {
    chip8->programCounter += 2;
  }
}

// Set Vx = delay timer value
void opcode_Fx07(Chip8 *chip8, uint16_t opcode) {
  uint8_t x = (opcode >> 8) & 0xF;
  chip8->V[x] = chip8->delayTimer;
}

// Wait for a key press, store the value of the key in Vx
void opcode_Fx0A(Chip8 *chip8, uint16_t opcode) {
  uint8_t x = (opcode >> 8) & 0xF;
  for (int i = 0; i <= 0xF; i++) {
    if (chip8->keyboard[i]) {
      chip8->V[x] = i;
      return;
    }
  }
  chip8->programCounter -= 2;
}

// Set delay timer = Vx
void opcode_Fx15(Chip8 *chip8, uint16_t opcode) {
  uint8_t x = (opcode >> 8) & 0xF;
  chip8->delayTimer = chip8->V[x];
}

// Set sound timer = Vx
void opcode_Fx18(Chip8 *chip8, uint16_t opcode) {
  uint8_t x = (opcode >> 8) & 0xF;
  chip8->soundTimer = chip8->V[x];
}

// Set I = I + Vx
void opcode_Fx1E(Chip8 *chip8, uint16_t opcode) {
  uint8_t x = (opcode >> 8) & 0xF;
  chip8->I += chip8->V[x];
}

// Set I = location of sprite for digit Vx
void opcode_Fx29(Chip8 *chip8, uint16_t opcode) {
  uint8_t x = (opcode >> 8) & 0xF;
  uint8_t sprite = chip8->V[x];
  chip8->I = 5 * sprite;
}

// Store BCD representation of Vx in memory location I, I + 1, and I + 2
void opcode_Fx33(Chip8 *chip8, uint16_t opcode) {
  uint8_t x = (opcode >> 8) & 0xF;
  uint8_t value = chip8->V[x];
  chip8->memory[chip8->I + 2] = value % 10; // Single digit
  value /= 10;
  chip8->memory[chip8->I + 1] = value % 10; // Ten digit
  value /= 10;
  chip8->memory[chip8->I] = value % 10; // Hundred digit
}

// Store register V0 through Vx in memory starting at location I
void opcode_Fx55(Chip8 *chip8, uint16_t opcode) {
  uint8_t x = (opcode >> 8) & 0xF;
  for (int i = 0; i <= x; i++) {
    chip8->memory[(chip8->I) + i] = chip8->V[i];
  }
  chip8->I += x + 1;
}

// Read register V0 through Vx from memory starting at location I
void opcode_Fx65(Chip8 *chip8, uint16_t opcode) {
  uint8_t x = (opcode >> 8) & 0xF;
  for (int i = 0; i <= x; i++) {
    chip8->V[i] = chip8->memory[(chip8->I) + i];
  }
  chip8->I += x + 1;
}

// Error
void opcode_err(Chip8 *chip8, uint16_t opcode) {
  printf("Error opcode: 0x%04x\n", opcode);
}

// Function dispatchers
void d_0(Chip8 *chip8, uint16_t opcode) {
  table0[opcode & 0x00FF](chip8, opcode);
}

void d_8(Chip8 *chip8, uint16_t opcode) {
  table8[opcode & 0x000F](chip8, opcode);
}

void d_E(Chip8 *chip8, uint16_t opcode) {
  tableE[opcode & 0x00FF](chip8, opcode);
}

void d_F(Chip8 *chip8, uint16_t opcode) {
  tableF[opcode & 0x00FF](chip8, opcode);
}

// Initialize function table
void initTables(void) {
  for (int i = 0; i < 16; i++) {
    table[i] = opcode_err;
  }
  table[0x0] = d_0;
  table[0x1] = opcode_1nnn;
  table[0x2] = opcode_2nnn;
  table[0x3] = opcode_3xkk;
  table[0x4] = opcode_4xkk;
  table[0x5] = opcode_5xy0;
  table[0x6] = opcode_6xkk;
  table[0x7] = opcode_7xkk;
  table[0x8] = d_8;
  table[0x9] = opcode_9xy0;
  table[0xA] = opcode_Annn;
  table[0xB] = opcode_Bnnn;
  table[0xC] = opcode_Cxkk;
  table[0xD] = opcode_Dxyn;
  table[0xE] = d_E;
  table[0xF] = d_F;

  for (int i = 0; i < 0x100; i++) {
    table0[i] = opcode_err;
  }
  table0[0xE0] = opcode_00E0;
  table0[0xEE] = opcode_00EE;

  for (int i = 0; i < 16; i++) {
    table8[i] = opcode_err;
  }
  table8[0x0] = opcode_8xy0;
  table8[0x1] = opcode_8xy1;
  table8[0x2] = opcode_8xy2;
  table8[0x3] = opcode_8xy3;
  table8[0x4] = opcode_8xy4;
  table8[0x5] = opcode_8xy5;
  table8[0x6] = opcode_8xy6;
  table8[0x7] = opcode_8xy7;
  table8[0xE] = opcode_8xyE;

  for (int i = 0; i < 0x100; i++) {
    tableE[i] = opcode_err;
  }
  tableE[0x9E] = opcode_Ex9E;
  tableE[0xA1] = opcode_ExA1;

  for (int i = 0; i < 0x100; i++) {
    tableF[i] = opcode_err;
  }
  tableF[0x07] = opcode_Fx07;
  tableF[0x0A] = opcode_Fx0A;
  tableF[0x15] = opcode_Fx15;
  tableF[0x18] = opcode_Fx18;
  tableF[0x1E] = opcode_Fx1E;
  tableF[0x29] = opcode_Fx29;
  tableF[0x33] = opcode_Fx33;
  tableF[0x55] = opcode_Fx55;
  tableF[0x65] = opcode_Fx65;
}

// Test
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
  // Decode and execute
  printf("PC: 0x%04x -> opcode: 0x%04x\n", chip8->programCounter - 2, opcode);
  uint8_t f = (opcode & 0xF000) >> 12;
  table[f](chip8, opcode);
}

// Renders renderer based on display array
void renderScreen(Chip8 *chip8, SDL_Renderer *renderer) {
  SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
  SDL_RenderClear(renderer);

  SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255);
  int scaleX = 900 / 64;
  int scaleY = 640 / 32;
  for (int i = 0; i < 32; i++) {
    for (int j = 0; j < 64; j++) {
      if (chip8->display[i][j] != 0) {
        SDL_FRect rect = {j * scaleX, i * scaleY, scaleX, scaleY};
        SDL_RenderFillRect(renderer, &rect);
      }
    }
  }
  SDL_RenderPresent(renderer);
}

int main(int argc, char *argv[]) {
  if (argc != 2) {
    printf("Error: Usage: ./emulator <ROM path>\n");
    exit(1);
  }

  Chip8 chip8;
  initChip(&chip8);
  initTables();
  loadROM(&chip8, argv[1]);
  SDL_Init(SDL_INIT_VIDEO);
  SDL_Window *window =
      SDL_CreateWindow("Chip-8 Emulator", 900, 640, SDL_WINDOW_RESIZABLE);
  SDL_Renderer *renderer = SDL_CreateRenderer(window, NULL);

  double lastCycleTime = SDL_GetTicks();
  double lastTimeTimer = SDL_GetTicks();

  bool running = true;
  while (running) {
    SDL_Event event;
    while (SDL_PollEvent(&event)) {
      switch (event.type) {
      case SDL_EVENT_QUIT:
        running = false;
        break;
      case SDL_EVENT_KEY_DOWN:
        switch (event.key.key) {
        case SDLK_1:
          chip8.keyboard[1] = 1;
          break;
        case SDLK_2:
          chip8.keyboard[2] = 1;
          break;
        case SDLK_3:
          chip8.keyboard[3] = 1;
          break;
        case SDLK_4:
          chip8.keyboard[0xC] = 1;
          break;
        case SDLK_Q:
          chip8.keyboard[4] = 1;
          break;
        case SDLK_W:
          chip8.keyboard[5] = 1;
          break;
        case SDLK_E:
          chip8.keyboard[6] = 1;
          break;
        case SDLK_R:
          chip8.keyboard[0xD] = 1;
          break;
        case SDLK_A:
          chip8.keyboard[7] = 1;
          break;
        case SDLK_S:
          chip8.keyboard[8] = 1;
          break;
        case SDLK_D:
          chip8.keyboard[9] = 1;
          break;
        case SDLK_F:
          chip8.keyboard[0xE] = 1;
          break;
        case SDLK_Z:
          chip8.keyboard[0xA] = 1;
          break;
        case SDLK_X:
          chip8.keyboard[0] = 1;
          break;
        case SDLK_C:
          chip8.keyboard[0xB] = 1;
          break;
        case SDLK_V:
          chip8.keyboard[0xF] = 1;
          break;
        case SDLK_ESCAPE:
          running = false;
        }
        break;
      case SDL_EVENT_KEY_UP:
        switch (event.key.key) {
        case SDLK_1:
          chip8.keyboard[1] = 0;
          break;
        case SDLK_2:
          chip8.keyboard[2] = 0;
          break;
        case SDLK_3:
          chip8.keyboard[3] = 0;
          break;
        case SDLK_4:
          chip8.keyboard[0xC] = 0;
          break;
        case SDLK_Q:
          chip8.keyboard[4] = 0;
          break;
        case SDLK_W:
          chip8.keyboard[5] = 0;
          break;
        case SDLK_E:
          chip8.keyboard[6] = 0;
          break;
        case SDLK_R:
          chip8.keyboard[0xD] = 0;
          break;
        case SDLK_A:
          chip8.keyboard[7] = 0;
          break;
        case SDLK_S:
          chip8.keyboard[8] = 0;
          break;
        case SDLK_D:
          chip8.keyboard[9] = 0;
          break;
        case SDLK_F:
          chip8.keyboard[0xE] = 0;
          break;
        case SDLK_Z:
          chip8.keyboard[0xA] = 0;
          break;
        case SDLK_X:
          chip8.keyboard[0] = 0;
          break;
        case SDLK_C:
          chip8.keyboard[0xB] = 0;
          break;
        case SDLK_V:
          chip8.keyboard[0xF] = 0;
          break;
        }
        break;
      }
    }

    double currentTime = SDL_GetTicks();

    if (currentTime - lastCycleTime >= CYCLE_INTERVAL) {
      cycle(&chip8);
      lastCycleTime = currentTime;
    }

    if (currentTime - lastTimeTimer >= TIMER_INTERVAL) {
      if (chip8.delayTimer > 0)
        chip8.delayTimer--;
      if (chip8.soundTimer > 0)
        chip8.soundTimer--;
      renderScreen(&chip8, renderer);
      lastTimeTimer = currentTime;
    }
  }

  SDL_DestroyRenderer(renderer);
  SDL_DestroyWindow(window);
  return 1;
}
