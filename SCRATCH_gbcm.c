/*
Scratch work: playing around with SDL rendering and mapping/decoding tiles.
Sample tiles from: https://www.huderlem.com/demos/gameboy2bpp.html

Committing to show how I fill in knowledge gaps before actual implementation.
(aka no ai/vibe coding)
*/
#include <stdlib.h>
#include <stdio.h>
#define SDL_MAIN_USE_CALLBACKS 1 /* use the callbacks instead of main() */
#include <SDL3/SDL.h>
#include <SDL3/SDL_main.h>

#include "sm83.h"

static SDL_Window *window = NULL;
static SDL_Renderer *renderer = NULL;
static struct sm83 *cpu = NULL;
static int COLORSs[] = { 0x0f380f, 0x306230, 0x8bac0f, 0x9bbc0f };
static int CCOLORS[4][3] = { { 0x0f, 0x38, 0x0f },
			     { 0x30, 0x62, 0x30 },
			     { 0x8b, 0xac, 0x0f },
			     { 0x9b, 0xbc, 0x0f } };

static int COLORS[4][3] = { { 0xff, 0xff, 0xff },
			    { 0xd3, 0xd3, 0xd3 },
			    { 0x33, 0x33, 0x33 },
			    { 0x00, 0x00, 0x00 } };
static uint8_t sctiles[32 * 32][16];
typedef enum COLOR {
	BLACK = 0,
	GRAY = 0x808080,
	LGRAY = 0xc0c0c0,
	WHITE = 0xffffff,
} COLOR;

// Copy the 96 bytes to VRAM starting at address 0x8000
uint8_t nintendo_logo_tiles[96] = {
	// Tile 0
	0xC8, 0xC8, 0xFC, 0xFC, 0xFC, 0xFC, 0xFC, 0xFC, 0xFC, 0xFC, 0xFC, 0xFC,
	0xC8, 0xC8, 0x00, 0x00,
	// Tile 1
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x18, 0x18, 0xF0, 0xF0,
	0x00, 0x00, 0x00, 0x00,
	// Tile 2
	0x00, 0x00, 0x18, 0x18, 0x18, 0x18, 0x1C, 0x1C, 0x38, 0x38, 0x38, 0x38,
	0x38, 0x38, 0x00, 0x00,
	// Tile 3
	0xFC, 0xFC, 0xFC, 0xFC, 0xFC, 0xFC, 0xFC, 0xFC, 0xF8, 0xF8, 0xF8, 0xF8,
	0xF0, 0xF0, 0x00, 0x00,
	// Tile 4
	0xFC, 0xFC, 0xFC, 0xFC, 0xFC, 0xFC, 0xFC, 0xFC, 0xFC, 0xFC, 0xF8, 0xF8,
	0x00, 0x00, 0x00, 0x00,
	// Tile 5
	0xFC, 0xFC, 0xFC, 0xFC, 0xFC, 0xFC, 0xFC, 0xFC, 0xF8, 0xF8, 0x18, 0x18,
	0x00, 0x00, 0x00, 0x00
};

struct ctdg *ctdg_load()
{
	struct ctdg *ct = malloc(sizeof(*ct));
	size_t sz = 2097152;
	int fd = open("metalgearsolid.gbc", O_RDONLY);
	if (fd < 0) {
		perror("Could not open metal gear solid\n");
		exit(EXIT_FAILURE);
	}
	ct->slb = malloc(sz);
	ssize_t n = read(fd, ct->slb, sz);
	if (n < sz) {
		perror("Did not get 2097152 bytes\n");
		exit(EXIT_FAILURE);
	}
	int c = 0;

	uint8_t buf[48];
	/*
	buf[47] = '\0';
	memcpy(buf, ct->slb + 0x104, 47);
	for (int i = 0x104; i < 0x133; i++) {
		fprintf(stderr, "%02x ", ct->slb[i]);
		if (++c % 16 == 0)
			fprintf(stderr, "\n");
	}
	fprintf(stderr, "\n\n%s\n\n", buf);
	c = 0;
	for (int i = 0; i < 48; i++) {
		fprintf(stderr, "%02x ", buf[i]);
		if (++c % 16 == 0)
			fprintf(stderr, "\n");
	}
	fprintf(stderr, "\n\n");
        */

	*(ct->slb + 0xff05) = 0; //TIMA
	*(ct->slb + 0xff06) = 0; //TMA
	*(ct->slb + 0xff07) = 0; //TAC

	*(ct->slb + 0xff10) = 0x80; //NR10
	*(ct->slb + 0xff11) = 0xbf; //NR11
	*(ct->slb + 0xff12) = 0xf3; //NR12
	*(ct->slb + 0xff14) = 0xbf; //NR14
	*(ct->slb + 0xff16) = 0x3f; //NR21
	*(ct->slb + 0xff17) = 0x00; //NR22
	*(ct->slb + 0xff19) = 0xbf; //NR24

	*(ct->slb + 0xff1a) = 0x7f; //NR30
	*(ct->slb + 0xff1b) = 0xff; //NR31
	*(ct->slb + 0xff1c) = 0x9f; //NR32
	*(ct->slb + 0xff1e) = 0xbf; //NR33
	*(ct->slb + 0xff20) = 0xff; //NR41
	*(ct->slb + 0xff21) = 0x00; //NR42
	*(ct->slb + 0xff22) = 0x00; //NR43
	*(ct->slb + 0xff23) = 0xbf; //NR30
	*(ct->slb + 0xff24) = 0x77; //NR50
	*(ct->slb + 0xff25) = 0xf3; //NR51

	*(ct->slb + 0xff26) = 0xf1; //NR52

	*(ct->slb + 0xff40) = 0x91; //LCDC
	*(ct->slb + 0xff42) = 0x00; //SCY
	*(ct->slb + 0xff43) = 0x00; //SCX
	*(ct->slb + 0xff45) = 0x00; //LYC
	*(ct->slb + 0xff47) = 0xfc; //BGP
	*(ct->slb + 0xff48) = 0xff; //OBP0
	*(ct->slb + 0xff49) = 0xff; //OBP1
	*(ct->slb + 0xff4a) = 0x00; //WY
	*(ct->slb + 0xff4b) = 0x00; //WX
	*(ct->slb + 0xffff) = 0x00; //IE

	//memcpy(&ct->slb[0x8000], nintendo_logo_tiles, 96);
	fprintf(stderr, "\n");
	return ct;
}

struct sm83 *sm_init(struct sm83 *s)
{
	//s->ra = 0x11;
	//s->rf = 0xb0;
	//s->rb = 0;
	//s->rc = 0x13;
	//s->rd = 0;
	//s->re = 0xd8;
	//s->rh = 0x01;
	//s->rl = 0x4d;

	s->af = 0x11b0;
	s->bc = 0x0013;
	s->de = 0x00d8;
	s->hl = 0x014d;
	//s->rsp = 0xfffe;
	s->sp = 0xfffe;

	s->ime = s->dblspd = false;
	fprintf(stderr,
		"af 0x%04x (addr 0x%p), a 0x%02x (addr 0x%p), f 0x%02x (addr 0x%p)\n",
		s->af, &s->af, s->a, &s->a, s->f, &s->f);
	fprintf(stderr,
		"hl 0x%04x (addr 0x%p), h 0x%02x (addr 0x%p), l 0x%02x (addr 0x%p)\n",
		s->hl, &s->hl, s->h, &s->h, s->l, &s->l);

	return s;
}

