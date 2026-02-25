#ifndef IO_H
#define IO_H
#include <stdint.h>
#include <stdbool.h>

#define IO_OFFSET 0xFF00U

extern uint8_t io_map[128];
extern uint16_t io_div16;
extern uint8_t io_ie;

enum io_joypad_ctrl { JOYPAD_BUTTONS = 1 << 5, JOYPAD_DPAD = 1 << 4 };

enum io_joypad {
	START$DOWN = (1 << 3),
	SELECT$UP = (1 << 2),
	B$LEFT = (1 << 1),
	A$RIGHT = (1 << 0)
};

struct GameBoy;

void io_init();
void io_timer_step(uint8_t dots);
uint8_t io_rd(uint16_t addr);
void io_wr(struct GameBoy *gb, uint16_t addr, uint8_t data);
void io_joypad_press(enum io_joypad_ctrl ctrl, enum io_joypad button);
void io_joypad_release(enum io_joypad_ctrl ctrl, enum io_joypad button);

enum io_registers {
	P1 = 0xFF00 - IO_OFFSET,
	SB = 0xFF01 - IO_OFFSET,
	SC = 0xFF02 - IO_OFFSET,
	DIV = 0xFF04 - IO_OFFSET,
	TIMA = 0xFF05 - IO_OFFSET,
	TMA = 0xFF06 - IO_OFFSET,
	TAC = 0xFF07 - IO_OFFSET,

	VBK = 0xFF4f - IO_OFFSET,
	SVBK = 0xFF70 - IO_OFFSET,

	IF = 0xFF0F - IO_OFFSET,
	IE = 0xFFFF - IO_OFFSET,

	LCDC = 0xFF40 - IO_OFFSET,
	STAT = 0xFF41 - IO_OFFSET,
	SCY = 0xfF42 - IO_OFFSET,
	SCX = 0xfF43 - IO_OFFSET,
	LY = 0xFF44 - IO_OFFSET,
	LYC = 0xFF45 - IO_OFFSET,
	DMA = 0xFF46 - IO_OFFSET,
	BGP = 0xFF47 - IO_OFFSET,
	OBP0 = 0xFF48 - IO_OFFSET,
	OBP1 = 0xFF49 - IO_OFFSET,
	WY = 0xFF4A - IO_OFFSET,
	WX = 0xFF4b - IO_OFFSET,
	HDMA1 = 0xFF51 - IO_OFFSET,
	HDMA2 = 0xFF52 - IO_OFFSET,
	HDMA3 = 0xFF53 - IO_OFFSET,
	HDMA4 = 0xFF54 - IO_OFFSET,
	HDMA5 = 0xFF55 - IO_OFFSET,
	BCPS = 0xFF68 - IO_OFFSET,
	BCPD = 0xFF69 - IO_OFFSET,
	OCPS = 0xFF6a - IO_OFFSET,
	OCPD = 0xFF6b - IO_OFFSET,
	OPRI = 0xFF6c - IO_OFFSET,

	NR10 = 0xFF10 - IO_OFFSET,
	NR11 = 0xFF11 - IO_OFFSET,
	NR12 = 0xFF12 - IO_OFFSET,
	NR13 = 0xFF13 - IO_OFFSET,
	NR14 = 0xFF14 - IO_OFFSET,
	NR21 = 0xFF16 - IO_OFFSET,
	NR22 = 0xFF17 - IO_OFFSET,
	NR23 = 0xFF18 - IO_OFFSET,
	NR24 = 0xFF19 - IO_OFFSET,
	NR30 = 0xFF1a - IO_OFFSET,
	NR31 = 0xFF1b - IO_OFFSET,
	NR32 = 0xFF1c - IO_OFFSET,
	NR33 = 0xFF1d - IO_OFFSET,
	NR34 = 0xFF1e - IO_OFFSET,
	NR41 = 0xFF20 - IO_OFFSET,
	NR42 = 0xFF21 - IO_OFFSET,
	NR43 = 0xFF22 - IO_OFFSET,
	NR44 = 0xFF23 - IO_OFFSET,
	NR50 = 0xFF24 - IO_OFFSET,
	NR51 = 0xFF25 - IO_OFFSET,
	NR52 = 0xFF26 - IO_OFFSET,

	RP = 0xFF56 - IO_OFFSET,
	KEY0 = 0xFF4c - IO_OFFSET,
	KEY1 = 0xFF4d - IO_OFFSET,

	BANK = 0xFF50 - IO_OFFSET
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
