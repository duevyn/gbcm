#include "PPU.h"
#include "GameBoy.h"
#include "logger.h"
#include "io.h"
#include <stdio.h>

#define fprintf(stderr, ...) ((void)0)

const char *const ModeNames[] = {
	"H Blank",
	"V Blank",
	"OAM",
	"DRAW",
};

static const uint32_t ppu_colors[4] = {
	0xFFE0F8D0, 0xFF88C070, 0xFF346856, 0xFF081820,
	//0xFF9BBC0F, 0xFF8BAC0F, 0xFF306230, 0xFF0F380F
};

static uint8_t bg_line_colors[160];

void update_stat(struct GameBoy *gb)
{
	io_map[STAT] &= ~0x03;
	io_map[STAT] |= (gb->ppu.mode & 0x03);
}

void check_lyc_eq_ly(struct GameBoy *gb)
{
	fprintf(stderr, "\n\n");
	if (io_map[LY] == io_map[LYC]) {
		io_map[STAT] |= 0x04;
		if (io_map[STAT] & 0x40)
			io_req_interrupt(IO_STAT);
		return;
	}
	io_map[STAT] &= ~0x04;
}

static inline void render_bg_wndw(struct PPU *ppu)
{
	bool sign_addr_md;
	uint8_t tl_id, hi_byte, lo_byte, hi_bit, lo_bit, bit, color, shade;
	uint16_t tl_ind, by_ind;

	uint16_t bg_tl_mp = (0x08 & io_map[LCDC]) ? 0x1C00 : 0x1800;
	uint16_t bg_map_addr = (io_map[LY] / 8) * 32 + bg_tl_mp;
	uint8_t tl_row_offset = (io_map[LY] % 8) * 2;

	// It is Ok to refetch tile 8 times instead of holding tile and
	// complicating logic because emulation loop is typcially under
	// 0.5 ms and we render once ever 16.742 seconds.
	for (int x = 0; x < 160; x++) {
		tl_id = ppu->vram[bg_map_addr + (x / 8)];
		by_ind = tl_id * 16 + tl_row_offset;

		sign_addr_md = !(0x10 & io_map[LCDC]);
		if (sign_addr_md && (tl_id <= 127))
			by_ind += 0x1000;

		lo_byte = ppu->vram[by_ind];
		hi_byte = ppu->vram[by_ind + 1];
		bit = (7 - (x % 8));
		lo_bit = (lo_byte >> bit) & 1;
		hi_bit = (hi_byte >> bit) & 1;

		color = (hi_bit << 1) | lo_bit;
		bg_line_colors[x] = color;
		shade = (io_map[BGP] >> (color * 2)) & 0x03;
		ppu->framebuffer[io_map[LY] * 160 + x] = ppu_colors[shade];
	}
}

static inline void render_sprites(struct PPU *ppu)
{
	if (!(io_map[LCDC] & 0x02))
		return;

	uint8_t hi_byte, lo_byte, hi_bit, lo_bit, color;
	uint8_t line = io_map[LY];
	int8_t height = (io_map[LCDC] & 0x04) ? 16 : 8;

	uint8_t count = 0;
	for (int i = 0; i < 40 && count < 10; i++) {
		uint8_t y_pos = ppu->oam[i * 4];
		uint8_t x_pos = ppu->oam[i * 4 + 1];
		uint8_t tile_idx = ppu->oam[i * 4 + 2];
		uint8_t flags = ppu->oam[i * 4 + 3];

		uint8_t sprite_top = y_pos - 16;
		if (line < sprite_top || ((line >= sprite_top + height)))
			continue;

		count++;

		if (height == 16)
			tile_idx &= 0xFE;

		uint8_t row = line - sprite_top;
		bool y_flipped = flags & 0x40;
		row = y_flipped ? height - 1 - row : row;

		uint16_t addr = (tile_idx * 16) + (row * 2);
		lo_byte = ppu->vram[addr];
		hi_byte = ppu->vram[addr + 1];

		uint8_t pal_reg = (flags & 0x10) ? io_map[OBP1] : io_map[OBP0];

		for (int bit = 7; bit >= 0; bit--) {
			uint8_t x_coord = x_pos - 8 + (7 - bit);

			if (x_coord < 0 || x_coord >= 160)
				continue;

			bool x_flipped = (flags & 0x20);
			bit = x_flipped ? 7 - bit : bit;

			lo_bit = (lo_byte >> bit) & 1;
			hi_bit = (hi_byte >> bit) & 1;

			color = (hi_bit << 1) | lo_bit;
			bool isCovered = (flags & 0x80) &&
					 bg_line_colors[x_coord] != 0;
			if (!color || isCovered)
				continue;

			uint8_t shade = (pal_reg >> (color * 2)) & 0x03;
			ppu->framebuffer[line * 160 + x_coord] =
				ppu_colors[shade];
		}
	}
}

void render_scanline(struct PPU *ppu)
{
	render_bg_wndw(ppu);
	render_sprites(ppu);
}
void ppu_step(struct GameBoy *gb, int dots)
{
	if ((io_map[LCDC] & 0x80) == 0) {
		fprintf(stderr, "\n");
		return;
	}

	gb->ppu.ly_dots += dots;
	uint8_t prev_ln = io_map[LY];
	enum ppu_mode prev_mode = gb->ppu.mode;
	switch (gb->ppu.mode) {
	case (OAM): //mode 2
		if (gb->ppu.ly_dots >= OAM_DOTS) {
			gb->ppu.mode = DRAW;
			gb->ppu.md3delay = 0; //TODO: calc mode 3 delay
		}
		break;
	case (DRAW): //mode 3
		if (gb->ppu.ly_dots >=
		    OAM_DOTS + DRAW_DOTS_MIN + gb->ppu.md3delay) {
			gb->ppu.mode = HBLNK;
			if (!gb->ppu.skip_frame)
				render_scanline(&gb->ppu);
		}
		break;
	case (HBLNK): // mode 0
		if (gb->ppu.ly_dots >= SCANLINE_DOTS) {
			gb->ppu.ly_dots -= SCANLINE_DOTS;
			io_map[LY]++;

			if (io_map[LY] < LY_VBLNK_FST) {
				gb->ppu.mode = OAM;
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
				gb->ppu.window_ly = 0;
				gb->ppu.skip_frame = false;
				gb->ppu.done_frame = true;
			}
		}
		break;
	}

	fprintf(stderr, "--- %s (ly=%d: ly_dots=%d, lyc=%d)\n",
		ModeNames[gb->ppu.mode], io_map[LY], gb->ppu.ly_dots,
		io_map[LYC]);

	if (prev_ln != io_map[LY])
		check_lyc_eq_ly(gb);

	if (prev_mode != gb->ppu.mode)
		update_stat(gb);
}
