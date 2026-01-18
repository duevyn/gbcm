#ifndef CPU_H
#define CPU_H

#include <stdint.h>

typedef struct CPU {
	uint16_t op;
} CPU;

typedef struct instr {
	const char *mnem;
	uint8_t len;
	uint8_t dots;
	void (*exec)(struct CPU *cpu);
} instr;

int cpu_step(struct CPU *cpu);
#endif
