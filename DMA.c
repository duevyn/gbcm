#include "DMA.h"
#include "GameBoy.h"
#include "bus.h"
#include <stdio.h>

void dma_start(struct GameBoy *gb, uint8_t data)
{
	gb->dma.active = true;
	gb->dma.cnt = 0;
	gb->dma.delay = 2; // Wait ~2 machine cycles before starting
	gb->dma.value = data;
	/*
	fprintf(stderr, "\n\n DMA: ");
	for (int i = 0; i < 160; i++) {
		fprintf(stderr, "%02x ", gb->ppu.oam[i]);
	}
	fprintf(stderr, "\n\n");
        */
}

uint8_t dma_step(GameBoy *gb)
{
	if (gb->dma.delay > 0) {
		fprintf(stderr, "ALERT: DMA DELAY ");
		gb->dma.delay--;
		return 4;
	}

	uint16_t src = (gb->dma.value << 8) + gb->dma.cnt;
	gb->ppu.oam[gb->dma.cnt] = bus_read(gb, src);

	fprintf(stderr, "DMA %d: 0x%04x 0x%02x  ", gb->dma.cnt, src,
		gb->ppu.oam[gb->dma.cnt]);

	if (++gb->dma.cnt > 159) {
		gb->dma.active = false;
		fprintf(stderr, "\n");
		for (int i = 0; i < 160; i++) {
			fprintf(stderr, "0x%02x ", gb->ppu.oam[i]);
		}
		fprintf(stderr, "\n");
	}

	return 4;
}
