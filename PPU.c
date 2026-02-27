#include "PPU.h"
#include "GameBoy.h"
#include "logger.h"
#include "io.h"
#include <stdio.h>
#include <stdlib.h>

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

static uint8_t sprites_pend[10];
static uint8_t sprite_cnt;
static uint8_t bg_line_colors[160];

static inline void update_stat(struct GameBoy *gb)
{
	io_map[STAT] &= ~0x03;
	io_map[STAT] |= (gb->ppu.mode & 0x03);
}

static inline void check_lyc_eq_ly(struct GameBoy *gb)
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
	uint8_t tl_id, tl_mp_row, tl_mp_col, tl_data_row, color, shade;
	uint8_t hi_byte, lo_byte, hi_bit, lo_bit, bit;
	uint16_t tl_mp_ind, tl_mp_addr, by_ind;

	uint8_t ln = io_map[LY];
	uint8_t lcdc = io_map[LCDC];

	uint16_t bg_tl_map_addr = (0x08 & lcdc) ? 0x1C00 : 0x1800;
	uint16_t wn_tl_map_addr = (0x40 & lcdc) ? 0x1C00 : 0x1800;

	bool draw_wn = ((ln >= io_map[WY]) && ((lcdc & 0x21) == 0x21));
	int16_t wn_x = io_map[WX] - 7;

	bool sign_addr_md = !(0x10 & lcdc);
	bool inc_wndw_ly = false;

	// It is Ok to refetch tile 8 times instead of holding tile and
	// complicating logic because emulation loop is typcially under
	// 0.5 ms and we render once ever 16.742 seconds.
	for (int i = 0; i < 160; i++) {
		if (!(draw_wn && (i >= wn_x))) {
			// uint8_t built in mod 256
			tl_mp_row = ln + io_map[SCY];
			tl_mp_col = i + io_map[SCX];
			tl_mp_addr = bg_tl_map_addr;
			tl_data_row = tl_mp_row % 8;
		} else {
			inc_wndw_ly = true;
			tl_mp_row = (ppu->window_ly);
			tl_mp_col = i - wn_x;
			tl_mp_addr = wn_tl_map_addr;
			tl_data_row = ppu->window_ly % 8;
		}

		tl_mp_ind = tl_mp_addr + (tl_mp_row / 8) * 32 + tl_mp_col / 8;
		tl_id = ppu->vram[tl_mp_ind];
		by_ind = tl_id * 16 + tl_data_row * 2;

		if (sign_addr_md && (tl_id <= 127))
			by_ind += 0x1000;

		lo_byte = ppu->vram[by_ind];
		hi_byte = ppu->vram[by_ind + 1];
		bit = (7 - (tl_mp_col % 8));
		lo_bit = (lo_byte >> bit) & 1;
		hi_bit = (hi_byte >> bit) & 1;

		color = (hi_bit << 1) | lo_bit;
		bg_line_colors[i] = color;
		shade = (io_map[BGP] >> (color * 2)) & 0x03;
		ppu->framebuffer[ln * 160 + i] = ppu_colors[shade];
	}

	if (inc_wndw_ly)
		ppu->window_ly++;
}

static inline void render_sprites(struct PPU *ppu)
{
	if (!(io_map[LCDC] & 0x02))
		return;

	uint8_t hi_byte, lo_byte, hi_bit, lo_bit, bit;
	uint8_t oam_i, x_pos, y_pos, tl_idx, flags, row;
	uint8_t color, shade, pal_reg;
	bool isCovered, x_flipped, y_flipped;
	int16_t lcd_x, sprite_top;
	uint16_t addr;

	int16_t line = io_map[LY];
	int8_t height = (io_map[LCDC] & 0x04) ? 16 : 8;

	for (int i = sprite_cnt - 1; i >= 0; i--) {
		oam_i = sprites_pend[i];
		y_pos = ppu->oam[oam_i * 4];
		x_pos = ppu->oam[oam_i * 4 + 1];
		tl_idx = ppu->oam[oam_i * 4 + 2];
		flags = ppu->oam[oam_i * 4 + 3];

		sprite_top = y_pos - 16;
		if (line < sprite_top || ((line >= sprite_top + height)))
			continue;

		if (height == 16)
			tl_idx &= 0xFE;

		row = line - sprite_top;
		y_flipped = flags & 0x40;
		row = y_flipped ? height - 1 - row : row;

		addr = (tl_idx * 16) + (row * 2);
		lo_byte = ppu->vram[addr];
		hi_byte = ppu->vram[addr + 1];

		pal_reg = (flags & 0x10) ? io_map[OBP1] : io_map[OBP0];

		for (int j = 7; j >= 0; j--) {
			lcd_x = x_pos - 8 + (7 - j);

			if (lcd_x < 0 || lcd_x >= 160)
				continue;

			x_flipped = (flags & 0x20);
			bit = x_flipped ? 7 - j : j;

			lo_bit = (lo_byte >> bit) & 1;
			hi_bit = (hi_byte >> bit) & 1;

			color = (hi_bit << 1) | lo_bit;
			isCovered = (flags & 0x80) &&
				    bg_line_colors[lcd_x] != 0;
			if (!color || isCovered)
				continue;

			shade = (pal_reg >> (color * 2)) & 0x03;
			ppu->framebuffer[line * 160 + lcd_x] =
				ppu_colors[shade];
		}
	}
}

static inline uint8_t oam_search(struct PPU *ppu)
{
	if (!(io_map[LCDC] & 0x02))
		return 0;

	ppu->md3delay = io_map[SCX] % 8;
	sprite_cnt = 0;

	int16_t sprite_top;
	uint8_t y_pos;

	uint8_t result = 0;
	uint8_t ln = io_map[LY];
	uint8_t height = (io_map[LCDC] & 0x04) ? 16 : 8;

	for (int i = 0; i < 40 && sprite_cnt < 10; i++) {
		y_pos = ppu->oam[i * 4];
		sprite_top = y_pos - 16;
		if (ln >= sprite_top && ln < sprite_top + height) {
			sprites_pend[sprite_cnt++] = i;
			if (ppu->oam[i * 4 + 1] > 0)
				ppu->md3delay += 6;
			else
				ppu->md3delay += 11;
		}
	}

	return result;
}

static inline void render_scanline(struct PPU *ppu)
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
			//TODO: calc mode 3 delay
			gb->ppu.md3delay = oam_search(&gb->ppu);
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

	fprintf(stderr, "--- %s (ly=%d: ly_dots=%d, stat=0x%04x)\n",
		ModeNames[gb->ppu.mode], io_map[LY], gb->ppu.ly_dots,
		io_map[STAT]);

	if (prev_ln != io_map[LY])
		check_lyc_eq_ly(gb);

	if (prev_mode != gb->ppu.mode)
		update_stat(gb);
}
