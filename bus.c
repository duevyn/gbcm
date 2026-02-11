#include "bus.h"
#include "GameBoy.h"
#include <stdio.h>

//#define fprintf(stderr, ...) ((void)0)
uint8_t io_read(struct GameBoy *gb, uint16_t addr)
{
	uint8_t data;
	switch (addr) {
	case 0xFF01:
		data = gb->sb;
		fprintf(stderr, "  RD @io SB 0x%04x 0x%02x", addr, data);
		return data;
	case 0xFF02:
		data = gb->sc;
		fprintf(stderr, "  RD @io SC 0x%04x 0x%02x", addr, data);
		return data;
	case 0xFF0F:
		data = gb->if_reg;
		fprintf(stderr, "  RD @io IF 0x%04x 0x%02x", addr, data);
		return data;
	case 0xFF40:
		data = gb->ppu.lcdc;
		fprintf(stderr, "  RD @io LCDC 0x%04x 0x%02x", addr, data);
		return data;
	case 0xFF41:
		data = gb->ppu.stat;
		fprintf(stderr, "  RD @io STAT 0x%04x 0x%02x", addr, data);
		return data;
	case 0xFF42:
		data = gb->ppu.scy;
		fprintf(stderr, "  RD @io SCY 0x%04x 0x%02x", addr, data);
		return data;

	case 0xFF43:
		data = gb->ppu.scx;
		fprintf(stderr, "  RD @io SCX 0x%04x 0x%02x", addr, data);
		return data;

	case 0xFF44:
		data = gb->ppu.ly;
		fprintf(stderr, "  RD @io LY 0x%04x 0x%02x", addr, data);
		return data;
	case 0xFF47:
		data = gb->ppu.bgp;
		fprintf(stderr, "  RD @io BGP 0x%04x 0x%02x", addr, data);
		return data;
	case 0xFF48:
		data = gb->ppu.obp0;
		fprintf(stderr, "  RD @io OBP0 0x%04x 0x%02x", addr, data);
		return data;
	case 0xFF49:
		data = gb->ppu.obp1;
		fprintf(stderr, "  RD @io OBP1 0x%04x 0x%02x", addr, data);
		return data;
	case 0xFF4A:
		data = gb->ppu.wy;
		fprintf(stderr, "  RD @io WY 0x%04x 0x%02x", addr, data);
		return data;
	case 0xFF4B:
		data = gb->ppu.wx;
		fprintf(stderr, "  RD @io WX 0x%04x 0x%02x", addr, data);
		return data;
	case 0xFFFF:
		data = gb->ie;
		fprintf(stderr, "  RD @io IE 0x%04x 0x%02x", addr, data);
		return data;
	default:
		fprintf(stderr,
			"  ALERT: rd @IO 0x%04x 0x%02x NOT IMPLEMENTED ", addr,
			data);
		break;
	}
	return -1;
}

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
	if (addr >= 0xFEA0 && addr <= 0xFEFF) { //PROHIBITED
		fprintf(stderr, "  ALERT: noop RD @PROHIBITED 0x%04x ", addr);
		return 0x00;
	}
	if (addr >= 0xFF00 && addr <= 0xFF7F) {
		//io
		return io_read(gb, addr);
	}
	if (addr >= 0xFF80 && addr <= 0xFFFE) {
		//hram
		return gb->hram[addr - 0xFF80];
	}

	return 0xFF;
}

void io_write(struct GameBoy *gb, uint16_t addr, uint8_t data)
{
	switch (addr) {
	case 0xFF01:
		gb->sb = data;
		fprintf(stderr, "  WR @io SB 0x%04x 0x%02x ", addr, data);
		break;
	case 0xFF02:
		gb->sc = data;
		fprintf(stderr, "  WR @io SC 0x%04x 0x%02x ", addr, data);
		break;

	case 0xFF0F:
		gb->if_reg = data;
		fprintf(stderr, "  WR @io IF 0x%04x 0x%02x ", addr, data);
		break;
	case 0xFF40:
		gb->ppu.lcdc = data;
		fprintf(stderr, "  WR @io LCDC 0x%04x 0x%02x ", addr, data);
		break;
	case 0xFF41:
		gb->ppu.stat = data;
		fprintf(stderr, "  WR @io STAT  0x%04x 0x%02x ", addr, data);
		break;
	case 0xFF42:
		gb->ppu.scy = data;
		fprintf(stderr, "  WR @io SCY 0x%04x 0x%02x ", addr, data);
		break;

	case 0xFF43:
		gb->ppu.scx = data;
		fprintf(stderr, "  WR @io SCX 0x%04x 0x%02x ", addr, data);
		break;

	case 0xFF44:
		fprintf(stderr, "  ALERT: noop wr @io LY 0x%04x 0x%02x ", addr,
			data);
		break;
	case 0xFF47:
		gb->ppu.bgp = data;
		fprintf(stderr, "  WR @io BGP 0x%04x 0x%02x ", addr, data);
		break;
	case 0xFF48:
		gb->ppu.obp0 = data;
		fprintf(stderr, "  WR @io OBP0 0x%04x 0x%02x ", addr, data);
		break;
	case 0xFF49:
		gb->ppu.obp1 = data;
		fprintf(stderr, "  WR @io OBP1 0x%04x 0x%02x ", addr, data);
		break;
	case 0xFF4A:
		gb->ppu.wy = data;
		fprintf(stderr, "  WR @io WY 0x%04x 0x%02x ", addr, data);
		break;
	case 0xFF4B:
		gb->ppu.wx = data;
		fprintf(stderr, "  WR @io WX 0x%04x 0x%02x ", addr, data);
		break;
	case 0xFFFF:
		gb->ie = data;
		fprintf(stderr, "  WR @io IE 0x%04x 0x%02x ", addr, data);
		break;
	default:
		fprintf(stderr,
			"  ALERT: wr @IO 0x%04x 0x%02x NOT IMPLEMENTED ", addr,
			data);
		break;
	}
}

void bus_write(struct GameBoy *gb, uint16_t addr, uint8_t data)
{
	if (addr < 0x8000) { //rom
		cart_write(&gb->crt, addr, data);
		fprintf(stderr, "  ALERT wr @rom 0x%04x 0x%02x ", addr, data);
		return;
	}

	if (addr >= 0x8000 && addr < 0xA000) { //vram
		if (gb->ppu.mode == DRAW) {
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
		gb->wram[addr - 0xC000] = data;
		return;
	}
	if (addr >= 0xFE00 && addr <= 0xFE9F) { //oam
		//oam
		if (gb->ppu.mode == OAM || gb->ppu.mode == DRAW) {
			//dma access allowed
			fprintf(stderr, "Cannot write to OAM. PPU in mode %d\n",
				gb->ppu.mode);
			return;
		}
		fprintf(stderr, "  wr @oam 0x%04x 0x%02x ", addr, data);
		gb->ppu.oam[addr - 0xFE00] = data;
		return;
	}
	if (addr >= 0xFEA0 && addr <= 0xFEFF) { //PROHIBITED
		fprintf(stderr, "  ALERT: noop wr @PROHIBITED 0x%04x 0x%02x ",
			addr, data);
		return;
	}
	if (addr >= 0xFF00 && addr <= 0xFF7F || addr == 0xFFFF) { //io
		io_write(gb, addr, data);
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
