#include "bus.h"
#include "GameBoy.h"
#include "DMA.h"
#include "io.h"
#include <stdio.h>
#include "logger.h"

#define fprintf(stderr, ...) ((void)0)

uint8_t bus_read(struct GameBoy *gb, uint16_t addr)
{
	if (addr == (P1 + IO_OFFSET))
		fprintf(logfile,
			"%lu (%lu):rd P1 ctrl=0x%02x, btns=%02x, dpad=%02x\n",
			gb->cpu.cnt, gb->cpu.dcnt / 70224, joypad_ctrl,
			joypad_dpad, joypad_buttons);
	if (addr < 0x8000)
		return mbc_rd[gb->crt.type](&gb->crt, addr);

	if (addr >= 0x8000 && addr < 0xA000) {
		if (!gb->dma.active && (io_map[LCDC] & 0x80) &&
		    (gb->ppu.mode == DRAW)) {
			fprintf(logfile,
				"%lu (%lu): ALERT Cannot RD VRAM. PPU in mode %d\n",
				gb->cpu.cnt, gb->cpu.dcnt / 70224,
				gb->ppu.mode);
			return 0xFF;
		}
		return gb->ppu.vram[addr - 0x8000];
	}

	if (addr >= 0xA000 && addr <= 0xBFFF) //cart ram
		return mbc_rd[gb->crt.type](&gb->crt, addr);

	if (addr >= 0xC000 && addr <= 0xDFFF) //wram
		return gb->wram[addr - 0xC000];

	if (addr >= 0xFE00 && addr <= 0xFE9F) {
		//oam
		if (!gb->dma.active && (io_map[LCDC] & 0x80) &&
		    (gb->ppu.mode == OAM || gb->ppu.mode == DRAW)) {
			//dma access allowed
			fprintf(logfile,
				"%lu (%lu): ALERT Cannot RD OAM. PPU in mode %d\n",
				gb->cpu.cnt, gb->cpu.dcnt / 70224,
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
	if (addr < 0x8000) { //rom
		mbc_wr[gb->crt.type](&gb->crt, addr, data);
	} else if (addr >= 0x8000 && addr < 0xA000) { //vram
		if ((io_map[LCDC] & 0x80) && (gb->ppu.mode == DRAW)) {
			fprintf(logfile,
				"%lu (%lu): ALERT Cannot write to VRAM. PPU in mode %d\n",
				gb->cpu.cnt, gb->cpu.dcnt / 70224,
				gb->ppu.mode);
			return;
		}
		gb->ppu.vram[addr - 0x8000] = data;
	} else if (addr >= 0xA000 && addr <= 0xBFFF) { // cart ram
		mbc_wr[gb->crt.type](&gb->crt, addr, data);
	} else if (addr >= 0xC000 && addr <= 0xDFFF) { //wram
		gb->wram[addr - 0xC000] = data;
	} else if (addr >= 0xFE00 && addr <= 0xFE9F) { //oam
		if ((io_map[LCDC] & 0x80) &&
		    ((gb->ppu.mode == OAM) || (gb->ppu.mode == DRAW))) {
			fprintf(logfile,
				"%lu (%lu): ALERT Cannot write to OAM. PPU in mode %d\n",
				gb->cpu.cnt, gb->cpu.dcnt / 70224,
				gb->ppu.mode);
			return;
		}
		gb->ppu.oam[addr - 0xFE00] = data;
	} else if ((addr >= 0xFF00 && addr <= 0xFF7F) || (addr == 0xFFFF)) {
		io_wr(gb, addr, data);
	} else if (addr >= 0xFF80 && addr <= 0xFFFE) { //hram
		gb->hram[addr - 0xFF80] = data;
	}

	fprintf(stderr, "-- Error writing to 0x%04x (bus.c)\n", addr);
}
