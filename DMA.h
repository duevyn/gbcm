#ifndef DMA_H
#define DMA_H

#include <stdint.h>
#include <stdbool.h>

struct GameBoy;

struct DMA {
	bool active;
	uint8_t cnt;
	uint8_t delay;
	uint8_t value;
};

void dma_start(struct GameBoy *gb, uint8_t data);
uint8_t dma_step(struct GameBoy *gb);

#endif
