#ifndef GAMEBOY_H
#define GAMEBOY_H

#include "CPU.h"
#include "PPU.h"
#include "cartridge.h"
#include "DMA.h"
#include <stdbool.h>

typedef struct GameBoy {
	struct CPU cpu;
	struct PPU ppu;
	struct Cartridge crt;
	struct DMA dma;
	bool running;
	bool error;
	uint8_t wram[8192];
	uint8_t hram[127];
} GameBoy;

void gb_emulate(struct GameBoy *gb);
void gb_loadrom(struct GameBoy *gb, const char *path);

#endif
