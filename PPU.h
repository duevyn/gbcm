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

	// Sprite properties (Y, X, Tile, Flags).
	// Mapped: 0xFE00 - 0xFE9F.
	// Locked: Cannot be accessed by CPU during Mode 2 or 3.
	uint8_t oam[160];

	uint8_t lcdc; // 0xFF40: LCD Control (Enable, Bg Map select, etc.)
	uint8_t stat; // 0xFF41: LCD Status (Modes, Interrupt selection)
	uint8_t scy; // 0xFF42: Scroll Y
	uint8_t scx; // 0xFF43: Scroll X
	uint8_t ly; // 0xFF44: LCD Y Coordinate (Current Scanline)
	uint8_t lyc; // 0xFF45: LY Compare (Interrupt trigger)
	uint8_t dma;
	uint8_t bgp; // 0xFF47: BG Palette
	uint8_t obp0; // 0xFF48: Object Palette 0
	uint8_t obp1; // 0xFF49: Object Palette 1
	uint8_t wy; // 0xFF4A: Window Y
	uint8_t wx; // 0xFF4B: Window X

	int ly_dots; // Tracks 0-455 dots within a scanline
	int window_line_counter; // Internal counter for Window rendering logic

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
