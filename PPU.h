#ifndef PPU_H
#define PPU_H

#include <stdint.h>

#define CLOCK 4194304
#define DOTS_PER_FRAME 70224.0f
#define NS_PER_FRAME (1000000000 * DOTS_PER_FRAME / CLOCK)

#define OAM_DOTS 80
#define DRAW_DOTS_MIN 172
#define SCANLINE_DOTS 456
#define LY_VBLNK_FST 144
#define LY_VBLNK_LST 153

struct GameBoy;

typedef struct PPU {
	// Mapped: 0x8000 - 0x9FFF.
	// Locked: Cannot be accessed by CPU during Mode 3.
	uint8_t vram[8192];

	// Locked: Cannot be accessed by CPU during Mode 2 or 3.
	uint8_t oam[160];
	int ly_dots;
	int window_line_counter;

	enum {
		HBLNK,
		VBLNK,
		OAM,
		DRAW,
	} mode;

	uint8_t md3delay; // mode 3 delay
	uint32_t framebuffer[160 * 144];

} PPU;

void ppu_step(struct GameBoy *gb, int dots);

#endif
