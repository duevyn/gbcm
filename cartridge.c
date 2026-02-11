#include "cartridge.h"
#include <stdio.h>
#include <stdint.h>
#include <unistd.h>
#include <fcntl.h>

#define KB32 32768

uint8_t cart_read(struct Cartridge *crt, uint16_t addr)
{
	//TODO: only works for tetris (no banking)
	return crt->rom_data[addr];
}

void cart_write(struct Cartridge *crt, uint16_t addr, uint8_t data)
{
}

void cart_load(struct Cartridge *crt, const char *path)
{
	//TODO: only works for tetris
	size_t sz = KB32;
	int fd = open(path, O_RDONLY);
	if (fd < 0) {
		perror("Error opening file\n");
		exit(EXIT_FAILURE);
	}
	crt->rom_data = malloc(sz);
	ssize_t n = read(fd, crt->rom_data, sz);
	if (n > sz) {
		perror("Did not read expected bytes\n");
		exit(EXIT_FAILURE);
	}
}
