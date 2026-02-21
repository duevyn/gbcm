#ifndef IO_H
#define IO_H
#include <stdint.h>

#define IO_OFF 0xFF00U

extern uint8_t io_map[128];
extern uint16_t internal_div;
extern uint8_t io_ie;

struct GameBoy;

void io_init();
uint8_t io_rd(uint16_t addr);
void io_wr(struct GameBoy *gb, uint16_t addr, uint8_t data);

enum io_registers {
	P1 = 0xFF00 - IO_OFF,
	SB = 0xFF01 - IO_OFF,
	SC = 0xFF02 - IO_OFF,
	DIV = 0xFF04 - IO_OFF,
	TIMA = 0xFF05 - IO_OFF,
	TMA = 0xFF06 - IO_OFF,
	TAC = 0xFF07 - IO_OFF,

	VBK = 0xFF4f - IO_OFF,
	SVBK = 0xFF70 - IO_OFF,

	IF = 0xFF0F - IO_OFF,
	IE = 0xFFFF - IO_OFF,

	LCDC = 0xFF40 - IO_OFF,
	STAT = 0xFF41 - IO_OFF,
	SCY = 0xfF42 - IO_OFF,
	SCX = 0xfF43 - IO_OFF,
	LY = 0xFF44 - IO_OFF,
	LYC = 0xFF45 - IO_OFF,
	DMA = 0xFF46 - IO_OFF,
	BGP = 0xFF47 - IO_OFF,
	OBP0 = 0xFF48 - IO_OFF,
	OBP1 = 0xFF49 - IO_OFF,
	WY = 0xFF4A - IO_OFF,
	WX = 0xFF4b - IO_OFF,
	HDMA1 = 0xFF51 - IO_OFF,
	HDMA2 = 0xFF52 - IO_OFF,
	HDMA3 = 0xFF53 - IO_OFF,
	HDMA4 = 0xFF54 - IO_OFF,
	HDMA5 = 0xFF55 - IO_OFF,
	BCPS = 0xFF68 - IO_OFF,
	BCPD = 0xFF69 - IO_OFF,
	OCPS = 0xFF6a - IO_OFF,
	OCPD = 0xFF6b - IO_OFF,
	OPRI = 0xFF6c - IO_OFF,

	NR10 = 0xFF10 - IO_OFF,
	NR11 = 0xFF11 - IO_OFF,
	NR12 = 0xFF12 - IO_OFF,
	NR13 = 0xFF13 - IO_OFF,
	NR14 = 0xFF14 - IO_OFF,
	NR21 = 0xFF16 - IO_OFF,
	NR22 = 0xFF17 - IO_OFF,
	NR23 = 0xFF18 - IO_OFF,
	NR24 = 0xFF19 - IO_OFF,
	NR30 = 0xFF1a - IO_OFF,
	NR31 = 0xFF1b - IO_OFF,
	NR32 = 0xFF1c - IO_OFF,
	NR33 = 0xFF1d - IO_OFF,
	NR34 = 0xFF1e - IO_OFF,
	NR41 = 0xFF20 - IO_OFF,
	NR42 = 0xFF21 - IO_OFF,
	NR43 = 0xFF22 - IO_OFF,
	NR44 = 0xFF23 - IO_OFF,
	NR50 = 0xFF24 - IO_OFF,
	NR51 = 0xFF25 - IO_OFF,
	NR52 = 0xFF26 - IO_OFF,

	RP = 0xFF56 - IO_OFF,
	KEY0 = 0xFF4c - IO_OFF,
	KEY1 = 0xFF4d - IO_OFF,

	BANK = 0xFF50 - IO_OFF
};

enum io_interrupt {
	IO_VBLANK = 0,
	IO_STAT,
	IO_TIMER,
	IO_SERIAL,
	IO_JOYPAD,
};

inline void io_req_interrupt(enum io_interrupt intrp)
{
	io_map[IF] |= (1U << intrp);
}

#endif
