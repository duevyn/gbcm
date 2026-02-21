#ifndef LOGGER_H
#define LOGGER_H

#include "GameBoy.h"
#include <stdio.h>

extern FILE *logfile;

void log_init(const char *filename);
void log_cpu_state(GameBoy *gb);
void log_close(void);

#endif
