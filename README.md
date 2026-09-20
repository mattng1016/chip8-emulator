# CHIP-8 Emulator

A CHIP-8 Emulator written with C and SDL3. Built based on the [Cowgod CHIP-8 Technical Reference](http://devernay.free.fr/hacks/chip8/C8TECH10.HTM) rather than an existing implementation, with a 11 35 opcodes implemented via a function pointer dispatch table.

<table>
  <tr>
    <td><img width="400" alt="demo 1" src="https://github.com/user-attachments/assets/61e4155f-1b78-4181-a3e3-d2545dba4b30" /></td>
    <td><img width="400" alt="demo 2" src="https://github.com/user-attachments/assets/c8120991-aaca-4fbb-bce3-734b2ae47def" /></td>
  </tr>
  <tr>
    <td align="center">Space Invaders</td>
    <td align="center">Tetris</td>
  </tr>
</table>


## Features
 
- Full CHIP-8 instruction set (35 opcodes), dispatched through a two-level function-pointer table (top-level table keyed on the opcode's first nibble, secondary tables for the `0x0`, `0x8`, `0xE`, `0xF` families that need a second nibble to disambiguate)
- SDL3 rendering, scaled from the native 64x32 display to a real window
- Full keyboard input mapped to the standard CHIP-8 hex keypad layout
- Timing-correct execution: CPU cycles run at ~540Hz, independent of a separate 60Hz timer/render loop
- Delay and sound timer support
## Build & Run
 
```bash
mkdir build && cd build
cmake ..
cmake --build .
./chip8 <path-to-rom>
```
 
Requires SDL3 (`brew install sdl3` on macOS).
## Controls
 
Mapped to the standard CHIP-8 keypad layout:
 
```
1 2 3 C          1 2 3 4
4 5 6 D    ->    Q W E R
7 8 9 E          A S D F
A 0 B F          Z X C V
```
 
## Architecture
 
The execution pipeline, roughly:
 
```
main loop
  -> poll SDL events (keyboard, quit)
  -> cycle() [throttled to ~540Hz]
       -> fetch: read 2 bytes at PC, combine into a 16-bit opcode
       -> decode/execute: index into the function-pointer table by
          the opcode's first nibble, dispatch (with a second-level
          table lookup for ambiguous families)
  -> timer/render tick [throttled to ~60Hz]
       -> decrement delay/sound timers
       -> render the display buffer via SDL
```
## ROMS and test suites

- [ROM games](https://github.com/kripod/chip8-roms) used for testing and playing.
- [Test Suites](https://github.com/kripod/chip8-roms) used for debugging.

## License
 
MIT
