#include <SDL3/SDL.h>
#include <SDL3/SDL_main.h>
#include <stdlib.h>
#include <stdio.h>
#include "GameBoy.h"
#include "bus.h"
#include "io.h"
#include "logger.h"
#include <errno.h>

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
	if (!SDL_CreateWindowAndRenderer("gbcM", 160 * 4, 144 * 4,
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

// X = A, Z = B, Backspace = Select, Enter = Start, ESC = quit
void hndlevnt(bool *running)
{
	SDL_Event event;
	while (SDL_PollEvent(&event)) {
		if (event.type == SDL_EVENT_QUIT) {
			*running = 0;
			return;
		}

		if (event.type == SDL_EVENT_KEY_DOWN) {
			switch (event.key.key) {
			case SDLK_X: //A
				io_joypad_press(JOYPAD_BUTTONS, A$RIGHT);
				break;
			case SDLK_Z: //B
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
			case SDLK_TAB:
				io_joypad_press(JOYPAD_BUTTONS, TAB);
				break;
			case SDLK_ESCAPE:
				*running = 0;
				break;
			default:
				break;
			}
		} else if (event.type == SDL_EVENT_KEY_UP) {
			switch (event.key.key) {
			case SDLK_X:
				io_joypad_release(JOYPAD_BUTTONS, A$RIGHT);
				break;
			case SDLK_Z:
				io_joypad_release(JOYPAD_BUTTONS, B$LEFT);
				break;
			case SDLK_BACKSPACE:
				io_joypad_release(JOYPAD_BUTTONS, SELECT$UP);
				break;
			case SDLK_RETURN:
				io_joypad_release(JOYPAD_BUTTONS, START$DOWN);
				break;
			case SDLK_TAB:
				io_joypad_release(JOYPAD_BUTTONS, TAB);
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

int main(int argc, char *argv[])
{
	struct GameBoy gb = { 0 };
	gb_loadrom(&gb, argv[1]);
	init_sdl();
	log_init("cpu_log.txt");
	log_cpu_state(&gb);

	double ms_tot = 0;
	uint64_t frames = 0;

	int running = 1;
	uint64_t start = SDL_GetTicksNS();
	uint64_t next_frame_time = start;

	while (gb.running) {
		next_frame_time += NS_PER_FRAME;
		hndlevnt(&gb.running);
		gb_emulate(&gb);
		render(&gb);

		uint64_t now = SDL_GetTicksNS();

		if (now < next_frame_time - 1000000)
			SDL_DelayNS(next_frame_time - now - 1000000);

		frames++;
		while (SDL_GetTicksNS() < next_frame_time) {
		}
	}
	uint64_t done = (SDL_GetTicksNS());
	ms_tot = (done - start);

	fprintf(stderr, "\nframes: %lu\ntotal time: %.12lf seconds,\n", frames,
		ms_tot / 1.0E9);
	fprintf(stderr, "expected time: %.12lf seconds\n\n",
		NS_PER_FRAME / 1.0E9 * frames);

	if (gb.error)
		fprintf(stderr,
			"\nERROR: last op: 0x%02x (prefix: %b)\npc: 0x%04x, instr: %lu\n\n",
			gb.cpu.op, gb.cpu.prefix, gb.cpu.pc, gb.cpu.cnt);
	else
		fprintf(stderr,
			"\nlast op: 0x%02x (prefix: %b)\npc: 0x%04x, instr: %lu\n\n",
			gb.cpu.op, gb.cpu.prefix, gb.cpu.pc, gb.cpu.cnt);

	log_close();
	crt_eject(&gb.crt);
	SDL_DestroyRenderer(renderer);
	SDL_DestroyWindow(window);
	SDL_Quit();
}
