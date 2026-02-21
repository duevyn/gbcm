#include "GameBoy.h"
#include "bus.h"
#include "io.h"
#include "logger.h"
#include <stdio.h>

//#define fprintf(stderr, ...) ((void)0)
uint8_t hndl_interrupts(struct GameBoy *gb)
{
	uint8_t pending = (io_map[IF] & io_ie);
	if (!pending || !gb->cpu.ime)
		return 0;

	gb->cpu.halted = 0;
	gb->cpu.ime = 0;

	uint8_t bit = __builtin_ctz(pending);
	io_map[IF] &= ~(1 << bit);

	gb->cpu.pc--; // move back since next op already fetched
	gb->cpu.instr = NULL;

	bus_write(gb, --gb->cpu.sp, gb->cpu.pc >> 8);
	bus_write(gb, --gb->cpu.sp, gb->cpu.pc & 0xFF);

	uint16_t pc = 0x40 + (8 * bit);
	fprintf(stderr,
		"-- Jump to 0x%04x (pending %b, bit %d, if %b, ret pc 0x%04x)\n\n\n\n",
		pc, pending, bit, io_map[IF], gb->cpu.pc);
	gb->cpu.pc = pc;
	if (logfile) {
		fprintf(logfile,
			"%lu: [INTERRUPT]  >>> CPU JUMP TO VBLANK (PC:0040) <<<\n",
			gb->cpu.cnt);
	}

	return 20; //5 M cycles
}

void gb_emulate(struct GameBoy *gb)
{
	int ticks = 0, tot_ticks = 0, itr_ticks = 0;

	fprintf(stderr,
		"PPU: mode %d, ly %d, ly_dots %d, lydc %08b, stat %08b\n\n",
		gb->ppu.mode, io_map[LY], gb->ppu.ly_dots, io_map[LCDC],
		io_map[STAT]);
	do {
		/*
		if (gb->cpu.stop) {
			tot_ticks += 4;
			continue;
		}
                */

		if (!gb->dma.active)
			ticks = cpu_step(gb);
		else
			ticks = dma_step(gb);

		ppu_step(gb, ticks);
		tmr_step(gb, ticks);

		if (!gb->dma.active && (itr_ticks = hndl_interrupts(gb))) {
			fprintf(stderr,
				"*** ALERT: PPU/timer step for interrupt ");
			ppu_step(gb, itr_ticks);
			tmr_step(gb, itr_ticks);
		}

		tot_ticks += ticks + itr_ticks;
	} while (tot_ticks < DOTS_PER_FRAME);
}

void gb_loadrom(struct GameBoy *gb, const char *path)
{
	cart_load(&gb->crt, path);
	io_init();

	gb->ppu.mode = OAM;

	gb->cpu.af = 0x01b0;
	gb->cpu.bc = 0x0013;
	gb->cpu.de = 0x00d8;
	gb->cpu.hl = 0x014d;
	gb->cpu.sp = 0xfffe;

	gb->cpu.ime = gb->cpu.dblspd = gb->cpu.prefix = false;
	gb->cpu.ime_delay = 0;
	gb->cpu.pc = 0x100;

	gb->cpu.rg[0] = &gb->cpu.b;
	gb->cpu.rg[1] = &gb->cpu.c;
	gb->cpu.rg[2] = &gb->cpu.d;
	gb->cpu.rg[3] = &gb->cpu.e;
	gb->cpu.rg[4] = &gb->cpu.h;
	gb->cpu.rg[5] = &gb->cpu.l;
	gb->cpu.rg[6] = NULL;
	gb->cpu.rg[7] = &gb->cpu.a;

	fprintf(stderr,
		"pc: 0x%04x af 0x%04x (addr 0x%p), a 0x%02x (addr 0x%p), f 0x%02x (addr 0x%p)\n",
		gb->cpu.pc, gb->cpu.af, &gb->cpu.af, gb->cpu.a, &gb->cpu.a,
		gb->cpu.f, &gb->cpu.f);
	fprintf(stderr,
		"hl 0x%04x (addr 0x%p), h 0x%02x (addr 0x%p), l 0x%02x (addr 0x%p)\n",
		gb->cpu.hl, &gb->cpu.hl, gb->cpu.h, &gb->cpu.h, gb->cpu.l,
		&gb->cpu.l);
}
