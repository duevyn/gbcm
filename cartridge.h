#ifndef CARTRIDGE_H
#define CARTRIDGE_H

#include <stdint.h>
#include <stdlib.h>
#include <stdbool.h>

typedef struct Cartridge {
	uint8_t *rom_data;
	size_t rom_size;

	uint8_t *cart_ram;
	size_t ram_size;

	struct {
		uint8_t rom_bank;
		uint8_t ram_bank;
		bool ram_enabled;
		uint8_t mode;
	} mbc;

} Cartridge;

uint8_t cart_read(struct Cartridge *crt, uint16_t addr);
void cart_write(struct Cartridge *crt, uint16_t addr, uint8_t data);
void cart_load(struct Cartridge *crt, const char *path);

#endif
