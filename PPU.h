#ifndef PPU_H
#define PPU_H

#include <stdint.h>
#include <stdbool.h>

#define CLOCK 4194304
#define DOTS_PER_FRAME 70224.0
#define NS_PER_FRAME (1.0E9 * DOTS_PER_FRAME / CLOCK)

#define OAM_DOTS 80
#define DRAW_DOTS_MIN 172
#define SCANLINE_DOTS 456
#define LY_VBLNK_FST 144
#define LY_VBLNK_LST 153

struct GameBoy;

enum ppu_mode {
	HBLNK,
	VBLNK,
	OAM,
	DRAW,
};

typedef struct PPU {
	uint8_t vram[8192];

	uint8_t oam[160];
	int ly_dots;
	int window_ly;

	enum ppu_mode mode;

	uint8_t md3delay;
	uint32_t framebuffer[160 * 144];
	bool skip_frame, done_frame;

} PPU;

void ppu_step(struct GameBoy *gb, int dots);

#endif
