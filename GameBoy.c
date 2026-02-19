#include "GameBoy.h"
#include "bus.h"
#include <stdio.h>

//#define fprintf(stderr, ...) ((void)0)
uint8_t hndl_interrupts(struct GameBoy *gb)
{
	uint8_t pending = gb->if_reg & gb->ie;
	if (!pending || !gb->cpu.ime)
		return 0;

	fprintf(stderr,
		"\n\n\n\n\n$$$$$$$$$$ interrupt. ime %08b if %08b ie %08b stat %08b ",
		gb->cpu.ime, gb->if_reg, gb->ie, gb->ppu.stat);

	gb->cpu.halted = 0;
	gb->cpu.ime = 0;

	uint8_t bit = __builtin_ctz(pending);
	gb->if_reg &= ~(1 << bit);

	gb->cpu.pc--; // move back since next op already fetched
	gb->cpu.instr = NULL;

	bus_write(gb, --gb->cpu.sp, gb->cpu.pc >> 8);
	bus_write(gb, --gb->cpu.sp, gb->cpu.pc & 0xFF);

	static const uint16_t vectors[5] = {
		0x40, // VBlank
		0x48, // STAT
		0x50, // Timer
		0x58, // Serial
		0x60 // Joypad
	};

	gb->cpu.pc = vectors[bit];
	fprintf(stderr, "-- Jump to 0x%04x (pending %b, bit %d, if %b)\n\n\n\n",
		gb->cpu.pc, pending, bit, gb->if_reg);

	return 20; //5 M cycles
}

void initreg(struct GameBoy *gb)
{
	//gb->cpu.rom[P1] = 0xcf;
	//gb->cpu.rom[SB] = 0x00;
	gb->sb = 0x00;
	gb->sc = 0x7e;
	//gb->cpu.rom[SC] = 0x7e;
	//gb->cpu.rom[DIV] = 0x18;

	//gb->cpu.rom[TIMA] = 0;
	//gb->cpu.rom[TMA] = 0;
	//gb->cpu.rom[TAC] = 0xf8;
	//gb->cpu.rom[IF] = 0xe1;
	gb->if_reg = 0xe1;

	//gb->cpu.rom[NR10] = 0x80;
	//gb->cpu.rom[NR11] = 0xbf;
	//gb->cpu.rom[NR12] = 0xf3;
	//gb->cpu.rom[NR13] = 0xff;
	//gb->cpu.rom[NR14] = 0xbf;
	//gb->cpu.rom[NR21] = 0x3f;
	//gb->cpu.rom[NR22] = 0x00;
	//gb->cpu.rom[NR23] = 0xff;
	//gb->cpu.rom[NR24] = 0xbf;

	//gb->cpu.rom[NR30] = 0x7f;
	//gb->cpu.rom[NR31] = 0xff;
	//gb->cpu.rom[NR32] = 0x9f;
	//gb->cpu.rom[NR33] = 0xff;
	//gb->cpu.rom[NR34] = 0xbf;
	//gb->cpu.rom[NR41] = 0xff;
	//gb->cpu.rom[NR42] = 0x00;
	//gb->cpu.rom[NR43] = 0x00;
	//gb->cpu.rom[NR44] = 0xbf;
	//gb->cpu.rom[NR50] = 0x77;
	//gb->cpu.rom[NR51] = 0xf3;
	//gb->cpu.rom[NR52] = 0xf1;

	//gb->cpu.rom[LCDC] = 0x91;
	//gb->cpu.rom[STAT] = 0x81;
	//gb->cpu.rom[SCY] = 0x00;
	//gb->cpu.rom[SCX] = 0x00;
	//gb->cpu.rom[LY] = 0x91;
	//gb->cpu.rom[LYC] = 0x00;
	//gb->cpu.rom[DMA] = 0xff;
	//gb->cpu.rom[BGP] = 0xfc;
	//gb->cpu.rom[WY] = 0x00;
	//gb->cpu.rom[WX] = 0x00;
	//gb->cpu.rom[IE] = 0x00;

	gb->ppu.lcdc = 0x91;
	gb->ppu.stat = 0x81;
	gb->ppu.scy = 0x00;
	gb->ppu.scx = 0x00;
	gb->ppu.ly = 0x91;
	gb->ppu.lyc = 0x00;
	gb->ppu.dma = 0xff;
	gb->ppu.bgp = 0xfc;
	gb->ppu.wy = 0x00;
	gb->ppu.wx = 0x00;

	gb->ie = 0x00;
}

void initreg_cgb(struct CPU *cpu)
{
	// overwrite dmg values
	//gb->cpu.rom_data[SC] = 0x7f;
	//gb->cpu.rom_data[DMA] = 0;

	// exclusive cgb
	//gb->cpu.rom_data[KEY1] = 0x7e;
	//gb->cpu.rom_data[VBK] = 0xfe;
	//gb->cpu.rom_data[HDMA1] = 0xff;
	//gb->cpu.rom_data[HDMA2] = 0xff;
	//gb->cpu.rom_data[HDMA3] = 0xff;
	//gb->cpu.rom_data[HDMA4] = 0xff;
	//gb->cpu.rom_data[HDMA5] = 0xff;
	//gb->cpu.rom_data[RP] = 0x3e;
	//gb->cpu.rom_data[SVBK] = 0xf8;
}

void gb_emulate(struct GameBoy *gb)
{
	int ticks = 0, tot_ticks = 0, itr_ticks = 0;

	fprintf(stderr,
		"PPU: mode %d, ly %d, ly_dots %d, lydc %08b, stat %08b\n\n",
		gb->ppu.mode, gb->ppu.ly, gb->ppu.ly_dots, gb->ppu.lcdc,
		gb->ppu.stat);
	do {
		if (gb->cpu.stop) {
			tot_ticks += 4;
			continue;
		}

		if (!gb->dma.active)
			ticks = cpu_step(gb);
		else
			ticks = dma_step(gb);

		ppu_step(gb, ticks);

		if (!gb->dma.active && (itr_ticks = hndl_interrupts(gb))) {
			fprintf(stderr, "*** ALERT: PPU step for interrupt ");
			ppu_step(gb, itr_ticks);
		}

		tot_ticks += ticks + itr_ticks;
	} while (tot_ticks < DOTS_PER_FRAME);
}

void gb_loadrom(struct GameBoy *gb, const char *path)
{
	cart_load(&gb->crt, path);

	gb->cpu.af = 0x01b0;
	//gb->cpu.af = 0x11b0;
	gb->cpu.bc = 0x0013;
	gb->cpu.de = 0x00d8;
	gb->cpu.hl = 0x014d;
	gb->cpu.sp = 0xfffe;

	// pan doc values. assume these are correct but verify against values above
	// after investigating. values above are dmg defaults.
	gb->cpu.bc = 0x0013; //cgb dmgmode 0x0000
	//gb->cpu.de = 0xff56; //cgb dmg mode 0x0008
	//jgb->cpu.hl = 0x000d; // cgb dmg ????

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

	initreg(gb);
	//initreg_cgb(&gb->cpu);
}