char *prefix(struct sm83 *cpu, uint8_t *bs)
{
	char *r, *rr;
	r = rr = "\0";

	uint8_t op = bs[cpu->pc++];
	uint16_t off;

	fprintf(stderr, "0x%04x: op %02x", cpu->pc - 1, op);
	switch (op) {
	case 0xbe:
		r = " -- RES 7,[HL] 2,16\n";
		rr = " -- RES 7,[HL] 2,16";
		fprintf(stderr, "%s :: hl 0x%04x prev 0x%02x -> ", rr, cpu->hl,
			cpu->pgm[cpu->hl]);
		cpu->pgm[cpu->hl] &= 0x7f;
		fprintf(stderr, "post 0x%02x", cpu->pgm[cpu->hl]);
		cpu->pc++;
		break;
	case 0x7e:
		r = " -- BIT 7,(HL) 2,16 z01-\n";
		rr = " -- BIT 7,(HL) 2,16 z01-";
		cpu->fN = 0;
		cpu->fH = 1;
		cpu->pc += 1;

		uint8_t val = cpu->pgm[cpu->hl];
		fprintf(stderr, "%s == hl 0x%04x [hl] 0x%02x f 0b%08b", rr,
			cpu->hl, val, cpu->f);
		cpu->fZ = ((1 << 7) & val) == 0 ? 1 : 0;
		fprintf(stderr, " --> 0b%08b", cpu->f);
		break;
	case 0xc6:
		off = cpu->pgm[cpu->hl];
		r = " -- CAUTION SET 0,(HL) 2,16 ";
		fprintf(stderr,
			" -- CAUTION SET 0,(HL) 2,16 $$ hl 0x%04x [hl] 0x%02x off 0x%04x",
			cpu->hl, cpu->pgm[off], off);
		cpu->pgm[off] |= 1;
		off = cpu->pgm[cpu->hl];
		fprintf(stderr, " new 0x%04x", off);
		cpu->pc += 1;
		break;
	case 0x02:
		r = " -- RLC D 2,8 z00c\n";
		//(*pc) += 2;
		cpu->pc += 2;
		fprintf(stderr, "%sprefixed op %02x not implemented\n", r, op);
		exit(1);
		break;
	default:
		if (r)
			fprintf(stderr, "%s", r);
		if (rr)
			fprintf(stderr, "%s", rr);
		fprintf(stderr, " -- prefixed op %02x not implemented\n", op);
		exit(1);
		break;
	}

	return r;
}

//static struct sm83 cpu;
struct sm83 *startGame(void)
{
	struct ctdg *cg = ctdg_load();
	//struct sm83 cpu = { .pc = 0x150, .ime = false, .cg = cg };
	struct sm83 *cpu = calloc(sizeof(*cpu), 1);
	cpu->pc = 0x150;
	cpu->ime = false;
	cpu->cg = cg;
	cpu->pgm = cg->slb;

	sm_init(cpu);

