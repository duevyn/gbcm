#include "DMA.h"
#include "GameBoy.h"
#include "bus.h"
#include <stdio.h>

static uint16_t dmadots;

void dma_start(struct GameBoy *gb, uint8_t data)
{
	gb->dma.active = true;
	gb->dma.cnt = 0;
	gb->dma.delay = 2; // Wait ~2 machine cycles before starting
	gb->dma.value = data;
	dmadots = 0;
}

uint8_t dma_step(GameBoy *gb)
{
	dmadots += 4;
	if (gb->dma.delay > 0) {
		fprintf(stderr, "ALERT: DMA DELAY ");
		gb->dma.delay--;
		return 4;
	}

	uint16_t src = (gb->dma.value << 8) + gb->dma.cnt;

	uint8_t val = bus_read(gb, src);
	fprintf(stderr, "DMA %d: 0x%04x 0x%02x dots %d ", gb->dma.cnt, src, val,
		dmadots);

	bus_write(gb, 0xFE00 + gb->dma.cnt, val);

	if (++gb->dma.cnt > 159) {
		gb->dma.active = false;
		fprintf(stderr, "\n");
		for (int i = 0; i < 160; i++) {
			fprintf(stderr, "0x%02x ", gb->ppu.oam[i]);
		}
		fprintf(stderr, "\n");
		//exit(1);
	}

	return 4;
}
