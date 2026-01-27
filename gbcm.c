#include <SDL3/SDL.h>
#include <SDL3/SDL_main.h>
#include <stdlib.h>
#include <stdio.h>
#include "CPU.h"

#define CLOCK 4194304
#define DOTS_PER_FRAME 70224.0f
#define NS_PER_FRAME (1000000000 * DOTS_PER_FRAME / CLOCK)

#define OAM_DOTS 80
#define DRAW_DOTS_MIN 172
#define SCANLINE_DOTS 456
#define LY_VBLNK_FST 144
#define LY_VBLNK_LST 153

static SDL_Window *window = NULL;
static SDL_Renderer *renderer = NULL;

typedef struct PPU {
	int dots, frm_dots, ly_dots;
	uint8_t ly, lyc, scy, scx, wy, wx, dma, m3delay;
	enum {
		HBLNK,
		VBLNK,
		OAM,
		DRAW,
	} mode;
} PPU;

const char *const ModeNames[] = {
	"H Blank",
	"V Blank",
	"OAM",
	"DRAW",
};

static PPU ppu;
static CPU cpu;

/* This function runs once at startup. */
void init_sdl()
{
	/* Create the window */
	if (!SDL_Init(SDL_INIT_VIDEO)) {
		SDL_Log("Error initializing SDL: %s\n", SDL_GetError());
		exit(1);
	}
	if (!SDL_CreateWindowAndRenderer(
		    //"Hello World", 160 * 7, 160 * 7,
		    "Hello World", 160 * 7, 144 * 7, SDL_WINDOW_ALWAYS_ON_TOP,
		    &window, &renderer)) {
		SDL_Log("Couldn't create window and renderer: %s\n",
			SDL_GetError());
		exit(1);
	}
}

void hndlevnt(int *running)
{
	SDL_Event event;
	while (SDL_PollEvent(&event)) {
		if (event.type == SDL_EVENT_KEY_DOWN ||
		    event.type == SDL_EVENT_QUIT)
			*running = 0;
	}
}

void update_stat()
{
	fprintf(stderr, "---- ALERT PPU MODE CHANGE ");
}

void request_interrupt()
{
	fprintf(stderr, "----  ALERT: REQUEST INTERUPPT ");
}

void check_lyc_eq_ly()
{
	if (ppu.ly == ppu.lyc)
		fprintf(stderr, "---- ALERT: LY == LYC ");
}

void render_scanline()
{
}

void ppu_step(int dots)
{
	ppu.frm_dots += dots;
	ppu.ly_dots += dots;
	const char *prv_mode = ModeNames[ppu.mode];
	switch (ppu.mode) {
	case (OAM): //mode 2
		if (ppu.ly_dots >= OAM_DOTS) {
			ppu.mode = DRAW;
			ppu.m3delay = 0; //TODO: calc m3 delay
			update_stat();
		}

		break;
	case (DRAW): //mode 3

		// mode 3: During Mode 3, by default the PPU outputs one pixel to the screen per dot, from left to right; the screen is 160 pixels wide, so the minimum Mode 3 length is 160 + 12(1) = 172 dots.
		// (1) The 12 extra dots of penalty come from two tile fetches at the beginning of Mode 3. One is the first tile in the scanline (the one that gets shifted by SCX % 8 pixels), the other is simply discarded

		if (ppu.ly_dots >= OAM_DOTS + DRAW_DOTS_MIN + ppu.m3delay) {
			ppu.mode = HBLNK;
			update_stat();
		}
		break;
	case (HBLNK): // mode 0

		// mode 0 (hblank) length is 376 - len mode 3
		// starts when line of pixels is completely drawn. Opportunity to do some work before next line. One for every line.
		// enables raster effects

		if (ppu.ly_dots >= SCANLINE_DOTS) {
			ppu.ly_dots -= SCANLINE_DOTS;
			ppu.ly++;
			check_lyc_eq_ly();

			if (ppu.ly < LY_VBLNK_FST) {
				ppu.mode = OAM;
				update_stat();
			} else {
				ppu.mode = VBLNK;
				request_interrupt();
			}
		}
		break;
	case (VBLNK): // mode 1
		if (ppu.ly_dots >= SCANLINE_DOTS) {
			ppu.ly_dots -= SCANLINE_DOTS;
			ppu.ly++;

			if (ppu.ly > LY_VBLNK_LST) {
				ppu.mode = OAM;
				ppu.ly = 0;
			}

			check_lyc_eq_ly();
			update_stat();
		}
		break;
	}

	fprintf(stderr, "--- %s (ly=%d: ly_dots=%d, frm_dots=%d)\n",
		ModeNames[ppu.mode], ppu.ly, ppu.ly_dots, ppu.frm_dots);

	return;
}

void hndl_interrupts()
{
}

void emulateframe()
{
	int ticks = 0, tot_ticks = 0;

	do {
		ticks = cpu_step(&cpu);
		ppu_step(ticks);
		hndl_interrupts();
		tot_ticks += ticks;
	} while (tot_ticks < DOTS_PER_FRAME);
}

/* This function runs once per frame, and is the heart of the program. */
void render()
{
	const char *message = "Hello World!";
	int w = 0, h = 0;
	float x, y;
	const float scale = 4.0f;

	/* Center the message and scale it up */
	SDL_GetRenderOutputSize(renderer, &w, &h);
	SDL_SetRenderScale(renderer, scale, scale);
	x = ((w / scale) -
	     SDL_DEBUG_TEXT_FONT_CHARACTER_SIZE * SDL_strlen(message)) /
	    2;
	y = ((h / scale) - SDL_DEBUG_TEXT_FONT_CHARACTER_SIZE) / 2;

	/* Draw the message */
	SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);

	SDL_RenderClear(renderer);
	SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255);
	SDL_RenderDebugText(renderer, x, y, message);

	SDL_RenderPresent(renderer);
}

int main(int argc, char *argv[])
{
	cpu_ldrom(&cpu, argv[1]);
	init_sdl();
	ppu.mode = OAM;
	ppu.ly = ppu.frm_dots = 0;
	int running = 1;
	while (running) {
		uint64_t frame_start = SDL_GetTicksNS();
		hndlevnt(&running);
		emulateframe();
		render();

		uint64_t delta_t = SDL_GetTicksNS() - frame_start;
		fprintf(stderr, "\ntime working: %06f ms",
			(SDL_GetTicksNS() - frame_start) / 1000000.0f);

		if (delta_t < NS_PER_FRAME - 1000000)
			SDL_DelayNS(NS_PER_FRAME - 1000000 - delta_t);
		while (delta_t < NS_PER_FRAME) {
			delta_t = SDL_GetTicksNS() - frame_start;
		}
		fprintf(stderr, "\n%06f ms (ppu %d)\n",
			(SDL_GetTicksNS() - frame_start) / 1000000.0f,
			ppu.frm_dots);
	}

	SDL_DestroyRenderer(renderer);
	SDL_DestroyWindow(window);
	SDL_Quit();
}
