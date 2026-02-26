#include "DMA.h"
#include "GameBoy.h"
#include "bus.h"
#include "logger.h"
#include <stdio.h>

//#define fprintf(stderr, ...) ((void)0)
void dma_start(struct GameBoy *gb, uint8_t data)
{
	gb->dma.active = true;
	gb->dma.cnt = 0;
	gb->dma.delay = 2; // Wait ~2 machine cycles before starting
	gb->dma.value = data;
}

uint8_t dma_step(GameBoy *gb)
{
	if (gb->dma.delay > 0) {
		gb->dma.delay--;
		return 4;
	}

	uint16_t src = (gb->dma.value << 8) + gb->dma.cnt;
	gb->ppu.oam[gb->dma.cnt] = bus_read(gb, src);

	if (gb->dma.cnt++ >= 160)
		gb->dma.active = false;

	return 4;
}
