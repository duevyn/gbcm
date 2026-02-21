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
	fprintf(stderr, "---- ALERT PPU MODE CHANGE ");
}

void check_lyc_eq_ly(struct GameBoy *gb)
{
	if (io_map[LY] == io_map[LYC]) {
		fprintf(stderr, "---- ALERT: LY == LYC ");
	}
}

void ppu_step(struct GameBoy *gb, int dots)
{
	if ((io_map[LCDC] & 0x80) == 0) {
		fprintf(stderr, "\n");
		return;
	}
	gb->ppu.ly_dots += dots;
	uint8_t prev = io_map[LY];
	const char *prv_mode = ModeNames[gb->ppu.mode];
	switch (gb->ppu.mode) {
	case (OAM): //mode 2
		if (gb->ppu.ly_dots >= OAM_DOTS) {
			gb->ppu.mode = DRAW;
			gb->ppu.md3delay = 0; //TODO: calc mode 3 delay
			update_stat();
		}

		break;
	case (DRAW): //mode 3

		// mode 3: During Mode 3, by default the PPU outputs one pixel to the screen per dot, from left to right; the screen is 160 pixels wide, so the minimum Mode 3 length is 160 + 12(1) = 172 dots.
		// (1) The 12 extra dots of penalty come from two tile fetches at the beginning of Mode 3. One is the first tile in the scanline (the one that gets shifted by SCX % 8 pixels), the other is simply discarded

		if (gb->ppu.ly_dots >=
		    OAM_DOTS + DRAW_DOTS_MIN + gb->ppu.md3delay) {
			gb->ppu.mode = HBLNK;
			update_stat();
		}
		break;
	case (HBLNK): // mode 0

		// mode 0 (hblank) length is 376 - len mode 3
		// starts when line of pixels is completely drawn. Opportunity to do some work before next line. One for every line.
		// enables raster effects

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

				if (logfile) {
					fprintf(logfile,
						"%lu: [VBLANK INTERRUPT REQ] prev line %d, curr line %d<<<\n",
						gb->cpu.cnt, prev, io_map[LY]);
				}
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
	if (prv_mode != ModeNames[gb->ppu.mode] && gb->ppu.mode == OAM)
		fprintf(stderr, "\n\n");

	return;
}
