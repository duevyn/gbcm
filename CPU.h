#ifndef CPU_H
#define CPU_H

#include <stdint.h>
#include <stdbool.h>

struct instr;
typedef enum fl_rgf {
	C = 0b00010000,
	H = 0b00100000,
	N = 0b01000000,
	Z = 0b10000000,
} fl_rgf;

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
	bool ime, dblspd, prefix;
	struct ctdg *cg;
	uint8_t *rom;

} CPU;

typedef struct instr {
	const char *mnem;
	uint8_t (*exec)(struct CPU *cpu);
	uint8_t len;
	uint8_t dots[2];
	uint8_t op;
} instr;

void cpu_ldrom(struct CPU *cpu, char *rom);
int cpu_step(struct CPU *cpu);
#endif
