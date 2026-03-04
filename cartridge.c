#include "cartridge.h"
#include "logger.h"
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/types.h>

enum KiB_sz {
	KiB = 1024,
	KiB8 = KiB << 3,
	KiB16 = KiB << 4,
	KiB32 = KiB << 5,
	KiB256 = KiB << 8,
	KiB512 = KiB << 9,
	MiB = KiB << 10
};

static const char *dir = "gbcm_data";
FILE *crt_mem = NULL;

void load_mem(struct Cartridge *c)
{
	char path[27];
	sprintf(path, "%s/%s", dir, c->titl);
	struct stat st = { 0 };
	if (stat(dir, &st) == -1) {
		if (mkdir(dir, 0777) == -1) {
			fprintf(stderr, "ERROR making path %s: ", dir);
			perror("");
			return;
		}
	}

	if (!S_ISDIR(st.st_mode)) {
		fprintf(stderr, "ERROR: path %s is not directory\n", dir);
		return;
	}

	if ((crt_mem = fdopen(open(path, O_RDWR | O_CREAT), "r+"))) {
		fread(c->ram, 1, c->ram_sz, crt_mem);
		fprintf(stderr, "Loaded save data for %s\n", c->titl);
	}
}

void crt_sv_ram(struct Cartridge *c)
{
	if (!crt_mem)
		return;

	rewind(crt_mem);
	fwrite(c->ram, sizeof(uint8_t), c->ram_sz, crt_mem);
	fflush(crt_mem);
	c->ram_dirty = false;
	fprintf(stderr, "Saved data for  %s\n", c->titl);
}

uint8_t mbc0_rd(struct Cartridge *crt, uint16_t addr)
{
	return crt->rom[addr];
}

void mbc0_wr(struct Cartridge *crt, uint16_t addr, uint8_t data)
{
}

uint8_t mbc1_rd(struct Cartridge *c, uint16_t addr)
{
	if (c->bk_md && c->rom_sz >= MiB) {
		fprintf(stderr, "Bank Mode 1 not supported\n");
		exit(EXIT_FAILURE);
	}

	if (addr < 0x4000)
		return c->rom[addr];

	uint8_t bank;
	if (c->ram_prms && 0xA000 <= addr && addr <= 0xBFFF) {
		bank = c->rom_sz >= MiB ? 0 : c->ram_bk;
		return c->ram[((addr - 0xA000) + (bank * KiB8)) % c->ram_sz];
	}

	bank = c->rom_bk & 0x1F;

	if (!bank)
		bank = 1;

	if (c->rom_sz <= KiB256)
		bank &= (c->n_rom_bk - 1);
	else if (c->rom_sz >= MiB)
		bank |= (c->ram_bk << 5);

	return c->rom[bank * 0x4000 + (addr - 0x4000)];
}

void mbc1_wr(struct Cartridge *c, uint16_t addr, uint8_t data)
{
	if (addr <= 0x1FFF) {
		c->ram_prms = ((data & 0x0A) == 0x0A);
	} else if (addr <= 0x3FFF) {
		c->rom_bk = data;
	} else if (addr <= 0x5FFF) {
		c->ram_bk = data & 0x03;
	} else if (addr <= 0x7FFF) {
		c->bk_md = data;
	} else if (c->ram_prms && 0xA000 <= addr && addr <= 0xBFFF) {
		uint8_t bank = c->rom_sz >= MiB ? 0 : c->ram_bk;
		c->ram[((addr - 0xA000) + (bank * KiB8)) % c->ram_sz] = data;
		c->ram_dirty = true;
	}
}

uint8_t mbc3_rd(struct Cartridge *c, uint16_t addr)
{
	if (addr < 0x4000)
		return c->rom[addr];

	uint8_t bank;
	if (c->ram_prms && 0xA000 <= addr && addr <= 0xBFFF) {
		bank = c->ram_bk & 0x03;
		return c->ram[((addr - 0xA000) + (bank * KiB8)) % c->ram_sz];
	}

	bank = c->rom_bk & 0x7F;

	if (!bank)
		bank = 1;

	return c->rom[bank * 0x4000 + (addr - 0x4000)];
}

void mbc3_wr(struct Cartridge *c, uint16_t addr, uint8_t data)
{
	if (addr <= 0x1FFF) {
		c->ram_prms = ((data & 0x0A) == 0x0A);
	} else if (addr <= 0x3FFF) {
		c->rom_bk = data;
	} else if (addr <= 0x5FFF) {
		if (data <= 0x07)
			c->ram_bk = data;
		else
			c->rtc = data;
	} else if (addr <= 0x7FFF) {
	} else if (c->ram_prms && 0xA000 <= addr && addr <= 0xBFFF) {
		addr = ((addr - 0xA000) + (c->ram_bk * KiB8)) % c->ram_sz;
		c->ram[addr] = data;
		c->ram_dirty = true;
	}
}

cart_rd mbc_rd[0x1C] = { [0x00] = mbc0_rd, [0x01] = mbc1_rd, [0x02] = mbc1_rd,
			 [0x03] = mbc1_rd, [0x0F] = mbc3_rd, [0x10] = mbc3_rd,
			 [0x11] = mbc3_rd, [0x12] = mbc3_rd, [0x13] = mbc3_rd };

cart_wr mbc_wr[0x1C] = { [0x00] = mbc0_wr, [0x01] = mbc1_wr, [0x02] = mbc1_wr,
			 [0x03] = mbc1_wr, [0x0F] = mbc3_wr, [0x10] = mbc3_wr,
			 [0x11] = mbc3_wr, [0x12] = mbc3_wr, [0x13] = mbc3_wr };

void crt_eject(struct Cartridge *c)
{
	free(c->rom);
	if (crt_mem) {
		crt_sv_ram(c);
		fclose(crt_mem);
		crt_mem = NULL;
	}
	if (c->ram)
		free(c->ram);
}

void crt_load(struct Cartridge *crt, const char *path)
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
	memcpy(crt->titl, &header[TITL], 16 * sizeof(char));
	crt->titl[16] = '\0';
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
		load_mem(crt);
		break;
	default:
		crt->ram_sz = 0;
		crt->ram = NULL;
		break;
	}
	fprintf(stderr, "Opened %s: rom %lu, ram %lu, type 0x%02x\n", crt->titl,
		crt->rom_sz / 1000, crt->ram_sz, crt->type);
}
