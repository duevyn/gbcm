#ifndef BUS_H
#define BUS_H

#include <stdint.h>

struct GameBoy;

uint8_t bus_read(struct GameBoy *gb, uint16_t addr);
void bus_write(struct GameBoy *gb, uint16_t addr, uint8_t data);

#endif