	uint8_t op;
	char *nm = "\0";
	bool pre = false;
	bool impl;
	uint16_t pctm;
	int loopcount = 0;
	uint16_t testsp;
	while (cpu->pc < 0xffff) {
		//while (cpu->pc < 0x7fff) {
		// fetch
		impl = false;
		pctm = cpu->pc;
		op = *(cg->slb + cpu->pc++);
		uint16_t sr;
		if (op != 0xcb)
			fprintf(stderr, "0x%04x: op %02x", cpu->pc - 1, op);

		if (op >= 0xf0) {
			switch (op) {
			case 0xf0:
				nm = " -- LDH A,[a8] 2,12\n";
				uint8_t a8 = cg->slb[cpu->pc++];
				fprintf(stderr,
					" -- LDH A,[a8] 2,12 *** a8 0x%02x + 0xff00 :: 0x%02x, cpu->a 0x%02x",
					a8, cg->slb[0xFF00 + a8], cpu->a);
				cpu->a = cg->slb[0xFF00 + a8];
				fprintf(stderr, " -> a 0x%02x, f 0b%08b",
					cpu->a, cpu->f);
				impl = true;
				break;
			case 0xf1:
				nm = " -- POP AF 1,12 znhc\n";
				cpu->pc += 1;
				break;
			case 0xf2:
				nm = " -- LDH [A],C 1,8\n";
				cpu->pc += 1;
				break;
			case 0xf3:
				nm = " -- DI --  1, 4\n";
				cpu->ime = false;
				//cpu->pc++;
				impl = true;
				break;
			case 0xf5:
				nm = " -- PUSH AF 1,16\n";
				cpu->sp -= 2;
				*(uint16_t *)(&cg->slb[cpu->sp]) = cpu->af;
				fprintf(stderr,
					"-- PUSH AF 1,16 -- af 0x%04x sp 0x%04x",
					cpu->af, cpu->sp);
				impl = true;
				break;
			case 0xf6:
				nm = " -- OR A,n8 2,8 z000\n";
				cpu->pc += 2;
				break;
			case 0xf7:
				nm = " -- RST $30 1,16\n";

				uint16_t fnoffs = cg->slb[cpu->pc] |
						  (cg->slb[cpu->pc + 1] << 8);
				fprintf(stderr,
					" -- RST $30 1,16 ** sp 0x%04x [sp] 0x%04x !! fnoffs 0x%04x",
					cpu->sp,
					*(uint16_t *)(&cg->slb[cpu->sp]),
					fnoffs);

				cpu->sp -= 2;
				*(uint16_t *)(&cg->slb[cpu->sp]) = cpu->pc;
				READ_UINT16(&testsp, (&cpu->pc));
				fprintf(stderr,
					" $$ sp 0x%04x = 0x%04x, pc 0x%04x -- TEST SP 0x%04x",
					cpu->sp,
					*(uint16_t *)(&cg->slb[cpu->sp]),
					cpu->pc, testsp);
				cpu->pc = 0x30;
				impl = true;
				break;
			case 0xf8:
				nm = " -- LD HL,SP+e8 2,12 00hc\n";
				cpu->pc += 2;
				break;
			case 0xf9:
				nm = " -- LD SP,HL 1,8\n";
				cpu->pc += 1;
				break;
			case 0xfa:
				nm = " -- LD A,[a16] 3,16\n";
				sr = *(uint16_t *)(&cg->slb[cpu->pc]);
				fprintf(stderr,
					" -- LD A,[a16] 3,16 sr 0x%04x, cpu->a 0x%02x",
					sr, cpu->a);
				cpu->a = cg->slb[sr];
				fprintf(stderr, " ## cpu->a 0x%02x", cpu->a);
				fprintf(stderr,
					" ===== TEST ==== 0x%02x , 0xcf07 -> 0x%02x",
					cg->slb[sr], cg->slb[0xcf07]);
				cpu->pc += 2;
				impl = true;
				break;
			case 0xfb:
				nm = " -- EI --  1, 4\n";
				cpu->pc++;
				break;
			case 0xfd:
				fprintf(stderr,
					"\nERROR: 0xfd invalid opcode\n");
				exit(1);
				break;
			case 0xfe:
				nm = " -- CP A,n8 2,8 z1hc\n";
				cpu->f = N;
				//uint8_t flg = N;
				//sm_setfl(&cpu-> N);
				uint8_t sr = *(cg->slb + cpu->pc);
				fprintf(stderr,
					" -- CP A,n8 2,8 z1hc -- a 0x%02x, a8 0x%02x, f 0b%08b",
					cpu->a, sr, cpu->f);
				cpu->fZ = cpu->a == sr;
				cpu->fC = cpu->a < sr;
				cpu->fH = (cpu->a & 0x0f) < (sr & 0x0f);
				fprintf(stderr, " -> 0b%08b", cpu->f);
				cpu->pc += 1;
				impl = true;
				break;
			case 0xff:
				nm = " -- RST $38 1,16\n";

				fnoffs = cg->slb[cpu->pc] |
					 (cg->slb[cpu->pc + 1] << 8);
				fprintf(stderr,
					" -- RST $38 1,16 ** sp 0x%04x = 0x%04x !! fnoffs 0x%04x",
					cpu->sp,
					*(uint16_t *)(&cg->slb[cpu->sp]),
					fnoffs);

				cpu->sp -= 2;
				*(uint16_t *)(&cg->slb[cpu->sp]) = cpu->pc;
				cpu->pc = 0x38;
				fprintf(stderr,
					" $$ sp 0x%04x = 0x%04x, pc 0x%04x",
					cpu->sp,
					*(uint16_t *)(&cg->slb[cpu->sp]),
					cpu->pc);
				impl = true;
				break;
			default:
				break;
			}
		} else if (op >= 0xe0) {
			switch (op) {
			case 0xe0:
				nm = " -- LDH [a8],A 2,12 ";
				uint8_t a8 = cg->slb[cpu->pc++];
				fprintf(stderr,
					" -- LDH [a8],A 2,12 *** a8 0x%02x, cpu->a 0x%02x",
					a8, cpu->a);
				cg->slb[0xFF00 + a8] = cpu->a;
				impl = true;
				break;
			case 0xe1:
				nm = " -- POP HL 1,12\n";
				cpu->pc += 1;
				break;
			case 0xe2:
				nm = " -- LDH [C],A 1,8\n";
				cpu->pc += 1;
				break;
			case 0xe5:
				nm = " -- PUSH HL 1,16\n";
				cpu->pc += 1;
				break;
			case 0xe6:
				nm = " -- AND A,n8 2,8 z010\n";
				fprintf(stderr, " -- cpu->a %02x", cpu->a);
				cpu->a &= cg->slb[cpu->pc++];
				fprintf(stderr, " :: cpu->a %02x cpu->f 0b%08b",
					cpu->a, cpu->f);
				impl = true;
				break;
			case 0xe7:
				nm = " -- RST $20 1,16\n";
				cpu->pc += 1;
				break;
			case 0xe8:
				nm = " -- ADD SP,e8 2,16 00hc\n";
				cpu->pc += 2;
				break;
			case 0xe9:
				nm = " -- JP HL 1,4\n";
				cpu->pc = cpu->hl;
				impl = true;
				break;
			case 0xea:
				nm = " -- LD [a16],A 3,16\n";
				uint16_t dst = *(uint16_t *)(&cg->slb[cpu->pc]);
				fprintf(stderr,
					" -- LD [a16],A 3,16 -- dst 0x%04x, a 0x%02x, pre 0x%02x",
					dst, cpu->a, cg->slb[dst]);
				cg->slb[dst] = cpu->a;
				fprintf(stderr, " $$ post 0x%02x ",
					cg->slb[dst]);
				cpu->pc += 2;
				impl = true;
				break;
			case 0xed:
				fprintf(stderr,
					"\nERROR: 0xed invalid opcode\n");
				exit(1);
				break;
			case 0xee:
				nm = " -- XOR A,n8 2,8 z000\n";
				cpu->pc += 2;
				break;
			case 0xef:
				nm = " -- RST $28 1,16\n";
				cpu->pc += 1;
				break;
			default:
				break;
			}
		} else if (op >= 0xd0) {
			switch (op) {
			case 0xd0:
				nm = " -- RET NC 1,20/8\n";
				cpu->pc += 1;
				break;
			case 0xd1:
				nm = " -- POP DE 1,12\n";
				cpu->pc += 1;
				break;
			case 0xd2:
				nm = " -- JP NC, a16 3,16/12\n";
				cpu->pc += 3;
				break;
			case 0xd4:
				nm = " -- CALL NC, a16 3,24/12\n";
				cpu->pc += 3;
				break;
			case 0xd5:
				nm = " -- PUSH DE 1,16\n";
				cpu->pc += 1;
				break;
			case 0xd6:
				nm = " -- SUB A,n8 2,8 z1hc\n";
				cpu->pc += 2;
				break;
			case 0xd7:
				nm = " -- RST $10 1,16\n";

				uint16_t fnoffs = cg->slb[cpu->pc] |
						  (cg->slb[cpu->pc + 1] << 8);
				fprintf(stderr,
					" -- RST $10 1,16 ** sp 0x%04x [sp] 0x%04x !! fnoffs 0x%04x",
					cpu->sp,
					*(uint16_t *)(&cg->slb[cpu->sp]),
					fnoffs);
				cpu->sp -= 2;
				*(uint16_t *)(&cg->slb[cpu->sp]) = cpu->pc;
				cpu->pc = 0x10;
				fprintf(stderr,
					" $$ sp 0x%04x = 0x%04x, pc 0x%04x",
					cpu->sp,
					*(uint16_t *)(&cg->slb[cpu->sp]),
					cpu->pc);
				impl = true;
				break;
			case 0xd8:
				nm = " -- RET C 1,20/8\n";
				cpu->pc += 1;
				break;
			case 0xd9:
				nm = " -- RET I?,16\n";
				cpu->pc += 1;
				break;
			case 0xda:
				nm = " -- JP C, a16 3,16/12\n";
				cpu->pc += 3;
				break;
			case 0xdc:
				nm = " -- CALL C, a16 3,24/12\n";
				cpu->pc += 3;
				break;
			case 0xdd:
				fprintf(stderr,
					"\n\n=========================\n ERROR 0xdd invalid op code\n=========================\n\n");
				exit(1);
				break;
			case 0xde:
				nm = " -- SBC A,n8 2,8 z1hc\n";
				cpu->pc += 2;
				break;
			case 0xdf:
				nm = " -- RST $18 1,16\n";
				cpu->pc += 1;
				break;
			default:
				fprintf(stderr,
					"\nERROR: 0xdd invalid opcode\n");
				exit(1);
				break;
			}
		} else if (op >= 0xc0) {
			switch (op) {
			case 0xc0:
				nm = " -- RET NZ 1,20/8\n";
				cpu->pc += 1;
				break;
			case 0xc1:
				nm = " -- POP BC 1,12\n";
				cpu->pc += 1;
				break;
			case 0xc2:
				nm = " -- JP NZ, a16 3,16/12\n";
				cpu->pc += 3;
				break;
			case 0xc3:
				nm = " -- JP a16 3,16\n";

				READ_UINT16(&testsp, (cg->slb + cpu->pc));
				cpu->pc = cg->slb[cpu->pc] |
					  (cg->slb[cpu->pc + 1] << 8);
				fprintf(stderr,
					" -- JP a16, 3,16 -- a16 0x%04x -- TEST SP 0x%04x",
					cpu->pc, testsp);
				impl = true;
				break;
			case 0xc4:
				nm = " -- CALL NZ, a16 3,24/12\n";
				cpu->pc += 3;
				break;
			case 0xc5:
				nm = " -- PUSH BC 1,16\n";
				cpu->pc += 1;
				break;
			case 0xc6:
				nm = " -- ADD A,n8 2,8 z0hc\n";
				cpu->pc += 2;
				break;
			case 0xc7:
				nm = " -- RST $00 1,16\n";
				cpu->pc += 1;
				break;
			case 0xc8:
				nm = " -- RET Z 1,20/8\n";
				fprintf(stderr,
					" -- RET Z 1,20/8 --  sp 0x%04x @@ fZ %b sp 0x%04x",
					cpu->sp, cpu->fZ,
					*(uint16_t *)(&cg->slb[cpu->sp]));
				if (cpu->fZ) {
					uint16_t rtaddr = *(
						uint16_t *)(&cg->slb[cpu->sp]);
					cpu->sp += 2;
					cpu->pc = rtaddr;

				} else {
					fprintf(stderr, "no jump :: z = 0");
				}
				impl = true;
				break;
			case 0xc9:
				nm = " -- RET 1,16\n";
				fprintf(stderr, " -- RET 1,16 -- sp 0x%04x",
					cpu->sp);
				cpu->pc = *(uint16_t *)(&cg->slb[cpu->sp]);
				cpu->sp += 2;
				fprintf(stderr, " :: 0x%04x", cpu->sp);
				impl = true;
				break;
			case 0xca:
				nm = " -- JP Z,a16 3,16/12\n";
				cpu->pc += 2;
				if (cpu->fZ)
					cpu->pc = cg->slb[cpu->pc - 2] |
						  (cg->slb[cpu->pc - 1] << 8);
				impl = true;
				break;
			case 0xcb:
				fprintf(stderr,
					"\n=========================\n0x%04x PREFIX 0xcb 1,4\n=========================\n",
					pctm);
				nm = prefix(cpu, cg->slb);

				pre = true;
				impl = true;
				break;
			case 0xcc:
				nm = " -- CALL Z, a16 3,24/12\n";
				cpu->pc += 3;
				break;
			case 0xcd:
				nm = " -- CALL a16 -- 3, 24\n";
				uint16_t fnoff =
					cg->slb[cpu->pc] |
					(cg->slb[cpu->pc + 1]
					 << 8); // read 2 bytes for uint16_t
				cpu->pc += 2;
				cpu->sp -= 2;
				fprintf(stderr,
					" -- CALL a16 -- 3, 24 ** sp 0x%04x pc 0x%04x !! fnoff 0x%04x",
					cpu->sp, cpu->pc, fnoff);
				*(uint16_t *)(&cg->slb[cpu->sp]) = cpu->pc;
				fprintf(stderr, " $$ sp 0x%04x [sp] 0x%04x",
					cpu->sp,
					*(uint16_t *)(&cg->slb[cpu->sp]));
				cpu->pc = fnoff;
				impl = true;
				break;
			case 0xce:
				nm = " -- ADC A,n8 2,8 z0hc\n";
				cpu->pc += 2;
				break;
			case 0xcf:
				nm = " -- RST $08 1,16\n";
				cpu->pc += 1;
				break;
			default:
				break;
			}
		} else if (op >= 0xb0) {
			switch (op) {
			case 0xb0:
				nm = " -- OR A,B 1,4 z000\n";
				cpu->pc += 1;
				break;
			case 0xb1:
				nm = " -- OR A,C 1,4 z000\n";
				cpu->pc += 1;
				break;
			case 0xb2:
				nm = " -- OR A,D 1,4 z000\n";
				cpu->pc += 1;
				break;
			case 0xb3:
				nm = " -- OR A,E 1,4 z000\n";
				cpu->pc += 1;
				break;
			case 0xb4:
				nm = " -- OR A,H 1,4 z000\n";
				cpu->pc += 1;
				break;
			case 0xb5:
				nm = " -- OR A,L 1,4 z000\n";
				cpu->pc += 1;
				break;
			case 0xb6:
				nm = " -- OR A,[HL] 1,8 z000\n";
				cpu->pc += 1;
				break;
			case 0xb7:
				nm = " -- OR A,A 1,4 z000\n";
				cpu->a |= cpu->a;
				cpu->f = 0;
				cpu->fZ = cpu->a == 0 ? 1 : 0;
				fprintf(stderr,
					" -- OR A,A 1,4 z000 -- cpu->a 0x%02x f 0b%08b ",
					cpu->a, cpu->f);
				impl = true;

				break;
			case 0xb8:
				nm = " -- CP A,B 1,4 z1hc\n";
				cpu->pc += 1;
				break;
			case 0xb9:
				nm = " -- CP A,C 1,4 z1hc\n";
				cpu->pc += 1;
				break;
			case 0xba:
				nm = " -- CP A,D 1,4 z1hc\n";
				cpu->pc += 1;
				break;
			case 0xbb:
				nm = " -- CP A,E 1,4 z1hc\n";
				cpu->pc += 1;
				break;
			case 0xbc:
				nm = " -- CP A,H 1,4 z1hc\n";
				cpu->pc += 1;
				break;
			case 0xbd:
				nm = " -- CP A,L 1,4 z1hc\n";
				cpu->pc += 1;
				break;
			case 0xbe:
				nm = " -- CP A,[HL] 1,8 z1hc\n";
				cpu->pc += 1;
				break;
			case 0xbf:
				nm = " -- CP A,A 1,4 z1hc\n";
				cpu->pc += 1;
				break;
			default:
				break;
			}
		} else if (op >= 0xa0) {
			switch (op) {
			case 0xa0:
				nm = " -- AND A,B 1,4 z010\n";
				cpu->pc += 1;
				break;
			case 0xa1:
				nm = " -- AND A,C 1,4 z010\n";
				cpu->pc += 1;
				break;
			case 0xa2:
				nm = " -- AND A,D 1,4 z010\n";
				cpu->pc += 1;
				break;
			case 0xa3:
				nm = " -- AND A,E 1,4 z010\n";
				cpu->pc += 1;
				break;
			case 0xa4:
				nm = " -- AND A,H 1,4 z010\n";
				cpu->pc += 1;
				break;
			case 0xa5:
				nm = " -- AND A,L 1,4 z010\n";
				cpu->pc += 1;
				break;
			case 0xa6:
				nm = " -- AND A,[HL] 1,8 z010\n";
				cpu->pc += 1;
				break;
			case 0xa7:
				nm = " -- AND A,A 1,4 z010\n";
				cpu->pc += 1;
				break;
			case 0xa8:
				nm = " -- XOR A,B 1,4 z000\n";
				cpu->pc += 1;
				break;
			case 0xa9:
				nm = " -- XOR A,C 1,4 z000\n";
				cpu->pc += 1;
				break;
			case 0xaa:
				nm = " -- XOR A,D 1,4 z000\n";
				cpu->pc += 1;
				break;
			case 0xab:
				nm = " -- XOR A,E 1,4 z000\n";
				cpu->pc += 1;
				break;
			case 0xac:
				nm = " -- XOR A,H 1,4 z000\n";
				cpu->pc += 1;
				break;
			case 0xad:
				nm = " -- XOR A,L 1,4 z000\n";
				cpu->pc += 1;
				break;
			case 0xae:
				nm = " -- XOR A,[HL] 1,8 z000\n";
				cpu->pc += 1;
				break;
			case 0xaf:
				nm = " -- XOR A, A -- 1, 4\n";
				fprintf(stderr,
					"  -- XOR A, A -- 1, 4 -- a 0x%02x f 0b%08b",
					cpu->a, cpu->f);
				cpu->f = 0;
				cpu->a ^= cpu->a;
				cpu->fZ = cpu->a == 0 ? 1 : 0;
				fprintf(stderr, " -- a 0x%02x f 0b%08b", cpu->a,
					cpu->f);
				impl = true;
				break;
			default:
				break;
			}
		} else if (op >= 0x90) {
			switch (op) {
			case 0x90:
				nm = " -- SUB A,B 1,4 z1hc\n";
				cpu->pc += 1;
				break;
			case 0x91:
				nm = " -- SUB A,C 1,4 z1hc\n";
				cpu->pc += 1;
				break;
			case 0x92:
				nm = " -- SUB A,D 1,4 z1hc\n";
				cpu->pc += 1;
				break;
			case 0x93:
				nm = " -- SUB A,E 1,4 z1hc\n";
				cpu->pc += 1;
				break;
			case 0x94:
				nm = " -- SUB A,H 1,4 z1hc\n";
				cpu->pc += 1;
				break;
			case 0x95:
				nm = " -- SUB A,L 1,4 z1hc\n";
				cpu->pc += 1;
				break;
			case 0x96:
				nm = " -- SUB A,[HL] 1,8 z1hc\n";
				cpu->pc += 1;
				break;
			case 0x97:
				nm = " -- SUB A,A 1,4 z1hc\n";
				cpu->pc += 1;
				break;
			case 0x98:
				nm = " -- ADC A,B 1,4 z1hc\n";
				cpu->pc += 1;
				break;
			case 0x99:
				nm = " -- ADC A,C 1,4 z1hc\n";
				cpu->pc += 1;
				break;
			case 0x9a:
				nm = " -- ADC A,D 1,4 z1hc\n";
				cpu->pc += 1;
				break;
			case 0x9b:
				nm = " -- ADC A,E 1,4 z1hc\n";
				cpu->pc += 1;
				break;
			case 0x9c:
				nm = " -- ADC A,H 1,4 z1hc\n";
				cpu->pc += 1;
				break;
			case 0x9d:
				nm = " -- ADC A,L 1,4 z1hc\n";
				cpu->pc += 1;
				break;
			case 0x9e:
				nm = " -- ADC A,[HL] 1,8 z1hc\n";
				cpu->pc += 1;
				break;
			case 0x9f:
				nm = " -- ADC A,A 1,4 z1hc\n";
				cpu->pc += 1;
				break;
			}
		} else if (op >= 0x80) {
			switch (op) {
			case 0x80:
				nm = " -- ADD A,B 1,4 z0hc\n";
				cpu->pc += 1;
				break;
			case 0x81:
				nm = " -- ADD A,C 1,4 z0hc\n";
				cpu->pc += 1;
				break;
			case 0x82:
				nm = " -- ADD A,D 1,4 z0hc\n";
				cpu->pc += 1;
				break;
			case 0x83:
				nm = " -- ADD A,E 1,4 z0hc\n";
				cpu->pc += 1;
				break;
			case 0x84:
				nm = " -- ADD A,H 1,4 z0hc\n";
				cpu->pc += 1;
				break;
			case 0x85:
				nm = " -- ADD A,L 1,4 z0hc\n";
				cpu->pc += 1;
				break;
			case 0x86:
				nm = " -- ADD A,[HL] 1,8 z0hc\n";
				cpu->pc += 1;
				break;
			case 0x87:
				nm = " -- ADD A,A 1,4 z0hc\n";
				cpu->pc += 1;
				break;
			case 0x88:
				nm = " -- ADC A,B 1,4 z0hc\n";
				cpu->pc += 1;
				break;
			case 0x89:
				nm = " -- ADC A,C 1,4 z0hc\n";
				cpu->pc += 1;
				break;
			case 0x8a:
				nm = " -- ADC A,D 1,4 z0hc\n";
				cpu->pc += 1;
				break;
			case 0x8b:
				nm = " -- ADC A,E 1,4 z0hc\n";
				cpu->pc += 1;
				break;
			case 0x8c:
				nm = " -- ADC A,H 1,4 z0hc\n";
				cpu->pc += 1;
				break;
			case 0x8d:
				nm = " -- ADC A,L 1,4 z0hc\n";
				cpu->pc += 1;
				break;
			case 0x8e:
				nm = " -- ADC A,[HL] 1,8 z0hc\n";
				cpu->pc += 1;
				break;
			case 0x8f:
				nm = " -- ADC A,A 1,4 z0hc\n";
				cpu->pc += 1;
				break;
			}
		} else if (op >= 0x70) {
			switch (op) {
			case 0x70:
				nm = " -- LD [HL],B 1,8\n";
				cpu->pc += 1;
				break;
			case 0x71:
				nm = " -- LD [HL],C 1,8\n";
				cpu->pc += 1;
				break;
			case 0x72:
				nm = " -- LD [HL],D 1,8\n";
				cpu->pc += 1;
				break;
			case 0x73:
				nm = " -- LD [HL],E 1,8\n";
				cpu->pc += 1;
				break;
			case 0x74:
				nm = " -- LD [HL],H 1,8\n";
				cpu->pc += 1;
				break;
			case 0x75:
				nm = " -- LD [HL],L 1,8\n";
				cpu->pc += 1;
				break;
			case 0x76:
				nm = " -- HALT 1,4\n";
				cpu->pc += 1;
				break;
			case 0x77:
				nm = " -- LD [HL],A 1,8\n";
				cpu->pc += 1;
				break;
			case 0x78:
				nm = " -- LD A,B 1,4\n";
				cpu->a = cpu->b;
				impl = true;
				break;
			case 0x79:
				nm = " -- LD A,C 1,4\n";
				cpu->a = cpu->c;
				impl = true;
				break;
			case 0x7A:
				nm = " -- LD A,D 1,4\n";
				cpu->pc += 1;
				break;
			case 0x7B:
				nm = " -- LD A,E 1,4\n";
				cpu->pc += 1;
				break;
			case 0x7C:
				nm = " -- LD A,H 1,4\n";
				cpu->pc += 1;
				break;
			case 0x7D:
				nm = " -- LD A,L 1,4\n";
				cpu->pc += 1;
				break;
			case 0x7e:
				nm = " -- LD A,[HL] 1,8\n";
				cpu->pc += 1;
				break;
			case 0x7f:
				nm = " -- LD A,A 1,4\n";
				cpu->pc += 1;
				break;
			default:
				break;
			}
		} else if (op >= 0x60) {
			switch (op) {
			case 0x60:
				nm = " -- LD H,B 1,4 \n";
				cpu->pc += 1;
				break;
			case 0x61:
				nm = " -- LD H,C 1,4 \n";
				cpu->pc += 1;
				break;
			case 0x62:
				nm = " -- LD H,D 1,4 \n";
				cpu->pc += 1;
				break;
			case 0x63:
				nm = " -- LD H,E 1,4 \n";
				cpu->pc += 1;
				break;
			case 0x64:
				nm = " -- LD H,H 1,4 \n";
				cpu->pc += 1;
				break;
			case 0x65:
				nm = " -- LD H,L 1,4 \n";
				cpu->pc += 1;
				break;
			case 0x66:
				nm = " -- LD H,[HL] 1,8 \n";
				cpu->pc += 1;
				break;
			case 0x67:
				nm = " -- LD H,A 1,4 \n";
				cpu->pc += 1;
				break;
			case 0x68:
				nm = " -- LD L,B 1,4 \n";
				cpu->pc += 1;
				break;
			case 0x69:
				nm = " -- LD L,C 1,4 \n";
				cpu->pc += 1;
				break;
			case 0x6a:
				nm = " -- LD L,D 1,4 \n";
				cpu->pc += 1;
				break;
			case 0x6b:
				nm = " -- LD L,E 1,4 \n";
				cpu->pc += 1;
				break;
			case 0x6c:
				nm = " -- LD L,H 1,4 \n";
				cpu->pc += 1;
				break;
			case 0x6d:
				nm = " -- LD L,L 1,4 \n";
				cpu->pc += 1;
				break;
			case 0x6e:
				nm = " -- LD L,[HL] 1,8 \n";
				cpu->pc += 1;
				break;
			case 0x6f:
				nm = " -- LD L,A 1,4\n";
				cpu->pc += 1;
				break;
			default:
				break;
			}
		} else if (op >= 0x50) {
			switch (op) {
			case 0x50:
				nm = " -- LD D,B 1,4 \n";
				cpu->pc += 1;
				break;
			case 0x51:
				nm = " -- LD D,C 1,4 \n";
				cpu->pc += 1;
				break;
			case 0x52:
				nm = " -- LD D,D 1,4 \n";
				cpu->pc += 1;
				break;
			case 0x53:
				nm = " -- LD D,E 1,4 \n";
				cpu->pc += 1;
				break;
			case 0x54:
				nm = " -- LD D,H 1,4 \n";
				cpu->pc += 1;
				break;
			case 0x55:
				nm = " -- LD D,L 1,4 \n";
				cpu->pc += 1;
				break;
			case 0x56:
				nm = " -- LD D,[HL] 1,8 \n";
				cpu->pc += 1;
				break;
			case 0x57:
				nm = " -- LD D,A 1,4 \n";
				cpu->pc += 1;
				break;
			case 0x58:
				nm = " -- LD E,B 1,4 \n";
				cpu->pc += 1;
				break;
			case 0x59:
				nm = " -- LD E,C 1,4 \n";
				cpu->pc += 1;
				break;
			case 0x5a:
				nm = " -- LD E,D 1,4 \n";
				cpu->pc += 1;
				break;
			case 0x5b:
				nm = " -- LD E,E 1,4 \n";
				cpu->pc += 1;
				break;
			case 0x5c:
				nm = " -- LD E,H 1,4 \n";
				cpu->pc += 1;
				break;
			case 0x5d:
				nm = " -- LD E,L 1,4 \n";
				cpu->pc += 1;
				break;
			case 0x5e:
				nm = " -- LD E,[HL] 1,8 \n";
				cpu->pc += 1;
				break;
			case 0x5f:
				nm = " -- LD E,A 1,4\n";
				cpu->pc += 1;
				break;
			default:
				break;
			}
		} else if (op >= 0x40) {
			switch (op) {
			case 0x40:
				nm = " -- LD B,B 1,4 \n";
				cpu->pc += 1;
				break;
			case 0x41:
				nm = " -- LD B,C 1,4 \n";
				cpu->pc += 1;
				break;
			case 0x42:
				nm = " -- LD B,D 1,4 \n";
				cpu->pc += 1;
				break;
			case 0x43:
				nm = " -- LD B,E 1,4 \n";
				fprintf(stderr, " -- pre b 0x%02x e 0x%02x",
					cpu->b, cpu->e);
				cpu->b = cpu->e;
				fprintf(stderr, " -- post b 0x%02x e 0x%02x",
					cpu->b, cpu->e);
				impl = true;
				break;
			case 0x44:
				nm = " -- LD B,H 1,4 \n";
				cpu->pc += 1;
				break;
			case 0x45:
				nm = " -- LD B,L 1,4 \n";
				cpu->pc += 1;
				break;
			case 0x46:
				nm = " -- LD B,[HL] 1,8 \n";

				sr = *(uint16_t *)(&cg->slb[cpu->pc]);
				fprintf(stderr,
					" -- LD B,[HL] 1,8 --  hl 0x%04x, [hl]  0x%02x, cpu->b 0x%02x",
					cpu->hl, cpu->b, cg->slb[cpu->hl]);
				cpu->b = cg->slb[cpu->hl];
				fprintf(stderr, " ## cpu->b 0x%02x", cpu->b);
				impl = true;
				break;
			case 0x47:
				nm = " -- LD B,A 1,4 \n";
				cpu->pc += 1;
				break;
			case 0x48:
				nm = " -- LD C,B 1,4 \n";
				cpu->pc += 1;
				break;
			case 0x49:
				nm = " -- LD C,C 1,4 \n";
				cpu->pc += 1;
				break;
			case 0x4a:
				nm = " -- LD C,D 1,4 \n";
				cpu->pc += 1;
				break;
			case 0x4b:
				nm = " -- LD C,E 1,4 \n";
				cpu->pc += 1;
				break;
			case 0x4c:
				nm = " -- LD C,H 1,4 \n";
				cpu->pc += 1;
				break;
			case 0x4d:
				nm = " -- LD C,L 1,4 \n";
				cpu->pc += 1;
				break;
			case 0x4e:
				nm = " -- LD C,[HL] 1,8 \n";
				cpu->pc += 1;
				break;
			case 0x4f:
				nm = " -- LD C,A 1,4\n";
				cpu->c = cpu->a;
				impl = true;
				break;
			default:
				break;
			}
		} else if (op >= 0x30) {
			switch (op) {
			case 0x30:
				nm = " -- JR NZ,e8 2,12/8\n";
				cpu->pc += 2;
				break;
			case 0x31:
				fprintf(stderr, " -- LD sp,n16 3,12");
				nm = " -- LD sp,n16 3,12\n";
				cpu->sp = (uint16_t)(*(
					(uint16_t *)(cg->slb + cpu->pc)));
				fprintf(stderr,
					" -- addr 0x%04x -> 0x%02x 0x%02x 0x%02x 0x%02x 0x%02x 0x%02x 0x%02x",
					cpu->pc - 1, *(cg->slb + cpu->pc - 1),
					*(cg->slb + cpu->pc),
					*(cg->slb + cpu->pc + 1),
					*(cg->slb + cpu->pc + 2),
					*(cg->slb + cpu->pc + 3),
					*(cg->slb + cpu->pc + 4),
					*(cg->slb + cpu->pc + 5));
				fprintf(stderr, " -> sp = %02x", cpu->sp);
				cpu->pc += 2;
				impl = true;
				break;
			case 0x32:
				nm = " -- LD [HL+], A 1, 8\n";
				cpu->pc += 1;
				break;
			case 0x33:
				nm = " -- INC SP 1,8\n";
				cpu->pc += 1;
				break;
			case 0x34:
				nm = " -- INC [HL] 1,12 z0h-\n";
				cpu->pc += 1;
				break;
			case 0x35:
				nm = " -- DEC [HL] 1,12 z1h-\n";
				cpu->pc += 1;
				break;
			case 0x36:
				nm = " -- LD [HL],n8 2,12\n";
				cpu->pc += 2;
				break;
			case 0x37:
				nm = " -- SCF 1,4 -001c\n";
				cpu->pc += 1;
				break;
			case 0x38:
				nm = " -- JR C,e8 2,12/8\n";
				cpu->pc += 2;
				break;
			case 0x39:
				nm = " -- ADD HL,SP 1,8 -0hc\n";
				cpu->pc += 1;
				break;
			case 0x3a:
				nm = " -- LD A,[HL-] 1,8\n";
				cpu->pc += 1;
				break;
			case 0x3b:
				nm = " -- DEC SP 1,8\n";
				cpu->pc += 1;
				break;
			case 0x3c:
				nm = " -- INC A 1,4 z0h-\n";
				cpu->pc += 1;
				break;
			case 0x3d:
				nm = " -- DEC A 1,4 z1h-\n";
				cpu->pc += 1;
				break;
			case 0x3e:
				nm = " -- LD A,n8 2,8\n";
				fprintf(stderr,
					" -- LD A,n8 2,8 -- cpu->a 0x%02x",
					cpu->a);
				cpu->a = cg->slb[cpu->pc++];
				fprintf(stderr, " --> aft 0x%02x", cpu->a);
				impl = true;
				break;
			case 0x3f:
				nm = " -- CCF 1,4 -00c\n";
				cpu->pc += 1;
				break;
			default:
				break;
			}
		} else if (op >= 0x20) {
			switch (op) {
			case 0x20:
				nm = " -- JR NZ,e8 2,12/8\n";
				fprintf(stderr, " -- JR NZ,e8 2,12/8");
				fprintf(stderr, " ** cpu->f 0b%08b", cpu->f);

				int8_t off = cg->slb[cpu->pc++];
				if (!cpu->fZ) {
					fprintf(stderr,
						" cc branch NZ = true e8 0x%02x (unsgn %d, sgn %d)",
						off, (uint8_t)off, (int8_t)off);
					cpu->pc += off;
				} else {
					fprintf(stderr,
						" cc branch NZ = false 0b%08b",
						cpu->f);
				}
				impl = true;
				break;
			case 0x21:
				nm = " -- LD HL,n16 3,12\n";
				fprintf(stderr,
					" -- LD HL,n16 3,12 pre cpu->hl 0x%04x",
					cpu->hl);
				cpu->hl = *(uint16_t *)(&cg->slb[cpu->pc]);
				fprintf(stderr, " $$ post cpu->hl 0x%04x",
					cpu->hl);
				cpu->pc += 2;
				impl = true;
				break;
			case 0x22:
				nm = " -- LD [HL+],A 1,8\n";
				cpu->pc += 1;
				break;
			case 0x23:
				nm = " -- INC HL 1,8\n";
				cpu->pc += 1;
				break;
			case 0x24:
				nm = " -- INC H 1,4 z0h-\n";
				cpu->pc += 1;
				break;
			case 0x25:
				nm = " -- DEC H 1,4 z1h-\n";
				cpu->pc += 1;
				break;
			case 0x26:
				nm = " -- LD H,n8 2,8\n";
				cpu->pc += 2;
				break;
			case 0x27:
				nm = " -- DAA 1,4 z-0c\n";
				cpu->pc += 1;
				break;
			case 0x28:
				nm = " -- JR Z,e8 2,12\n";
				cpu->pc += 2;
				break;
			case 0x29:
				nm = " -- ADD HL,HL 1,8 -0hc\n";
				cpu->pc += 1;
				break;
			case 0x2a:
				nm = " -- LD A,[HL+] 1,8\n";
				cpu->pc += 1;
				break;
			case 0x2b:
				nm = " -- DEC HL 1,8\n";
				cpu->pc += 1;
				break;
			case 0x2c:
				nm = " -- INC L 1,4 z0h-\n";
				cpu->pc += 1;
				break;
			case 0x2d:
				nm = " -- DEC L 1,4 z1h-\n";
				cpu->pc += 1;
				break;
			case 0x2e:
				nm = " -- LD L,n8 2,8\n";
				cpu->pc += 2;
				break;
			case 0x2f:
				nm = " -- CPL 1,4 -11-\n";
				cpu->pc += 1;
				break;
			default:
				break;
			}
		} else if (op >= 0x10) {
			switch (op) {
			case 0x10:
				nm = " -- STOP n8 2,4\n";
				cpu->pc += 1;
				cpu->dblspd = false;
				impl = true;
				break;
			case 0x11:
				nm = " -- LD DE,n16 3,12\n";
				cpu->pc += 3;
				break;
			case 0x12:
				nm = " -- LD [DE],A 1,8\n";
				cpu->pc += 1;
				break;
			case 0x13:
				nm = " -- INC DE 1,8\n";
				cpu->pc += 1;
				break;
			case 0x14:
				nm = " -- INC D 1,4 z0h-\n";
				cpu->pc += 1;
				break;
			case 0x15:
				nm = " -- DEC D 1,4 z1h-\n";
				cpu->pc += 1;
				break;
			case 0x16:
				nm = " -- LD D,n8 2,8\n";
				cpu->pc += 2;
				break;
			case 0x17:
				nm = " -- RLA 1,4 000c\n";
				cpu->pc += 1;
				break;
			case 0x18:
				nm = " -- JR r8 2,12\n";
				int8_t r8 = cg->slb[cpu->pc++];

				cpu->pc += r8;
				impl = true;
				break;
			case 0x19:
				nm = " -- ADD HL,DE 1,8 -0hc\n";
				cpu->pc += 1;
				break;
			case 0x1a:
				nm = " -- LD A,[DE] 1,8\n";
				cpu->pc += 1;
				break;
			case 0x1b:
				nm = " -- DEC  DE 1,8\n";
				cpu->pc += 1;
				break;
			case 0x1c:
				nm = " -- INC E 1,4 z0h-\n";
				cpu->pc += 1;
				break;
			case 0x1d:
				nm = " -- DEC E 1,4 z1h-\n";
				cpu->pc += 1;
				break;
			case 0x1e:
				nm = " -- LD E,n8 2,8\n";
				cpu->pc += 2;
				break;
			case 0x1f:
				nm = " -- RRA 1,4 000c\n";
				cpu->pc += 1;
				break;
			default:
				break;
			}
		} else if (op >= 0x00) {
			switch (op) {
			case 0x00:
				nm = " -- NOP 1,4\n";
				cpu->pc++;
				break;
			case 0x01:
				nm = " -- LD BC,n16 3,12\n";

				fprintf(stderr,
					" $$ -- LD BC,n16 3,12 ** bc = 0x%04x, b 0x%02x, c 0x%02x",
					cpu->bc, cpu->b, cpu->c);
				cpu->bc = (uint16_t)(*(
					(uint16_t *)(cg->slb + cpu->pc)));
				fprintf(stderr, " @@@ bc = %04x (%d)", cpu->bc,
					cpu->bc);
				fprintf(stderr,
					" ** 0x%04x 0x%02x 0x%02x 0x%02x 0x%02x 0x%02x 0x%02x",
					cpu->pc, *(cg->slb + cpu->pc),
					*(cg->slb + cpu->pc + 1),
					*(cg->slb + cpu->pc + 2),
					*(cg->slb + cpu->pc + 3),
					*(cg->slb + cpu->pc + 4),
					*(cg->slb + cpu->pc + 5));
				cpu->pc += 2;
				impl = true;
				break;
			case 0x02:
				nm = " -- LD [BC],A 1,8\n";
				cpu->pc += 1;
				break;
			case 0x03:
				nm = " -- INC BC 1,8\n";
				cpu->pc += 1;
				break;
			case 0x04:
				nm = " -- INC B 1,4 z0h-\n";
				cpu->pc += 1;
				break;
			case 0x05:
				nm = " -- DEC B 1,4 z1h-\n";
				cpu->pc += 1;
				break;
			case 0x06:
				nm = " -- LD B,n8 2,8\n";
				//cpu->pc += 2;
				cpu->b = cg->slb[cpu->pc++];
				impl = true;

				break;
			case 0x07:
				nm = " -- RLCA 1,4 000c\n";
				cpu->pc += 1;
				break;
			case 0x08:
				nm = " -- LD [a16],sp 3,20\n";
				cpu->pc += 3;
				break;
			case 0x09:
				nm = " -- ADD HL,BC 1,8 -0hc\n";
				cpu->pc += 1;
				break;
			case 0x0a:
				nm = " -- LD A,[BC] 1,8\n";
				cpu->pc += 1;
				break;
			case 0x0b:
				nm = " -- DEC  BC 1,8\n";
				cpu->pc += 1;
				break;
			case 0x0c:
				nm = " -- INC C 1,4 z0h-\n";
				cpu->pc += 1;
				break;
			case 0x0d:
				nm = " -- DEC C 1,4 z1h-\n";
				cpu->pc += 1;
				break;
			case 0x0e:
				nm = " -- LD C,n8 2,8\n";
				cpu->pc += 2;
				break;
			case 0x0f:
				nm = " -- RRCA 1,4 000c\n";
				cpu->pc += 1;
				break;
			default:
				break;
			}
		}

		char ns[strlen(nm)];
		ns[strlen(nm) - 1] = '\0';
		memcpy(ns, nm, strlen(nm) - 1);
		fprintf(stderr, "%s", ns);
		if (pre || !impl) {
			if (!impl)
				fprintf(stderr,
					" !!! ERROR: NOT IMPLEMENTED\n");
			if (pre)
				fprintf(stderr,
					"\n=========================\nEND prefix\n=========================\n");
			if (!impl) {
				fprintf(stderr,
					"\n0x%04x: ", MIN(pctm, cpu->pc));
				int in = 0;
				for (int i = MIN(pctm, cpu->pc); in++ < 25;
				     i++) {
					if (i == cpu->pc)
						fprintf(stderr, " ## 0x%04x: ",
							cpu->pc);
					fprintf(stderr, "%02x ", cg->slb[i]);
				}
				fprintf(stderr, "\n");

				fprintf(stderr, "0xcfff - 0x%04x (sp) -- \n",
					cpu->sp);
				for (int i = 0xcfff; i > cpu->sp; i -= 2)
					fprintf(stderr, "0x%04x ",
						cg->slb[i] |
							(cg->slb[i - 1] << 8));
				fprintf(stderr, "\n");
				//exit(1);
				break;
			}
		}
		pre = false;
		impl = false;
		fprintf(stderr, "\n");
		// if (cpu->pc == 0x38) {
		if (pctm == 0x423 && (loopcount++ == 3)) {
			fprintf(stderr, "\n");
			for (int i = 0x38; i < 0x38 + 10; i++)
				fprintf(stderr, "0x%04x: 0x%02x, ", i,
					cg->slb[i]);
			fprintf(stderr, "\n\n0xcfff - 0x%04x (sp) -- ",
				cpu->sp);
			for (int i = 0xcfff; i > cpu->sp; i -= 2)
				fprintf(stderr, "\n0x%04x = 0x%04x", i - 2,
					cg->slb[i - 2] | (cg->slb[i - 1] << 8));
			fprintf(stderr, "\n\n============================\n\n");
			cpu->pgm[0xff44] = 0x91;
			//exit(1);
			//break;
		}
	}

