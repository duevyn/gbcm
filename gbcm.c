#include <SDL3/SDL.h>
#include <SDL3/SDL_main.h>
#include <stdlib.h>
#include <stdio.h>
#include "GameBoy.h"
#include "bus.h"
#include "io.h"
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

// Z = A, X = B, Backspace = Select, Enter = Start, ESC = quit
void hndlevnt(int *running)
{
	SDL_Event event;
	while (SDL_PollEvent(&event)) {
		if (event.type == SDL_EVENT_QUIT) {
			*running = 0;
			return;
		}

		if (event.type == SDL_EVENT_KEY_DOWN) {
			switch (event.key.key) {
			case SDLK_Z: //A
				io_joypad_press(JOYPAD_BUTTONS, A$RIGHT);
				break;
			case SDLK_X: //B
				io_joypad_press(JOYPAD_BUTTONS, B$LEFT);
				break;
			case SDLK_BACKSPACE: //Select
				io_joypad_press(JOYPAD_BUTTONS, SELECT$UP);
				break;
			case SDLK_RETURN: //Start
				io_joypad_press(JOYPAD_BUTTONS, START$DOWN);
				break;

			case SDLK_RIGHT:
				io_joypad_press(JOYPAD_DPAD, A$RIGHT);
				break;
			case SDLK_LEFT:
				io_joypad_press(JOYPAD_DPAD, B$LEFT);
				break;
			case SDLK_UP:
				io_joypad_press(JOYPAD_DPAD, SELECT$UP);
				break;
			case SDLK_DOWN:
				io_joypad_press(JOYPAD_DPAD, START$DOWN);
				break;

			case SDLK_ESCAPE:
				*running = 0;
				break;
			default:
				break;
			}
		} else if (event.type == SDL_EVENT_KEY_UP) {
			switch (event.key.key) {
			case SDLK_Z:
				io_joypad_release(JOYPAD_BUTTONS, A$RIGHT);
				break;
			case SDLK_X:
				io_joypad_release(JOYPAD_BUTTONS, B$LEFT);
				break;
			case SDLK_BACKSPACE:
				io_joypad_release(JOYPAD_BUTTONS, SELECT$UP);
				break;
			case SDLK_RETURN:
				io_joypad_release(JOYPAD_BUTTONS, START$DOWN);
				break;

			case SDLK_RIGHT:
				io_joypad_release(JOYPAD_DPAD, A$RIGHT);
				break;
			case SDLK_LEFT:
				io_joypad_release(JOYPAD_DPAD, B$LEFT);
				break;
			case SDLK_UP:
				io_joypad_release(JOYPAD_DPAD, SELECT$UP);
				break;
			case SDLK_DOWN:
				io_joypad_release(JOYPAD_DPAD, START$DOWN);
				break;

			default:
				break;
			}
		}
	}
}

void render(struct GameBoy *gb)
{
	int pitch = 160 * sizeof(uint32_t);

	SDL_UpdateTexture(texture, NULL, gb->ppu.framebuffer, pitch);
	SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255);
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
	fprintf(stderr, "\nLCDC: %08b\n", io_map[LCDC]);
}

int main(int argc, char *argv[])
{
	fprintf(stderr, "MS per frame %06f ms\n\n", NS_PER_FRAME / 1000000.0f);
	struct GameBoy gb = { 0 };
	gb_loadrom(&gb, argv[1]);
	init_sdl();
	int running = 1;
	log_init("cpu_log.txt");
	fprintf(logfile, "JPB 0x%02x, JPD 0x%02x\n\n", JOYPAD_BUTTONS,
		JOYPAD_DPAD);
	log_cpu_state(&gb);
	uint64_t ms_tot = 0;

	while (running) {
		uint64_t frame_start = SDL_GetTicksNS();
		hndlevnt(&running);
		gb_emulate(&gb);

		double emTime = (SDL_GetTicksNS() - frame_start) / 1000000.0;

		fprintf(stderr, "time emulating: %lf ms ", emTime);

		uint64_t delta_t = SDL_GetTicksNS() - frame_start;
		if (delta_t < NS_PER_FRAME - 1000000)
			SDL_DelayNS(NS_PER_FRAME - 1000000 - delta_t);

		double wakeTime = (SDL_GetTicksNS() - frame_start) / 1000000.0;
		fprintf(stderr, "-- aftr wakeup: %lf ms ", wakeTime);
		render(&gb);
		double rendTime =
			((SDL_GetTicksNS() - frame_start) / 1000000.0) -
			wakeTime;
		fprintf(stderr, "-- render time: %lf ms ", rendTime);

		while ((SDL_GetTicksNS() - frame_start) < NS_PER_FRAME) {
		}
		fprintf(stderr, "-- after delay %lf ms %lu\n",
			(SDL_GetTicksNS() - frame_start) / 1000000.0,
			gb.cpu.dcnt / 70224);
		//if ((gb.cpu.dcnt / 70224) >= 3038)
		//	running = 0;
	}
	ms_tot = (SDL_GetTicksNS() / 1000000.0);
	fprintf(stderr, "\ntotal time: %f seconds\n\n", ms_tot / 1000.0);

	printTiles(&gb);
	fprintf(stderr, "\nTAC %08b, cnt %lu \n", bus_read(&gb, 0xFF07),
		gb.cpu.cnt);

	log_close();
	SDL_DestroyRenderer(renderer);
	SDL_DestroyWindow(window);
	SDL_Quit();
}
