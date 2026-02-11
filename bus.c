#include "bus.h"
#include "GameBoy.h"
#include <stdio.h>

uint8_t bus_read(struct GameBoy *gb, uint16_t addr)
{
	if (addr < 0x8000) {
		return cart_read(&gb->crt, addr);
	}
	if (addr >= 0x8000 && addr < 0xA000) {
		if (gb->ppu.mode == DRAW) {
			fprintf(stderr, "Cannot read VRAM. PPU mode %d\n",
				gb->ppu.mode);
			return 0xFF;
		}
		return gb->ppu.vram[addr - 0x8000];
	}

	if (addr >= 0xA000 && addr <= 0xBFFF) {
		//cart ram
		return cart_read(&gb->crt, addr);
	}

	if (addr >= 0xC000 && addr <= 0xDFFF) {
		//wram
		return gb->wram[addr - 0xC000];
	}
	if (addr >= 0xFE00 && addr <= 0xFE9F) {
		//oam
		if (gb->ppu.mode == OAM || gb->ppu.mode == DRAW) {
			//dma access allowed
			fprintf(stderr, "Cannot read OAM. PPU mode %d\n",
				gb->ppu.mode);
			return 0xFF;
		}
		return gb->ppu.oam[addr - 0xFE00];
	}
	if (addr >= 0xFF00 && addr <= 0xFF7F) {
		//io
		fprintf(stderr, "Reading IO 0xFF00-0xFF7F\n");
		exit(EXIT_FAILURE);
	}
	if (addr >= 0xFF80 && addr <= 0xFFFE) {
		//hram
		return gb->hram[addr - 0xFF80];
	}

	return 0xFF;
}

void bus_write(struct GameBoy *gb, uint16_t addr, uint8_t data)
{
	if (addr < 0x8000) {
		cart_write(&gb->crt, addr, data);
	}

	if (addr >= 0x8000 && addr < 0xA000) {
		if (gb->ppu.mode == DRAW) {
			fprintf(stderr,
				"Cannot write to VRAM. PPU in mode %d\n",
				gb->ppu.mode);
			return;
		}
		fprintf(stderr, "-- VRAM 0x%04x 0x%02x ", addr, data);
		gb->ppu.vram[addr - 0x8000] = data;
		return;
	}

	if (addr >= 0xA000 && addr <= 0xBFFF) {
		//cart ram
		cart_write(&gb->crt, addr, data);
	}
	if (addr >= 0xC000 && addr <= 0xDFFF) {
		//wram
		gb->wram[addr - 0xC000] = data;
	}
	if (addr >= 0xFE00 && addr <= 0xFE9F) {
		//oam
		if (gb->ppu.mode == OAM || gb->ppu.mode == DRAW) {
			//dma access allowed
			fprintf(stderr, "Cannot write to OAM. PPU in mode %d\n",
				gb->ppu.mode);
			return;
		}
		fprintf(stderr, "-- OAM 0x%04x 0x%02x ", addr, data);
		gb->ppu.oam[addr - 0xFE00] = data;
		return;
	}
	if (addr >= 0xFF00 && addr <= 0xFF7F) {
		//io
		fprintf(stderr, "Reading IO 0xFF00-0xFF7F\n");
		exit(EXIT_FAILURE);
	}
	if (addr >= 0xFF80 && addr <= 0xFFFE) {
		//hram
		fprintf(stderr, "-- HRAM 0x%04x 0x%02x ", addr, data);
		gb->hram[addr - 0xFF80] = data;
	}

	fprintf(stderr, "Error writing to  %04x\n", addr);
	exit(EXIT_FAILURE);
}