	fprintf(stderr, "\nLCD Registers\n=============\n\n");
	fprintf(stderr, "0xff4f: 0x%02x vbk\n0xff70: 0x%02x svbk\n",
		cpu->pgm[0xff4f], cpu->pgm[0xff70]);
	char *rns[] = { "lcdc: bg/wnd display switch",
			"stat: current status of lcd controller",
			"scy: scroll y",
			"scx: scroll x",
			"ly: current scanline",
			"lyc: if == ly, set STAT coincedent flag (Interrupt)",
			"dma: dma transfer start address",
			"bgp: bg palette data",
			"obp0: object palette 0 data",
			"obp1: object palette 1 data",
			"wy: window y position",
			"wx: window x position" };
	for (int i = 0xff40; i <= 0xff4b; i++) {
		fprintf(stderr, "0x%04x: 0x%02x (0b%08b) %s\n", i, cpu->pgm[i],
			cpu->pgm[i], rns[i - 0xff40]);
	}
	fprintf(stderr, "\nBG window\n");
	int byrwsz = 32;
	int off = 0x9800;
	for (int i = 0; i < 256; i++) {
		if (i % byrwsz == 0)
			fprintf(stderr, "\n0x%04x: ", off + i);
		fprintf(stderr, "%02x ", cpu->pgm[off + i]);
	}
	fprintf(stderr, "\n\n");
	byrwsz = 16;
	off = 0x8000;
	for (int i = 0; i < 4096; i++) {
		if (i % byrwsz == 0)
			fprintf(stderr, "\n0x%04x: ", off + i);
		fprintf(stderr, "%02x ", cpu->pgm[off + i]);
	}
	fprintf(stderr, "\n\n");
	return cpu;
}
/* This function runs once at startup. */
SDL_AppResult SDL_AppInit(void **appstate, int argc, char *argv[])
{
	/* Create the window */
	cpu = startGame();
	int rowsize = 32;
	for (int i = 0; i < 32 * 32; i++) {
		uint16_t offset = cpu->pgm[0x9800 + i] * 16;
		fprintf(stderr, "%04x: %d -> %02x (0x%04x)\n", 0x9800 + i,
			cpu->pgm[0x9800 + i], offset, 0x8000 + offset);
		memcpy(sctiles[i], &cpu->pgm[0x8000 + offset], 16);
	}
	for (int i = 0; i < 32 * 32; i++) {
		if (i % 2 == 0)
			fprintf(stderr, "\n%03d: ", i);
		else
			fprintf(stderr, " |  ");
		for (int j = 0; j < 16; j++) {
			fprintf(stderr, "%02x ", sctiles[i][j]);
		}
	}
	//exit(1);
	if (!SDL_CreateWindowAndRenderer(
		    //"Hello World", 160 * 7, 160 * 7,
		    "Hello World", 160 * 7, 144 * 7, SDL_WINDOW_ALWAYS_ON_TOP,
		    &window, &renderer)) {
		SDL_Log("Couldn't create window and renderer: %s",
			SDL_GetError());
		return SDL_APP_FAILURE;
	}
	return SDL_APP_CONTINUE;
}

