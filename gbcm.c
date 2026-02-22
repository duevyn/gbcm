#include <SDL3/SDL.h>
#include <SDL3/SDL_main.h>
#include <stdlib.h>
#include <stdio.h>
#include "GameBoy.h"
#include "bus.h"
#include "logger.h"

static SDL_Window *window = NULL;
static SDL_Renderer *renderer = NULL;
static SDL_Texture *texture = NULL;
//#define fprintf(stderr, ...) ((void)0)

void init_sdl()
{
	if (!SDL_Init(SDL_INIT_VIDEO)) {
		SDL_Log("Error initializing SDL: %s\n", SDL_GetError());
		exit(1);
	}
	if (!SDL_CreateWindowAndRenderer("Hello World", 160 * 7, 144 * 7,
					 SDL_WINDOW_ALWAYS_ON_TOP, &window,
					 &renderer)) {
		SDL_Log("Couldn't create window and renderer: %s\n",
			SDL_GetError());
		exit(1);
	}
	if (!(texture = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_ARGB8888,
					  SDL_TEXTUREACCESS_STREAMING, 160,
					  144))) {
		fprintf(stderr, "Failed to create texture: %s\n",
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

void render(struct GameBoy *gb)
{
	int pitch = 160 * sizeof(uint32_t);

	SDL_UpdateTexture(texture, NULL, gb->ppu.framebuffer, pitch);
	SDL_SetRenderDrawColor(renderer, 50, 50, 50, 255);
	SDL_RenderClear(renderer);
	SDL_RenderTexture(renderer, texture, NULL, NULL);
	SDL_RenderPresent(renderer);
}

void printTiles(struct GameBoy *gb)
{
	int data = 0x8000;
	int map = 0x9800;

	gb->ppu.mode = HBLNK;

	for (int i = 0; i < 32; i++) {
		for (int j = 0; j < 32; j++) {
			fprintf(stderr, "%02x ",
				bus_read(gb, map + i * 32 + j));
		}
		fprintf(stderr, "  -> off: %04x, addr: %04x\n", i * 32,
			map + i * 32 - 0x8000);
	}
	fprintf(stderr, "\n\n");
}

int main(int argc, char *argv[])
{
	fprintf(stderr, "MS per frame %06f ms\n\n", NS_PER_FRAME / 1000000.0f);
	struct GameBoy gb = { 0 };
	gb_loadrom(&gb, argv[1]);
	init_sdl();
	int running = 1;
	//log_init("cpu_log.txt");
	//log_cpu_state(&gb);
	uint64_t ms_tot = 0;

	while (running) {
		uint64_t frame_start = SDL_GetTicksNS();
		hndlevnt(&running);
		gb_emulate(&gb);

		fprintf(stderr, "time emulating: %06f ms ",
			(SDL_GetTicksNS() - frame_start) / 1000000.0f);
		render(&gb);
		fprintf(stderr, "-- aftr render: %06f ms ",
			(SDL_GetTicksNS() - frame_start) / 1000000.0f);

		uint64_t delta_t = SDL_GetTicksNS() - frame_start;
		if (delta_t < NS_PER_FRAME - 1000000)
			SDL_DelayNS(NS_PER_FRAME - 1000000 - delta_t);
		while (delta_t < NS_PER_FRAME) {
			delta_t = SDL_GetTicksNS() - frame_start;
		}
		fprintf(stderr, "-- after delay %06f ms %lu\n",
			(SDL_GetTicksNS() - frame_start) / 1000000.0f,
			gb.cpu.dcnt / 70224);
		//if ((gb.cpu.dcnt / 70224) >= 519)
		//	running = 0;
	}
	ms_tot = (SDL_GetTicksNS() / 1000000.0f);
	fprintf(stderr, "\ntotal time: %f seconds\n\n", ms_tot / 1000.0f);

	printTiles(&gb);

	log_close();
	SDL_DestroyRenderer(renderer);
	SDL_DestroyWindow(window);
	SDL_Quit();
}
