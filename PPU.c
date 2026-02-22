#include "PPU.h"
#include "GameBoy.h"
#include "logger.h"
#include "io.h"
#include <stdio.h>

//#define fprintf(stderr, ...) ((void)0)

const char *const ModeNames[] = {
	"H Blank",
	"V Blank",
	"OAM",
	"DRAW",
};

void update_stat()
{
}

void check_lyc_eq_ly(struct GameBoy *gb)
{
}

static const uint32_t colors[4] = {
	0xFFE0F8D0, 0xFF88C070, 0xFF346856, 0xFF081820,
	//0xFF9BBC0F,
	//0xFF8BAC0F,
	//0xFF306230,
	//0xFF0F380F
};

void render_scanline(struct PPU *ppu)
{
	uint8_t hi_byte, lo_byte, hi_bit, lo_bit, color;

	uint8_t line = io_map[LY];
	uint16_t tl_offset = (line / 8) * 32 + 0x1800;
	uint16_t px_ind = 160 * line;
	uint8_t tl_row = line % 8;

	for (int i = 0; i < 20; i++) {
		uint16_t tl_id = ppu->vram[tl_offset + i];
		uint16_t tl_ind = tl_id * 16 + tl_row * 2;
		lo_byte = ppu->vram[tl_ind];
		hi_byte = ppu->vram[tl_ind + 1];

		for (int bit = 7; bit >= 0; bit--) {
			lo_bit = (lo_byte >> bit) & 1;
			hi_bit = (hi_byte >> bit) & 1;
			color = (hi_bit << 1) | lo_bit;
			uint8_t shade = (io_map[BGP] >> (color * 2)) & 0x03;
			ppu->framebuffer[px_ind++] = colors[color];
		}
	}
}

void ppu_step(struct GameBoy *gb, int dots)
{
	if ((io_map[LCDC] & 0x80) == 0) {
		fprintf(stderr, "\n");
		return;
	}

	gb->ppu.ly_dots += dots;
	uint8_t prev_ln = io_map[LY];
	switch (gb->ppu.mode) {
	case (OAM): //mode 2
		if (gb->ppu.ly_dots >= OAM_DOTS) {
			gb->ppu.mode = DRAW;
			gb->ppu.md3delay = 0; //TODO: calc mode 3 delay
			update_stat();
		}
		break;
	case (DRAW): //mode 3
		if (gb->ppu.ly_dots >=
		    OAM_DOTS + DRAW_DOTS_MIN + gb->ppu.md3delay) {
			gb->ppu.mode = HBLNK;
			update_stat();
			render_scanline(&gb->ppu);
		}
		break;
	case (HBLNK): // mode 0
		if (gb->ppu.ly_dots >= SCANLINE_DOTS) {
			gb->ppu.ly_dots -= SCANLINE_DOTS;
			io_map[LY]++;
			check_lyc_eq_ly(gb);

			if (io_map[LY] < LY_VBLNK_FST) {
				gb->ppu.mode = OAM;
				update_stat();
			} else {
				gb->ppu.mode = VBLNK;
				io_req_interrupt(IO_VBLANK);
			}
		}
		break;
	case (VBLNK): // mode 1
		if (gb->ppu.ly_dots >= SCANLINE_DOTS) {
			gb->ppu.ly_dots -= SCANLINE_DOTS;
			io_map[LY]++;

			if (io_map[LY] > LY_VBLNK_LST) {
				gb->ppu.mode = OAM;
				io_map[LY] = 0;
			}

			check_lyc_eq_ly(gb);
			update_stat();
		}
		break;
	}

	fprintf(stderr, "--- %s (ly=%d: ly_dots=%d, lyc=%d)\n",
		ModeNames[gb->ppu.mode], io_map[LY], gb->ppu.ly_dots,
		io_map[LYC]);

	if (prev_ln != io_map[LY])
		fprintf(stderr, "\n\n");

	return;
}