/* This function runs when a new event (mouse input, keypresses, etc) occurs. */
SDL_AppResult SDL_AppEvent(void *appstate, SDL_Event *event)
{
	if (event->type == SDL_EVENT_KEY_DOWN ||
	    event->type == SDL_EVENT_QUIT) {
		return SDL_APP_SUCCESS; /* end the program, reporting success to the OS. */
	}
	return SDL_APP_CONTINUE;
}

static uint64_t tckms = 0, tckns = 0;
static int frcnt = -1;
static bool doneprnt = false;
//static char ijstr[][];

/* This function runs once per frame, and is the heart of the program. */
SDL_AppResult SDL_AppIterate(void *appstate)
{
	const char *message = "Hello World!";
	int w = 0, h = 0;
	float x, y;
	const float scale = 7.0f;
	/* Center the message and scale it up */
	SDL_GetRenderOutputSize(renderer, &w, &h);
	SDL_SetRenderScale(renderer, scale, scale);
	x = ((w / scale) -
	     SDL_DEBUG_TEXT_FONT_CHARACTER_SIZE * SDL_strlen(message)) /
	    2;
	y = ((h / scale) - SDL_DEBUG_TEXT_FONT_CHARACTER_SIZE) / 2;

	/* Draw the message */
	SDL_SetRenderDrawColor(renderer, 0, 128, 255, 255);
	SDL_RenderClear(renderer);
	SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
	SDL_RenderDebugText(renderer, x, y, message);

	//SDL_SetRenderScale(renderer, 1, 1);

	int yc = 0;
	for (int sl = 0; sl < 144; sl++) {
		fprintf(stderr, "\nscanline %d: yc %d", sl + 1, yc);
		if (sl % 8 == 0) {
			// get new tiles
		}
		//exit(1);

		for (int tl = 0; tl < 20; tl++) {
			uint8_t *tile = sctiles[yc + tl];

			int off = (sl % 8) * 2;
			uint8_t lo = tile[off];
			uint8_t hi = tile[off + 1];
			fprintf(stderr, "\ntl (%d) in %02d:  %02x %02x  ",
				yc + tl, tl, lo, hi);
			int cur = 0;
			for (int i = 0b10000000; i > 0; i /= 2) {
				uint8_t colorbyy = (hi >> 6) & (lo >> 7) & 0b11;
				uint8_t colorby = (hi & i) & (lo & i) & 0b11;
				if ((hi & i) && (lo & i)) {
					// 3: black
					SDL_SetRenderDrawColor(renderer, 0, 0,
							       0, 255);
				} else if (hi & i) {
					// 2: dark grey
					SDL_SetRenderDrawColor(renderer, 55, 55,
							       55, 255);

				} else if (lo & i) {
					// 1: light grey
					SDL_SetRenderDrawColor(renderer, 170,
							       170, 170, 255);
				} else {
					//white
					SDL_SetRenderDrawColor(renderer, 255,
							       255, 255, 255);
				}
				//fprintf(stderr, "%d %02b :: ", i, colorby);
				//if (lo & i)
				// SDL_RenderT
				fprintf(stderr, "x: %d, ", (int)(tl * 8 + cur));
				SDL_RenderPoint(renderer, tl * 8 + cur, sl);
				cur++;
			}
			//SDL_RenderPoint(renderer, tl, sl - 1);
		}
		if ((sl + 1) % 8 == 0)
			yc += 32;
		fprintf(stderr, "\n");
		// break;
	}

	fprintf(stderr, "\n");
	SDL_RenderPresent(renderer);
	return SDL_APP_CONTINUE;
}

