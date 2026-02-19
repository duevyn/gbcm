#include <stdio.h>
#include "logger.h"
#include "bus.h"

static FILE *logfile = NULL;

void log_init(const char *filename)
{
	if (logfile)
		fclose(logfile);
	logfile = fopen(filename, "w");
	// setvbuf(logfile, NULL, _IOFBF, 64 * 1024);
}

void log_cpu_state(GameBoy *gb)
{
	if (!logfile)
		return;

	uint8_t a = gb->cpu.a;
	uint8_t f = gb->cpu.f;
	uint8_t b = gb->cpu.b;
	uint8_t c = gb->cpu.c;
	uint8_t d = gb->cpu.d;
	uint8_t e = gb->cpu.e;
	uint8_t h = gb->cpu.h;
	uint8_t l = gb->cpu.l;
	uint16_t sp = gb->cpu.sp;
	uint16_t pc = gb->cpu.pc;

	uint8_t p0 = bus_read(gb, pc);
	uint8_t p1 = bus_read(gb, (pc + 1) & 0xFFFF);
	uint8_t p2 = bus_read(gb, (pc + 2) & 0xFFFF);
	uint8_t p3 = bus_read(gb, (pc + 3) & 0xFFFF);

	fprintf(logfile,
		"A:%02X F:%02X B:%02X C:%02X D:%02X E:%02X H:%02X L:%02X SP:%04X PC:%04X PCMEM:%02X,%02X,%02X,%02X\n",
		a, f, b, c, d, e, h, l, sp, pc, p0, p1, p2, p3);
}

void log_close(void)
{
	if (logfile) {
		fclose(logfile);
		logfile = NULL;
	}
}
