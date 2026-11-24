# alchem-c

> High-performance terminal falling-sand and cellular automata chemical reaction engine in pure C99.

![alchem-c Showcase](assets/demo.gif)

[![CI](https://github.com/omchandra0907-jpg/alchem-c/actions/workflows/ci.yml/badge.svg)](https://github.com/omchandra0907-jpg/alchem-c/actions/workflows/ci.yml)
[![License: MIT](https://img.shields.io/badge/License-MIT-yellow.svg)](https://opensource.org/licenses/MIT)
[![C99](https://img.shields.io/badge/C-C99-blue.svg)](https://en.wikipedia.org/wiki/C99)

> **Live Interactive Web Demo:** [https://omchandra0907-jpg.github.io/alchem-c/](https://omchandra0907-jpg.github.io/alchem-c/)

---

## Architectural Highlights

- **Cache-Friendly 1D Grid Memory**: The entire simulation grid is laid out in a single contiguous `cell_t *cells` array (`width * height`) rather than fragmented pointer arrays (`cell_t **`), providing peak spatial cache locality and single-syscall cleanup.
- **Bit-Packed 4-Byte Cell Struct**:
  ```c
  typedef struct {
      uint8_t type;   /* Element identifier (Air, Sand, Water, Lava, etc.) */
      uint8_t clock;  /* Parity bit flipped per tick to prevent double-updating */
      uint8_t life;   /* Burn/decay/dissolve countdown counter */
      uint8_t tint;   /* Organic color jitter (0..15) for natural texture */
  } cell_t;
  ```
- **Unbiased Symmetrical Physics**: Bottom-to-top evaluation (`height - 1` down to 0) with alternating horizontal scan directions (`x` sweep oscillates left-to-right and right-to-left based on `(y + clock) % 2`), eliminating directional drift in sand mounds and fluid dispersal.
- **Single-Syscall Flicker-Free ANSI TrueColor Rendering**: Packs two vertical cells per terminal character using the Unicode upper half-block (`▀`) with 24-bit ANSI background and foreground escape codes. The entire frame buffer is composed in memory and flushed via one atomic `write(STDOUT_FILENO, ...)` syscall per tick.
- **Interactive Mouse & Keyboard Input**: Supports terminal SGR 1006 mouse drag painting (`\033[?1006h`) and hotkeys for element selection, pause/resume, and canvas reset.

---

## Chemical Reaction Matrix

| Reactant A | Reactant B | Resulting Product | Mechanism |
|---|---|---|---|
| **LAVA** | **WATER** | **STONE + STEAM** | Rapid thermal exchange; water boils into steam, lava vitrifies into stone. |
| **FIRE** | **GUNPOWDER** | **FIRE (Radial Shockwave)** | High-volatility combustion triggers a radial explosion clearing surrounding cells. |
| **ACID** | **SAND / STONE / GUNPOWDER** | **EMPTY (Dissolved)** | Corrosive breakdown decrements particle life until substrate is eradicated. |
| **WATER** | **SAND** | **SAND (Displaced)** | Density differentiation: heavy sand sinks through water columns. |
| **FIRE** | **WATER** | **STEAM** | Immediate phase transition into buoyant vapor. |

---

## Quickstart

### Build Requirements
- GCC or Clang (C99 standard support)
- GNU Make
- POSIX-compliant terminal with 24-bit TrueColor and UTF-8 support

### Build & Run
```bash
git clone https://github.com/omchandra0907-jpg/alchem-c.git
cd alchem-c

# Build release binary (-O2)
make

# Run the simulation
./alchem-c
```

### Controls & Keybindings

| Key / Mouse Action | Function |
|---|---|
| **Left Click + Drag** | Paint with currently selected element |
| **Right Click + Drag** | Erase particles (Air / Empty) |
| **Keys `1` – `8`** | Select brush: `1` Sand, `2` Water, `3` Lava, `4` Acid, `5` Gunpowder, `6` Fire, `7` Stone, `8` Steam |
| **Space** | Pause / resume physics simulation |
| **Key `c`** | Clear entire simulation grid |
| **Key `q` / ESC** | Exit cleanly and restore terminal modes |

---

## Verification & Sanity Checks

Build and execute the headless verification suite with AddressSanitizer and UndefinedBehaviorSanitizer enabled:

```bash
make test
```

---

## Technical Specifications

| Parameter | Specification |
|---|---|
| **Language** | ISO C99 (`-std=c99 -pedantic`) |
| **Cell Size** | Exactly 4 bytes (`sizeof(cell_t) == 4`) |
| **Frame Rate** | Locked 60 FPS via `clock_gettime(CLOCK_MONOTONIC)` |
| **Memory Footprint** | ~76.8 KB grid buffer for standard 160x120 terminal resolution |
| **Sanitizers** | Compiles clean under `-fsanitize=address,undefined` |

---

## License

Distributed under the MIT License. Developed by [Om Chandra](https://github.com/omchandra0907-jpg).