SDL_AppResult SDL_AppIteratee(void *appstate)
{
	const char *message = "Hello World!";
	int w = 0, h = 0;
	float x, y;
	const float scale = 17.0f;
	uint8_t fir[] = { 0x7c, 0x7c, 0x00, 0xc6, 0xc6, 0x0, 0x0, 0xfe,
			  0xc6, 0xc6, 0x00, 0xc6, 0xc6, 0x0, 0x0, 0x0 };

	uint8_t sec[] = { 0xFF, 0x0,  0x7E, 0xFF, 0x85, 0x81, 0x89, 0x83,
			  0x93, 0x85, 0xA5, 0x8B, 0xC9, 0x97, 0x7E, 0xFF };
	uint8_t *tiles[16] = { fir, sec, fir, sec };
	/* Center the message and scale it up */
	SDL_GetRenderOutputSize(renderer, &w, &h);
	SDL_SetRenderScale(renderer, scale, scale);
	x = ((w / scale) -
	     SDL_DEBUG_TEXT_FONT_CHARACTER_SIZE * SDL_strlen(message)) /
	    2;
	y = ((h / scale) - SDL_DEBUG_TEXT_FONT_CHARACTER_SIZE) / 2;

	/* Draw the message */
	SDL_SetRenderDrawColor(renderer, 0, 128, 255, 255);
	SDL_RenderClear(renderer);
	SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
	SDL_RenderDebugText(renderer, x, y, message);

	//SDL_SetRenderScale(renderer, 1, 1);
	for (int sl = 0; sl < 16; sl++) {
		fprintf(stderr, "scanline %d: ", sl + 1);
		for (int tl = 0; tl < 4; tl++) {
			uint8_t *tile = tiles[tl];

			int off = (sl % 8) * 2;
			uint8_t lo = tile[off];
			uint8_t hi = tile[off + 1];
			fprintf(stderr, "0x%02x 0x%02x    ", lo, hi);
			int cur = 0;
			for (int i = 0b10000000; i > 0; i /= 2) {
				uint8_t colorbyy = (hi >> 6) & (lo >> 7) & 0b11;
				uint8_t colorby = (hi & i) & (lo & i) & 0b11;
				if ((hi & i) && (lo & i)) {
					// 3: black
					SDL_SetRenderDrawColor(renderer, 0, 0,
							       0, 255);
				} else if (hi & i) {
					// 2: dark grey
					SDL_SetRenderDrawColor(renderer, 255, 0,
							       0, 255);

				} else if (lo & i) {
					// 1: light grey
					SDL_SetRenderDrawColor(renderer, 150,
							       150, 150, 255);
				} else {
					//white
					SDL_SetRenderDrawColor(renderer, 255,
							       255, 255, 255);
				}
				//fprintf(stderr, "%d %02b :: ", i, colorby);
				//if (lo & i)
				// SDL_RenderT
				SDL_RenderPoint(renderer, tl * scale + cur, sl);
				cur++;
			}
			//SDL_RenderPoint(renderer, tl, sl - 1);
		}
		fprintf(stderr, "\n");
	}

	fprintf(stderr, "\n");
	SDL_RenderPresent(renderer);
	return SDL_APP_CONTINUE;
}

