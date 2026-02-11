#include "GameBoy.h"
#include <stdio.h>

void hndl_interrupts()
{
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
	int ticks = 0, tot_ticks = 0;

	gb->ppu.mode = OAM;
	gb->ppu.ly = 0;
	do {
		ticks = cpu_step(gb);
		ppu_step(gb, ticks);
		hndl_interrupts();
		tot_ticks += ticks;
	} while (tot_ticks < DOTS_PER_FRAME);
}

void gb_loadrom(struct GameBoy *gb, const char *path)
{
	cart_load(&gb->crt, path);

	gb->cpu.af = 0x11b0;
	gb->cpu.bc = 0x0013;
	gb->cpu.de = 0x00d8;
	gb->cpu.hl = 0x014d;
	gb->cpu.sp = 0xfffe;

	// pan doc values. assume these are correct but verify against values above
	// after investigating. values above are dmg defaults.
	gb->cpu.bc = 0x0013; //cgb dmgmode 0x0000
	gb->cpu.de = 0xff56; //cgb dmg mode 0x0008
	gb->cpu.hl = 0x000d; // cgb dmg ????

	gb->cpu.ime = gb->cpu.dblspd = gb->cpu.prefix = false;
	gb->cpu.pc = 0x150; // assume valid rom. (gb->cpu.pc = 0x100)

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
