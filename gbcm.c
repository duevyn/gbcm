#include <SDL3/SDL.h>
#include <SDL3/SDL_main.h>
#include <stdlib.h>
#include <stdio.h>
#include "CPU.h"

#define CLOCK 4194304
#define DOTS_PER_FRAME 70224.0f
#define NS_PER_FRAME (1000000000 * DOTS_PER_FRAME / CLOCK)

static SDL_Window *window = NULL;
static SDL_Renderer *renderer = NULL;

typedef struct PPU {
	int dots;
	enum {
		HBLNK,
		VBLNK,
		SCAN,
		DRAW,
	} mode;
} PPU;

const char *const ModeNames[] = {
	"H Blank",
	"V Blank",
	"SCAN",
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

void ppu_step(int dots)
{
	// Advance PPU dots by 'clocks'
	// Update modes, render pixels to gb_framebuffer if in drawing mode
	// If LY == 144 and entering VBlank, set ppu.frame_ready = 1, trigger interrupt
	// If LY == 154, reset to 0 for next frame
	//fprintf(stderr, "::: dots %02d md %d\n", ppu.dots, ppu.mode);
	ppu.dots += dots;
	if (ppu.dots >= 154 || ppu.dots <= 80) {
		ppu.mode = SCAN;
		ppu.dots = ppu.dots >= 154 ? 0 : ppu.dots;
		fprintf(stderr, ":: (%s: %d)\n", ModeNames[ppu.mode], ppu.dots);
		return;
	}
	if (ppu.dots >= 144 && ppu.dots < 154) {
		// last for 4560 dots (10 scanlines)
		// gameboy triggers vblank interrupt here
		ppu.mode = VBLNK;
		fprintf(stderr, ":: (%s: %d)\n", ModeNames[ppu.mode], ppu.dots);
		return;
	}
	fprintf(stderr, ":: (%s: %d)\n", ModeNames[ppu.mode], ppu.dots);

	// figure out mode 3 (draw) length: 172-289 dots
	// mode 3: During Mode 3, by default the PPU outputs one pixel to the screen per dot, from left to right; the screen is 160 pixels wide, so the minimum Mode 3 length is 160 + 12(1) = 172 dots.
	// (1) The 12 extra dots of penalty come from two tile fetches at the beginning of Mode 3. One is the first tile in the scanline (the one that gets shifted by SCX % 8 pixels), the other is simply discarded
	//

	// mode 0 (hblank) length is 376 - len mode 3
	// starts when line of pixels is completely drawn. Opportunity to do some work before next line. One for every line.
	// enables raster effects
	// update LY to reflect next line
	// The Game Boy constantly compares the value of the LYC and LY registers. When both values are identical, the “LYC=LY” flag in the STAT register is set, and (if enabled) a STAT interrupt is requested.
	return;
}

void hndl_interrupts()
{
}

void emulate()
{
	int ticks = 0;

	do {
		ticks = cpu_step(&cpu);
		ppu_step(ticks);
	} while (ppu.mode != VBLNK);
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
	ppu.dots = 0;
	ppu.mode = SCAN;
}

int main(int argc, char *argv[])
{
	cpu_ldrom(&cpu, argv[1]);
	init_sdl();
	ppu.mode = SCAN;
	int running = 1;
	while (running) {
		uint64_t frame_start = SDL_GetTicksNS();
		hndlevnt(&running);
		emulate();
		hndl_interrupts();
		render();

		uint64_t delta_t = SDL_GetTicksNS() - frame_start;

		if (delta_t < NS_PER_FRAME - 1000000)
			SDL_DelayNS(NS_PER_FRAME - 1000000 - delta_t);
		while (delta_t < NS_PER_FRAME) {
			delta_t = SDL_GetTicksNS() - frame_start;
		}
		fprintf(stderr, "\n%06f ms (ppu %d)\n",
			(SDL_GetTicksNS() - frame_start) / 1000000.0f,
			ppu.dots);
	}

	SDL_DestroyRenderer(renderer);
	SDL_DestroyWindow(window);
	SDL_Quit();
}
