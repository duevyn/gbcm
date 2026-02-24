#include "io.h"
#include "GameBoy.h"
#include <stdio.h>
#include "logger.h"

//#define fprintf(stderr, ...) ((void)0)
extern inline void io_req_interrupt(enum io_interrupt intrp);
static uint32_t tima_acc;

uint8_t io_map[128] = { 0 };
uint8_t io_ie;
uint16_t io_div16;

void io_init()
{
	io_div16 = (0xAB << 8);
	io_ie = 0;

	io_map[SB] = 0x00; //0xFF01
	io_map[SC] = 0x7E; //0xFF02
	io_map[TIMA] = 0; //0xFF05
	io_map[TMA] = 0; //0xFF06
	io_map[TAC] = 0xF8; //0xFF07
	io_map[IF] = 0xE1; //0xFF0F
	io_map[LCDC] = 0x91; //0xFF40
	io_map[STAT] = 0x85; //0xFF41
	io_map[SCY] = 0x00; //0xFF42
	io_map[SCX] = 0x00; //0xFF43
	io_map[LY] = 0x00; //0xFF44
	io_map[LYC] = 0x00; //0xFF45
	io_map[DMA] = 0xff; //0xFF46
	io_map[BGP] = 0xfc; //0xFF47
	io_map[WY] = 0x00; //0xFF4A
	io_map[WX] = 0x00; //0xFF4B
}

uint8_t io_rd(uint16_t addr)
{
	addr -= IO_OFFSET;
	fprintf(stderr, "  RD @io 0x%04x 0x%02x  ", addr + IO_OFFSET,
		io_map[addr]);

	switch (addr) {
	case P1:
		return 0xCF;
	case DIV:
		return io_div16 >> 8;
	case LY:
		//return 0x90; // gameboy doctor
		return io_map[LY];
	case IE:
		return io_ie;
	default:
		return io_map[addr];
	}
}

void io_wr(struct GameBoy *gb, uint16_t addr, uint8_t data)
{
	fprintf(stderr, "  WR @io 0x%04x 0x%02x  ", addr, data);
	addr -= IO_OFFSET;
	uint8_t prev_on, cur_on, writable;

	switch (addr) {
	case DIV:
		tima_acc = 0;
		io_div16 = 0;
		break;
	case LCDC:
		prev_on = (io_map[LCDC] & 0x80);
		cur_on = (data & 0x80);
		if (!prev_on && (prev_on ^ cur_on)) { //off to on
			io_map[LY] = 0;
			gb->ppu.mode = OAM;
			gb->ppu.ly_dots = 0;
		}
		io_map[LCDC] = data;
		break;

	case STAT:
		writable = data & 0x78; // bits 3-6
		io_map[STAT] = (io_map[STAT] & 0x87) | writable;
		break;
	case LY: //read only
		break;
	case DMA:
		io_map[DMA] = data;
		dma_start(gb, data);
		break;
	case IE:
		io_ie = data;
		break;
	default:
		io_map[addr] = data;
		break;
	}
}
void io_timer_step(uint8_t dots)
{
	io_div16 += dots;
	if (!(io_map[TAC] & 0x04))
		return;

	static const int freq_table[4] = { 1024, 16, 64, 256 };

	int period = freq_table[io_map[TAC] & 0x03];
	tima_acc += dots;

	while (tima_acc >= period) {
		tima_acc -= period;
		io_map[TIMA]++;

		if (io_map[TIMA] == 0) {
			io_map[TIMA] = io_map[TMA];
			io_req_interrupt(IO_TIMER);
		}
	}
}
