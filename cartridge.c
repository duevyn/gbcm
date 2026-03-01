#include "cartridge.h"
#include "logger.h"
#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>

enum KiB_sz {
	KiB = 1024,
	KiB8 = KiB << 3,
	KiB16 = KiB << 4,
	KiB32 = KiB << 5,
	KiB256 = KiB << 8,
	KiB512 = KiB << 9,
	MiB = KiB << 10
};

uint8_t mbc0_rd(struct Cartridge *crt, uint16_t addr)
{
	return crt->rom[addr];
}

void mbc0_wr(struct Cartridge *crt, uint16_t addr, uint8_t data)
{
}

uint8_t mbc1_rd(struct Cartridge *crt, uint16_t addr)
{
	if (crt->bk_md && crt->rom_sz >= MiB) {
		fprintf(stderr, "Bank Mode 1 not supported\n");
		exit(EXIT_FAILURE);
	}

	if (addr < 0x4000)
		return crt->rom[addr];

	uint8_t bank;
	if (crt->ram_prms && 0xA000 <= addr && addr <= 0xBFFF) {
		bank = crt->rom_sz >= MiB ? 0 : crt->ram_bk;
		return crt->ram[((addr - 0xA000) + (bank * KiB8)) % KiB8];
	}

	bank = crt->rom_bk & 0x1F;

	if (!bank)
		bank = 1;

	if (crt->rom_sz <= KiB256)
		bank &= (crt->n_rom_bk - 1);
	else if (crt->rom_sz >= MiB)
		bank |= (crt->ram_bk << 5);

	return crt->rom[bank * 0x4000 + (addr - 0x4000)];
}

void mbc1_wr(struct Cartridge *crt, uint16_t addr, uint8_t data)
{
	if (addr <= 0x1FFF) {
		crt->ram_prms = ((data & 0x0A) == 0x0A);
	} else if (addr <= 0x3FFF) {
		crt->rom_bk = data;
	} else if (addr <= 0x5FFF) {
		crt->ram_bk = data & 0x03;
	} else if (addr <= 0x7FFF) {
		crt->bk_md = data;
	} else if (crt->ram_prms && 0xA000 <= addr && addr <= 0xBFFF) {
		uint8_t bank = crt->rom_sz >= MiB ? 0 : crt->ram_bk;
		crt->ram[((addr - 0xA000) + (bank * KiB8)) % KiB8] = data;
	}
}

cart_rd mbc_rd
	[0x1C] = { [0] = mbc0_rd, [1] = mbc1_rd, [2] = mbc1_rd, [3] = mbc1_rd };

cart_wr mbc_wr
	[0x1C] = { [0] = mbc0_wr, [1] = mbc1_wr, [2] = mbc1_wr, [3] = mbc1_wr };

void cart_load(struct Cartridge *crt, const char *path)
{
	int fd = open(path, O_RDONLY);
	if (fd < 0) {
		perror("Error opening file\n");
		exit(EXIT_FAILURE);
	}

	uint8_t header[0x150];
	if (!read(fd, header, 0x150)) {
		exit(EXIT_FAILURE);
	}

	crt->type = header[CART_TYPE];
	if (!mbc_rd[crt->type]) {
		fprintf(stderr, "ERROR: Cartridge type 0x%02x not supported\n",
			crt->type);
		exit(EXIT_FAILURE);
	}

	crt->rom_sz = (KiB * 32) << header[ROM_SZ];
	crt->rom = malloc(crt->rom_sz);

	if (!read(fd, &crt->rom[0x150], crt->rom_sz - 0x150)) {
		perror("Did not read expected bytes\n");
		exit(EXIT_FAILURE);
	}

	memcpy(crt->rom, header, 0x150);
	crt->rom_bk = 0;
	crt->ram_bk = 0;
	crt->bk_md = 0;
	crt->n_rom_bk = crt->rom_sz >> 14;
	crt->n_rg_bits = 63 - __builtin_clzll(crt->n_rom_bk);

	const int ram_sz[] = { 0, 0, 8 * KiB, 32 * KiB, 128 * KiB, 64 * KiB };
	switch (crt->type) {
	case MBC1_RAM:
	case MBC1_RAM_BATTERY:
	case ROM_RAM_11:
	case ROM_RAM_BATTERY_11:
	case MMM01_RAM:
	case MMM01_RAM_BATTERY:
	case MBC3_TIMER_RAM_BATTERY_12:
	case MBC3_RAM_12:
	case MBC3_RAM_BATTERY_12:
	case MBC5_RAM:
	case MBC5_RAM_BATTERY:
	case MBC5_RUMBLE_RAM:
	case MBC5_RUMBLE_RAM_BATTERY:
	case MBC7_SENSOR_RUMBLE_RAM_BATTERY:
	case HuC1_RAM_BATTERY:
		crt->ram_sz = ram_sz[header[RAM_SZ]];
		crt->ram = malloc(crt->ram_sz);
		break;

	default:
		crt->ram_sz = 0;
		crt->ram = NULL;
		break;
	}
	Cartridge c = *crt;
	fprintf(stderr, "rom %lu, ram %lu, type 0x%02x\n", crt->rom_sz / 1000,
		crt->ram_sz, crt->type);
}
