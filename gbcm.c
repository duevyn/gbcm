#include <SDL3/SDL.h>
#include <SDL3/SDL_main.h>
#include <stdlib.h>
#include <stdio.h>
#include "GameBoy.h"

static SDL_Window *window = NULL;
static SDL_Renderer *renderer = NULL;

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

void render_scanline()
{
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
	//cpu_ldrom(&cpu, argv[1]);
	fprintf(stderr, "MS per frame %06f ms\n\n", NS_PER_FRAME / 1000000.0f);
	struct GameBoy gb;
	gb_loadrom(&gb, argv[1]);
	init_sdl();
	int running = 1;
	while (running) {
		uint64_t frame_start = SDL_GetTicksNS();
		hndlevnt(&running);
		gb_emulate(&gb);

		fprintf(stderr, "\ntime emulating: %06f ms ",
			(SDL_GetTicksNS() - frame_start) / 1000000.0f);
		render();
		fprintf(stderr, "-- aftr render: %06f ms ",
			(SDL_GetTicksNS() - frame_start) / 1000000.0f);

		uint64_t delta_t = SDL_GetTicksNS() - frame_start;
		if (delta_t < NS_PER_FRAME - 1000000)
			SDL_DelayNS(NS_PER_FRAME - 1000000 - delta_t);
		while (delta_t < NS_PER_FRAME) {
			delta_t = SDL_GetTicksNS() - frame_start;
		}
		fprintf(stderr, "-- after delay %06f ms\n\n\n",
			(SDL_GetTicksNS() - frame_start) / 1000000.0f);
	}

	SDL_DestroyRenderer(renderer);
	SDL_DestroyWindow(window);
	SDL_Quit();
}
