#ifndef REGISTERS_H
#define REGISTERS_H

// Port/mode
#define P1 0xff00 //joypad input
#define SB 0xff01 //serial data transer
#define SC 0xff02
#define DIV 0xff04
#define TIMA 0xff05
#define TMA 0xff06
#define TAC 0xff07

// bank control
#define VBK 0xff4f
#define SVBK 0xff70

// interrupt flags
#define IF 0xff0f
#define IE 0xffff
#define IME -1

// lcd display
#define LCDC 0xff40
#define STAT 0xff41
#define SCY 0xff42
#define SCX 0xff43
#define LY 0xff44
#define LYC 0xff45
#define DMA 0xff46
#define BGP 0xff47
#define OBP0 0xff48
#define OBP1 0xff49
#define WY 0xff4a
#define WX 0xff4b
// lcd display gameboy color
#define HDMA1 0xff51
#define HDMA2 0xff52
#define HDMA3 0xff53
#define HDMA4 0xff54
#define HDMA5 0xff55
#define BCPS 0xff68
#define BCPD 0xff69
#define OCPS 0xff6a
#define OCPD 0xff6b
#define OPRI 0xff6c //object priority mode. 0:cgb, 1:dmg

// sound
#define NR10 0xff10
#define NR11 0xff11
#define NR12 0xff12
#define NR13 0xff13
#define NR14 0xff14
#define NR21 0xff16
#define NR22 0xff17
#define NR23 0xff18
#define NR24 0xff19
#define NR30 0xff1a
#define NR31 0xff1b
#define NR32 0xff1c
#define NR33 0xff1d
#define NR34 0xff1e
#define NR41 0xff20
#define NR42 0xff21
#define NR43 0xff22
#define NR44 0xff23
#define NR50 0xff24
#define NR51 0xff25
#define NR52 0xff26

// gameboy color
#define RP \
	0xff56 //infrared comm port. byte 76 read enable -> 0:disab, 3:enable. byte 1: recieving -> 0:rcv ir sig, 1:nrm. byte 0: emitting -> 0:led off, 1:on.
#define KEY0 \
	0xff4c //written only by cgb boot rom. locked until reset. 0:full cgb; 1:dmg
#define KEY1 \
	0xff4d // switch b/w double speed mode. byte 7: current speed -> 0:nrm, 1:dbl. byte 0: switch armed -> 0:no, 1:armed

//misc
#define BANK 0xff50 //unmap boot rom

#endif
