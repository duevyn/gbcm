#ifndef GAMEBOY_H
#define GAMEBOY_H

#include "CPU.h"
#include "PPU.h"
#include "cartridge.h"

typedef struct GameBoy {
	CPU cpu;
	PPU ppu;
	Cartridge crt;

	uint8_t wram[8192];
	uint8_t hram[127];
	uint8_t ie_reg;
	uint8_t if_reg;
} GameBoy;

void gb_emulate(struct GameBoy *gb);
void gb_loadrom(struct GameBoy *gb, const char *path);

#endif