SDL_AppResult SDL_AppIterate2(void *appstate)
{
	const char *message = "Hello World!";
	int w = 0, h = 0, i = 0, j = 0, rw, rh;
	float x, y;
	const float scale = 8.0f;
	uint64_t dltms, dltns;
	bool odd = false;

	/* Center the message and scale it up */
	SDL_GetRenderOutputSize(renderer, &w, &h);
	x = ((w / scale) -
	     SDL_DEBUG_TEXT_FONT_CHARACTER_SIZE * SDL_strlen(message)) /
	    2;
	y = ((h / scale) - SDL_DEBUG_TEXT_FONT_CHARACTER_SIZE) / 2;

	dltns = SDL_GetTicksNS() - tckns;
	dltms = SDL_GetTicks() - tckms;
	if (++frcnt > 0 && frcnt <= 20) {
		fprintf(stderr,
			"%02d: %ld ms (%04f mcs) (%ld ns)    --->>   dt %f\n",
			frcnt, dltms, dltns / 1000.0f, dltns,
			dltns / 1000000.0f);
	}

	rw = w / 20;
	rh = h / 18;
	//fprintf(stderr, "rw %d, rh %d\n", rw, rh);
	tckms += dltms;
	tckns += dltns;

	/* Draw the message */
	SDL_RenderClear(renderer);
	SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255);
	SDL_SetRenderScale(renderer, 1, 1);
	const int red = 0xff0000, green = 0x00ff00, blue = 0x0000ff;
	int color = red;
	int cnt = 0;
	int ind = 0, row = 0;
	for (i = 0; i < h; i += rh) {
		if (row % 2 != 0)
			color = green;
		else
			color = red;
		row++;
		for (j = 0; j < w; j += rw) {
			// if (frcnt == 3)
			//fprintf(stderr, "i: %d, j %d color %d\n", i, j, color);
			if (color == red) {
				SDL_SetRenderDrawColor(renderer, 255, 0, 128,
						       255);
				color = green;
			} else {
				SDL_SetRenderDrawColor(renderer, 128, 128, 0,
						       255);
				color = red;
			}

			char ist[32];
			snprintf(ist, sizeof(ist), "%d", ind++);
			SDL_FRect rct = { .x = j, .y = i, .w = rw, .h = rh };
			SDL_RenderFillRect(renderer, &rct);

			SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
			SDL_RenderDebugText(renderer, j + (rw / 2.5f),
					    i + (rh / 2.2f), ist);
		}
	}

	SDL_SetRenderScale(renderer, scale, scale);
	SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255);
	SDL_RenderDebugText(renderer, x, y, message);
	//SDL_FillSurfaceRect(renderer, &rct);
	SDL_RenderPresent(renderer);
	/*
	if (frcnt > 50)

		fprintf(stderr, "%d: delta time for squares: %f ms\n", frcnt,
			(SDL_GetTicksNS() - tckns) / 1000000.0f);
        */
	return SDL_APP_CONTINUE;
}

SDL_AppResult SDL_AppIterate3(void *appstate)
{
	const char *message = "Hello World!";
	int w = 0, h = 0;
	float x, y;
	const float scale = 4.0f;

	/* Center the message and scale it up */
	SDL_GetRenderOutputSize(renderer, &w, &h);
	SDL_SetRenderScale(renderer, scale, scale);
	x = ((w / scale) -
	     SDL_DEBUG_TEXT_FONT_CHARACTER_SIZE * SDL_strlen(message)) /
	    2;
	y = ((h / scale) - SDL_DEBUG_TEXT_FONT_CHARACTER_SIZE) / 2;

	/* Draw the message */
	SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
	SDL_RenderClear(renderer);
	SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255);
	SDL_RenderDebugText(renderer, x, y, message);
	SDL_RenderPresent(renderer);

	return SDL_APP_CONTINUE;
}

/* This function runs once at shutdown. */
void SDL_AppQuit(void *appstate, SDL_AppResult result)
{
}
