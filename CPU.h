#ifndef CPU_H
#define CPU_H

#include <stdint.h>
#include <stdbool.h>

#define FLAG_Z 0x80
#define FLAG_N 0x40
#define FLAG_H 0x20
#define FLAG_C 0x10

struct GameBoy;
struct instr;

typedef struct CPU {
	uint16_t op;
	struct instr *instr;
	union {
		uint16_t af;
		struct {
			union {
				uint8_t f; //(lsb)
				struct {
					uint8_t : 4;
					uint8_t fC : 1;
					uint8_t fH : 1;
					uint8_t fN : 1;
					uint8_t fZ : 1;
				};
			};
			uint8_t a; //(msb)
		};
	};
	union {
		uint16_t bc;
		struct {
			uint8_t c; //(lsb)
			uint8_t b; //(msb)
		};
	};
	union {
		uint16_t de;
		struct {
			uint8_t e; //(lsb)
			uint8_t d; //(msb)
		};
	};
	union {
		uint16_t hl;
		struct {
			uint8_t l; //(lsb)
			uint8_t h; //(msb)
		};
	};
	uint16_t pc, sp;
	bool ime, dblspd, prefix, halted;
	int ime_delay;

} CPU;

typedef struct instr {
	const char *mnem;
	uint8_t (*exec)(struct GameBoy *gb);
	uint8_t len;
	uint8_t dots[2];
	uint8_t op;
} instr;

void cpu_ldrom(struct CPU *cpu, char *rom);
int cpu_step(struct GameBoy *gb);
#endif
