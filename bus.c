#include "bus.h"
#include "GameBoy.h"
#include "DMA.h"
#include "io.h"
#include <stdio.h>
#include "logger.h"

//#define fprintf(stderr, ...) ((void)0)

uint8_t bus_read(struct GameBoy *gb, uint16_t addr)
{
	if (addr < 0x8000)
		return cart_read(&gb->crt, addr);

	if (addr >= 0x8000 && addr < 0xA000) {
		if ((io_map[LCDC] & 0x80) && (gb->ppu.mode == DRAW)) {
			fprintf(stderr, "Cannot read VRAM. PPU mode %d\n",
				gb->ppu.mode);
			return 0xFF;
		}
		return gb->ppu.vram[addr - 0x8000];
	}

	if (addr >= 0xA000 && addr <= 0xBFFF) //cart ram
		return cart_read(&gb->crt, addr);

	if (addr >= 0xC000 && addr <= 0xDFFF) //wram
		return gb->wram[addr - 0xC000];

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

	if (addr >= 0xFEA0 && addr <= 0xFEFF) //PROHIBITED
		return 0x00;

	if ((addr >= 0xFF00 && addr <= 0xFF7F) || (addr == 0xFFFF)) //io
		return io_rd(addr);

	if (addr >= 0xFF80 && addr <= 0xFFFE) //hram
		return gb->hram[addr - 0xFF80];

	return 0xFF;
}

void bus_write(struct GameBoy *gb, uint16_t addr, uint8_t data)
{
	if (addr == 0xFF80) {
		if (logfile) {
			fprintf(logfile,
				"%lu: [MEM] WRITE FF80 <= %02X (AT PC:%04X)\n",
				gb->cpu.cnt, data, gb->cpu.pc);
		}
	}
	if (addr < 0x8000) { //rom
		cart_write(&gb->crt, addr, data);
		fprintf(stderr, "  ALERT wr @rom 0x%04x 0x%02x ", addr, data);
		return;
	}

	if (addr >= 0x8000 && addr < 0xA000) { //vram
		if ((io_map[LCDC] & 0x80) && (gb->ppu.mode == DRAW)) {
			fprintf(stderr,
				"ALERT: Cannot write to VRAM. PPU in mode %d ",
				gb->ppu.mode);
			return;
		}

		fprintf(stderr, "  wr @vram 0x%04x 0x%02x ", addr, data);
		gb->ppu.vram[addr - 0x8000] = data;
		return;
	}

	if (addr >= 0xA000 && addr <= 0xBFFF) { // cart ram
		//cart ram
		fprintf(stderr, "  wr @crt ram 0x%04x 0x%02x ", addr, data);
		cart_write(&gb->crt, addr, data);
		return;
	}

	if (addr >= 0xC000 && addr <= 0xDFFF) { //wram
		//wram
		//fprintf(stderr, "  wr @wram 0x%04x 0x%02x ", addr, data);
		fprintf(stderr, "  wr @wram 0x%04x 0x%02x ", addr, data);
		gb->wram[addr - 0xC000] = data;
		return;
	}

	if (addr >= 0xFE00 && addr <= 0xFE9F) { //oam
		//oam
		if ((io_map[LCDC] & 0x80) &&
		    ((gb->ppu.mode == OAM) || (gb->ppu.mode == DRAW))) {
			//dma access allowed
			fprintf(stderr, "Cannot write to OAM. PPU in mode %d\n",
				gb->ppu.mode);
			return;
		}
		fprintf(stderr, "  wr @oam 0x%04x 0x%02x ", addr, data);
		gb->ppu.oam[addr - 0xFE00] = data;
		return;
	}

	if (addr >= 0xFEA0 && addr <= 0xFEFF) //PROHIBITED
		return;

	if ((addr >= 0xFF00 && addr <= 0xFF7F) || (addr == 0xFFFF)) { //io
		io_wr(gb, addr, data);
		return;
	}

	if (addr >= 0xFF80 && addr <= 0xFFFE) { //hram
		//hram
		fprintf(stderr, "  wr @hram 0x%04x 0x%02x ", addr, data);

		gb->hram[addr - 0xFF80] = data;
		return;
	}

	fprintf(stderr, "-- Error writing to 0x%04x (bus.c)\n", addr);
	exit(EXIT_FAILURE);
}
