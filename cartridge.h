#ifndef CARTRIDGE_H
#define CARTRIDGE_H

#include <stdint.h>
#include <stdlib.h>
#include <stdbool.h>
#include <stdio.h>

enum i_cart_head {
	TITL = 0x134,
	CART_TYPE = 0x147,
	ROM_SZ = 0x148,
	RAM_SZ = 0x149
};

enum cart_type {
	ROM_ONLY = 0x00,
	MBC1 = 0x01,
	MBC1_RAM = 0x02,
	MBC1_RAM_BATTERY = 0x03,
	MBC2 = 0x05,
	MBC2_BATTERY = 0x06,
	ROM_RAM_11 = 0x08,
	ROM_RAM_BATTERY_11 = 0x09,
	MMM01 = 0x0B,
	MMM01_RAM = 0x0C,
	MMM01_RAM_BATTERY = 0x0D,
	MBC3_TIMER_BATTERY = 0x0F,
	MBC3_TIMER_RAM_BATTERY_12 = 0x10,
	MBC3 = 0x11,
	MBC3_RAM_12 = 0x12,
	MBC3_RAM_BATTERY_12 = 0x13,
	MBC5 = 0x19,
	MBC5_RAM = 0x1A,
	MBC5_RAM_BATTERY = 0x1B,
	MBC5_RUMBLE = 0x1C,
	MBC5_RUMBLE_RAM = 0x1D,
	MBC5_RUMBLE_RAM_BATTERY = 0x1E,
	MBC6 = 0x20,
	MBC7_SENSOR_RUMBLE_RAM_BATTERY = 0x22,
	POCKET_CAMERA = 0xFC,
	BANDAI_TAMA5 = 0xFD,
	HuC3 = 0xFE,
	HuC1_RAM_BATTERY = 0xFF
};

typedef struct Cartridge {
	uint8_t *rom;
	size_t rom_sz;

	uint8_t *ram;
	size_t ram_sz;

	uint8_t rom_bk;
	uint8_t ram_bk;
	bool ram_prms;
	uint8_t bk_md;
	uint8_t n_rom_bk;
	uint8_t n_rg_bits;
	uint8_t rtc;
	char titl[17];
	bool ram_dirty;

	enum cart_type type;

} Cartridge;

typedef uint8_t (*cart_rd)(struct Cartridge *c, uint16_t addr);
typedef void (*cart_wr)(struct Cartridge *c, uint16_t addr, uint8_t data);
extern cart_rd mbc_rd[];
extern cart_wr mbc_wr[];

void crt_load(struct Cartridge *c, const char *pth);
void crt_eject(struct Cartridge *c);
void crt_sv_ram(struct Cartridge *c);

#endif
