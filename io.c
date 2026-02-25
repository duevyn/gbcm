#include "io.h"
#include "GameBoy.h"
#include <stdio.h>
#include "logger.h"

//#define fprintf(stderr, ...) ((void)0)
extern inline void io_req_interrupt(enum io_interrupt intrp);

static uint32_t tima_acc;
static uint8_t jp_buttons = 0x0F;
static uint8_t jp_dpad = 0x0F;
static uint8_t jp_ctrl = 0x30;

uint8_t io_map[128] = { 0 };
uint8_t io_ie;
uint16_t io_div16;

void io_init()
{
	io_div16 = (0xAB << 8);
	io_ie = 0;

	io_map[P1] = 0xCF; //FF00
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
	uint8_t output, p1;
	switch (addr) {
	case P1:
		output = 0xCF;

		if (!(io_map[P1] & JOYPAD_DPAD)) {
			output &= jp_dpad;
			if ((jp_dpad != 0x0f))
				fprintf(logfile,
					"DPAD p1=0x%02x, output=0x%02x, jp_ctrl=0x%02x, jp_btn 0x%02x, jp_dpad=0x%02x\n",
					io_map[P1], output, jp_ctrl, jp_buttons,
					jp_dpad);
		}
		if (!(io_map[P1] & JOYPAD_BUTTONS)) {
			output &= jp_buttons;
			if ((jp_buttons != 0x0f))
				fprintf(logfile,
					"BUTTONS p1=0x%02x, output=0x%02x, jp_ctrl=0x%02x, jp_btn 0x%02x, jp_dpad=0x%02x\n",
					io_map[P1], output, jp_ctrl, jp_buttons,
					jp_dpad);
		}

		return output;

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
	addr -= IO_OFFSET;
	uint8_t prev_on, cur_on, writable;

	switch (addr) {
	case P1:
		io_map[P1] = 0xCF | (data & 0x30);
		break;
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
		fprintf(logfile, "%lu (%lu): WR STAT 0x%04x, 0x%02x\n",
			gb->cpu.cnt, gb->cpu.dcnt / 70224, addr + IO_OFFSET,
			data);
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

void io_joypad_press(enum io_joypad_ctrl ctrl, enum io_joypad button)
{
	//io_map[P1] &= ~button;
	if (ctrl == JOYPAD_DPAD)
		jp_dpad &= ~button;
	else
		jp_buttons &= ~button;

	jp_ctrl &= ~ctrl;

	if (!(io_map[P1] & ctrl))
		io_req_interrupt(IO_JOYPAD);

	fprintf(logfile,
		"PRESS p1=0x%02x, ctrl=0x%02x, btn=0x%02x, jp_ctrl=0x%02x, jp_btn 0x%02x, jp_dpad=0x%02x\n",
		io_map[P1], ctrl, button, jp_ctrl, jp_buttons, jp_dpad);
}

void io_joypad_release(enum io_joypad_ctrl ctrl, enum io_joypad button)
{
	if (ctrl == JOYPAD_DPAD)
		jp_dpad |= button;
	else
		jp_buttons |= button;

	jp_ctrl |= ctrl;
	fprintf(logfile,
		"RELEASE p1=0x%02x, ctrl=0x%02x, btn=0x%02x, jp_ctrl=0x%02x, jp_btn 0x%02x, jp_dpad=0x%02x\n",
		io_map[P1], ctrl, button, jp_ctrl, jp_buttons, jp_dpad);
}
