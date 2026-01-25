#include <stdlib.h>
#include <stdio.h>
#include <fcntl.h>
#include <unistd.h>
#include "CPU.h"
#include "registers.h"

// portable 16 bit operations for BE/LE compatability
static inline uint16_t ld_le16(const uint8_t *src)
{
	return (uint16_t)src[0] | ((uint16_t)src[1] << 8);
}

static inline void wr_le16(uint8_t *dest, uint16_t val)
{
	dest[0] = val & 0xff;
	dest[1] = (val >> 8) & 0xff;
}

uint8_t op_noop(struct CPU *cpu) //0x00
{
	return cpu->instr->dots[0];
}

uint8_t op_jr_NZ_e8(struct CPU *cpu) //0x20
{
	int8_t e8 = cpu->rom[cpu->pc++];
	if (cpu->fZ) {
		fprintf(stderr, "NZ false %08b ", cpu->f);
		return cpu->instr->dots[1];
	}
	fprintf(stderr, "NZ TRUE %08b", cpu->f);
	cpu->pc += e8;
	return cpu->instr->dots[0];
}

uint8_t op_ld_HL_n16(struct CPU *cpu) //0x21
{
	cpu->hl = ld_le16(cpu->rom + cpu->pc);
	cpu->pc += 2;
	return cpu->instr->dots[0];
}

uint8_t op_ld_sp_n16(struct CPU *cpu) //0x31
{
	cpu->sp = ld_le16(cpu->rom + cpu->pc);
	cpu->pc += 2;
	return cpu->instr->dots[0];
}

uint8_t op_ld_A_n8(struct CPU *cpu) //0x3e
{
	cpu->a = cpu->rom[cpu->pc++];
	return cpu->instr->dots[0];
}

uint8_t op_or_A_A(struct CPU *cpu) //0xb7
{
	cpu->a |= cpu->a;
	cpu->f = 0;
	cpu->fZ = cpu->a == 0 ? 1 : 0;
	return cpu->instr->dots[0];
}

uint8_t op_ret_NZ(struct CPU *cpu) //0xc0
{
	fprintf(stderr, "fZ=%b ", cpu->fZ);
	if (cpu->fZ) {
		fprintf(stderr, "fZ == 1 NZ=FALSE ");
		return cpu->instr->dots[1];
	}
	fprintf(stderr, "fZ == 0 NZ=TRUE ");
	cpu->pc = ld_le16(cpu->rom + cpu->sp);
	cpu->sp += 2;
	return cpu->instr->dots[0];
}

uint8_t op_ret_Z(struct CPU *cpu) //0xc8
{
	fprintf(stderr, "fZ=%b ", cpu->fZ);
	if (!cpu->fZ) {
		fprintf(stderr, "fZ == 0 FALSE ");
		return cpu->instr->dots[1];
	}
	fprintf(stderr, "fZ == 0 TRUE ");
	cpu->pc = ld_le16(cpu->rom + cpu->sp);
	cpu->sp += 2;
	return cpu->instr->dots[0];
}

uint8_t op_call_a16(struct CPU *cpu) //0xcd
{
	int16_t a16 = ld_le16(cpu->rom + cpu->pc);
	cpu->pc += 2;
	cpu->sp -= 2;
	wr_le16(cpu->rom + cpu->sp, cpu->pc);

	fprintf(stderr, "pc=0x%04x a16=0x%04x [sp] 0x%04x (0x%04x)", cpu->pc,
		a16, *(uint16_t *)(cpu->rom + cpu->sp),
		*(uint16_t *)(&cpu->rom[cpu->sp]));

	cpu->pc = a16;
	return cpu->instr->dots[0];
}

uint8_t op_ld_$a16_A(struct CPU *cpu) //0xea
{
	int16_t a16 = ld_le16(cpu->rom + cpu->pc);
	cpu->pc += 2;
	cpu->rom[a16] = cpu->a;
	return cpu->instr->dots[0];
}

uint8_t op_di(struct CPU *cpu) //0xf3
{
	cpu->ime = false;
	return cpu->instr->dots[0];
}

uint8_t op_ld_A_$a16(struct CPU *cpu) //0xfa
{
	int16_t a16 = ld_le16(cpu->rom + cpu->pc);
	cpu->pc += 2;
	fprintf(stderr, "-> a 0x%02x ", cpu->a);
	cpu->a = cpu->rom[a16];
	fprintf(stderr, "-> 0x%02x ", cpu->a);
	return cpu->instr->dots[0];
}

uint8_t op_cp_A_n8(struct CPU *cpu) //0xfe
{
	uint8_t n8 = cpu->rom[cpu->pc++];
	cpu->fZ = cpu->a == n8;
	cpu->fN = 1;
	cpu->fH = (cpu->a & 0x0f) < (n8 & 0x0f); //lsb a lt lsb n8
	cpu->fC = cpu->a < n8;
	fprintf(stderr, "-> a=0x%02x n8=0x%02x F=0b%08b ", cpu->a, n8, cpu->f);
	return cpu->instr->dots[0];
}

static inline void set_u3_$HL(uint8_t *dest, uint8_t n)
{
	fprintf(stderr, "[HL] 0b%08b ", *dest);
	*dest |= (1U << n);
	fprintf(stderr, "0b%08b ", *dest);
}

uint8_t cb_set_0_$HL(struct CPU *cpu) //0xc6
{
	set_u3_$HL(&cpu->rom[cpu->hl], 0);
	return cpu->instr->dots[0];
}

uint8_t cb_set_1_$HL(struct CPU *cpu) //0xce
{
	set_u3_$HL(&cpu->rom[cpu->hl], 1);
	return cpu->instr->dots[0];
}

uint8_t cb_set_2_$HL(struct CPU *cpu) //0xd6
{
	set_u3_$HL(&cpu->rom[cpu->hl], 2);
	return cpu->instr->dots[0];
}

uint8_t cb_set_3_$HL(struct CPU *cpu) //0xde
{
	set_u3_$HL(&cpu->rom[cpu->hl], 3);
	return cpu->instr->dots[0];
}

uint8_t cb_set_4_$HL(struct CPU *cpu) //0xe6
{
	set_u3_$HL(&cpu->rom[cpu->hl], 4);
	return cpu->instr->dots[0];
}

uint8_t cb_set_5_$HL(struct CPU *cpu) //0xee
{
	set_u3_$HL(&cpu->rom[cpu->hl], 5);
	return cpu->instr->dots[0];
}

uint8_t cb_set_6_$HL(struct CPU *cpu) //0xf6
{
	set_u3_$HL(&cpu->rom[cpu->hl], 6);
	return cpu->instr->dots[0];
}

uint8_t cb_set_7_$HL(struct CPU *cpu) //0xfe
{
	set_u3_$HL(&cpu->rom[cpu->hl], 7);
	return cpu->instr->dots[0];
}

static inline void bit_u3_$HL(struct CPU *cpu, uint8_t n)
{
	fprintf(stderr, "f 0x%02x ", cpu->f);
	uint8_t $hl = cpu->rom[cpu->hl];
	cpu->fZ = !($hl & (1 << n));
	cpu->fN = 0;
	cpu->fH = 1;
	fprintf(stderr, "[HL] 0b%08b f 0x%02x", $hl, cpu->f);
}

uint8_t cb_bit_0_$HL(struct CPU *cpu) //0x46
{
	bit_u3_$HL(cpu, 0);
	return cpu->instr->dots[0];
}

uint8_t cb_bit_1_$HL(struct CPU *cpu) //0x4e
{
	bit_u3_$HL(cpu, 1);
	return cpu->instr->dots[0];
}

uint8_t cb_bit_2_$HL(struct CPU *cpu) //0x56
{
	bit_u3_$HL(cpu, 2);
	return cpu->instr->dots[0];
}

uint8_t cb_bit_3_$HL(struct CPU *cpu) //0x5e
{
	bit_u3_$HL(cpu, 3);
	return cpu->instr->dots[0];
}

uint8_t cb_bit_4_$HL(struct CPU *cpu) //0x66
{
	bit_u3_$HL(cpu, 4);
	return cpu->instr->dots[0];
}

uint8_t cb_bit_5_$HL(struct CPU *cpu) //0x6e
{
	bit_u3_$HL(cpu, 5);
	return cpu->instr->dots[0];
}

uint8_t cb_bit_6_$HL(struct CPU *cpu) //0x76
{
	bit_u3_$HL(cpu, 6);
	return cpu->instr->dots[0];
}

uint8_t cb_bit_7_$HL(struct CPU *cpu) //0x7e
{
	bit_u3_$HL(cpu, 7);
	return cpu->instr->dots[0];
}

struct instr cbtbl[256] = {
	[0x00] = { "*** cb RLC B 2,8 Z00C", NULL, 2, { 8, 8 }, 0x00 },
	[0x01] = { "*** cb RLC C 2,8 Z00C", NULL, 2, { 8, 8 }, 0x01 },
	[0x02] = { "*** cb RLC D 2,8 Z00C", NULL, 2, { 8, 8 }, 0x02 },
	[0x03] = { "*** cb RLC E 2,8 Z00C", NULL, 2, { 8, 8 }, 0x03 },
	[0x04] = { "*** cb RLC H 2,8 Z00C", NULL, 2, { 8, 8 }, 0x04 },
	[0x05] = { "*** cb RLC L 2,8 Z00C", NULL, 2, { 8, 8 }, 0x05 },
	[0x06] = { "*** cb RLC [HL] 2,16 Z00C", NULL, 2, { 16, 16 }, 0x06 },
	[0x07] = { "*** cb RLC A 2,8 Z00C", NULL, 2, { 8, 8 }, 0x07 },
	[0x08] = { "*** cb RRC B 2,8 Z00C", NULL, 2, { 8, 8 }, 0x08 },
	[0x09] = { "*** cb RRC C 2,8 Z00C", NULL, 2, { 8, 8 }, 0x09 },
	[0x0A] = { "*** cb RRC D 2,8 Z00C", NULL, 2, { 8, 8 }, 0x0A },
	[0x0B] = { "*** cb RRC E 2,8 Z00C", NULL, 2, { 8, 8 }, 0x0B },
	[0x0C] = { "*** cb RRC H 2,8 Z00C", NULL, 2, { 8, 8 }, 0x0C },
	[0x0D] = { "*** cb RRC L 2,8 Z00C", NULL, 2, { 8, 8 }, 0x0D },
	[0x0E] = { "*** cb RRC [HL] 2,16 Z00C", NULL, 2, { 16, 16 }, 0x0E },
	[0x0F] = { "*** cb RRC A 2,8 Z00C", NULL, 2, { 8, 8 }, 0x0F },
	[0x10] = { "*** cb RL B 2,8 Z00C", NULL, 2, { 8, 8 }, 0x10 },
	[0x11] = { "*** cb RL C 2,8 Z00C", NULL, 2, { 8, 8 }, 0x11 },
	[0x12] = { "*** cb RL D 2,8 Z00C", NULL, 2, { 8, 8 }, 0x12 },
	[0x13] = { "*** cb RL E 2,8 Z00C", NULL, 2, { 8, 8 }, 0x13 },
	[0x14] = { "*** cb RL H 2,8 Z00C", NULL, 2, { 8, 8 }, 0x14 },
	[0x15] = { "*** cb RL L 2,8 Z00C", NULL, 2, { 8, 8 }, 0x15 },
	[0x16] = { "*** cb RL [HL] 2,16 Z00C", NULL, 2, { 16, 16 }, 0x16 },
	[0x17] = { "*** cb RL A 2,8 Z00C", NULL, 2, { 8, 8 }, 0x17 },
	[0x18] = { "*** cb RR B 2,8 Z00C", NULL, 2, { 8, 8 }, 0x18 },
	[0x19] = { "*** cb RR C 2,8 Z00C", NULL, 2, { 8, 8 }, 0x19 },
	[0x1A] = { "*** cb RR D 2,8 Z00C", NULL, 2, { 8, 8 }, 0x1A },
	[0x1B] = { "*** cb RR E 2,8 Z00C", NULL, 2, { 8, 8 }, 0x1B },
	[0x1C] = { "*** cb RR H 2,8 Z00C", NULL, 2, { 8, 8 }, 0x1C },
	[0x1D] = { "*** cb RR L 2,8 Z00C", NULL, 2, { 8, 8 }, 0x1D },
	[0x1E] = { "*** cb RR [HL] 2,16 Z00C", NULL, 2, { 16, 16 }, 0x1E },
	[0x1F] = { "*** cb RR A 2,8 Z00C", NULL, 2, { 8, 8 }, 0x1F },
	[0x20] = { "*** cb SLA B 2,8 Z00C", NULL, 2, { 8, 8 }, 0x20 },
	[0x21] = { "*** cb SLA C 2,8 Z00C", NULL, 2, { 8, 8 }, 0x21 },
	[0x22] = { "*** cb SLA D 2,8 Z00C", NULL, 2, { 8, 8 }, 0x22 },
	[0x23] = { "*** cb SLA E 2,8 Z00C", NULL, 2, { 8, 8 }, 0x23 },
	[0x24] = { "*** cb SLA H 2,8 Z00C", NULL, 2, { 8, 8 }, 0x24 },
	[0x25] = { "*** cb SLA L 2,8 Z00C", NULL, 2, { 8, 8 }, 0x25 },
	[0x26] = { "*** cb SLA [HL] 2,16 Z00C", NULL, 2, { 16, 16 }, 0x26 },
	[0x27] = { "*** cb SLA A 2,8 Z00C", NULL, 2, { 8, 8 }, 0x27 },
	[0x28] = { "*** cb SRA B 2,8 Z00C", NULL, 2, { 8, 8 }, 0x28 },
	[0x29] = { "*** cb SRA C 2,8 Z00C", NULL, 2, { 8, 8 }, 0x29 },
	[0x2A] = { "*** cb SRA D 2,8 Z00C", NULL, 2, { 8, 8 }, 0x2A },
	[0x2B] = { "*** cb SRA E 2,8 Z00C", NULL, 2, { 8, 8 }, 0x2B },
	[0x2C] = { "*** cb SRA H 2,8 Z00C", NULL, 2, { 8, 8 }, 0x2C },
	[0x2D] = { "*** cb SRA L 2,8 Z00C", NULL, 2, { 8, 8 }, 0x2D },
	[0x2E] = { "*** cb SRA [HL] 2,16 Z00C", NULL, 2, { 16, 16 }, 0x2E },
	[0x2F] = { "*** cb SRA A 2,8 Z00C", NULL, 2, { 8, 8 }, 0x2F },
	[0x30] = { "*** cb SWAP B 2,8 Z000", NULL, 2, { 8, 8 }, 0x30 },
	[0x31] = { "*** cb SWAP C 2,8 Z000", NULL, 2, { 8, 8 }, 0x31 },
	[0x32] = { "*** cb SWAP D 2,8 Z000", NULL, 2, { 8, 8 }, 0x32 },
	[0x33] = { "*** cb SWAP E 2,8 Z000", NULL, 2, { 8, 8 }, 0x33 },
	[0x34] = { "*** cb SWAP H 2,8 Z000", NULL, 2, { 8, 8 }, 0x34 },
	[0x35] = { "*** cb SWAP L 2,8 Z000", NULL, 2, { 8, 8 }, 0x35 },
	[0x36] = { "*** cb SWAP [HL] 2,16 Z000", NULL, 2, { 16, 16 }, 0x36 },
	[0x37] = { "*** cb SWAP A 2,8 Z000", NULL, 2, { 8, 8 }, 0x37 },
	[0x38] = { "*** cb SRL B 2,8 Z00C", NULL, 2, { 8, 8 }, 0x38 },
	[0x39] = { "*** cb SRL C 2,8 Z00C", NULL, 2, { 8, 8 }, 0x39 },
	[0x3A] = { "*** cb SRL D 2,8 Z00C", NULL, 2, { 8, 8 }, 0x3A },
	[0x3B] = { "*** cb SRL E 2,8 Z00C", NULL, 2, { 8, 8 }, 0x3B },
	[0x3C] = { "*** cb SRL H 2,8 Z00C", NULL, 2, { 8, 8 }, 0x3C },
	[0x3D] = { "*** cb SRL L 2,8 Z00C", NULL, 2, { 8, 8 }, 0x3D },
	[0x3E] = { "*** cb SRL [HL] 2,16 Z00C", NULL, 2, { 16, 16 }, 0x3E },
	[0x3F] = { "*** cb SRL A 2,8 Z00C", NULL, 2, { 8, 8 }, 0x3F },
	[0x40] = { "*** cb BIT 0,B 2,8 Z01-", NULL, 2, { 8, 8 }, 0x40 },
	[0x41] = { "*** cb BIT 0,C 2,8 Z01-", NULL, 2, { 8, 8 }, 0x41 },
	[0x42] = { "*** cb BIT 0,D 2,8 Z01-", NULL, 2, { 8, 8 }, 0x42 },
	[0x43] = { "*** cb BIT 0,E 2,8 Z01-", NULL, 2, { 8, 8 }, 0x43 },
	[0x44] = { "*** cb BIT 0,H 2,8 Z01-", NULL, 2, { 8, 8 }, 0x44 },
	[0x45] = { "*** cb BIT 0,L 2,8 Z01-", NULL, 2, { 8, 8 }, 0x45 },
	[0x46] = { "*** cb BIT 0,[HL] 2,12 Z01-", NULL, 2, { 12, 12 }, 0x46 },
	[0x47] = { "*** cb BIT 0,A 2,8 Z01-", NULL, 2, { 8, 8 }, 0x47 },
	[0x48] = { "*** cb BIT 1,B 2,8 Z01-", NULL, 2, { 8, 8 }, 0x48 },
	[0x49] = { "*** cb BIT 1,C 2,8 Z01-", NULL, 2, { 8, 8 }, 0x49 },
	[0x4A] = { "*** cb BIT 1,D 2,8 Z01-", NULL, 2, { 8, 8 }, 0x4A },
	[0x4B] = { "*** cb BIT 1,E 2,8 Z01-", NULL, 2, { 8, 8 }, 0x4B },
	[0x4C] = { "*** cb BIT 1,H 2,8 Z01-", NULL, 2, { 8, 8 }, 0x4C },
	[0x4D] = { "*** cb BIT 1,L 2,8 Z01-", NULL, 2, { 8, 8 }, 0x4D },
	[0x4E] = { "*** cb BIT 1,[HL] 2,12 Z01-", NULL, 2, { 12, 12 }, 0x4E },
	[0x4F] = { "*** cb BIT 1,A 2,8 Z01-", NULL, 2, { 8, 8 }, 0x4F },
	[0x50] = { "*** cb BIT 2,B 2,8 Z01-", NULL, 2, { 8, 8 }, 0x50 },
	[0x51] = { "*** cb BIT 2,C 2,8 Z01-", NULL, 2, { 8, 8 }, 0x51 },
	[0x52] = { "*** cb BIT 2,D 2,8 Z01-", NULL, 2, { 8, 8 }, 0x52 },
	[0x53] = { "*** cb BIT 2,E 2,8 Z01-", NULL, 2, { 8, 8 }, 0x53 },
	[0x54] = { "*** cb BIT 2,H 2,8 Z01-", NULL, 2, { 8, 8 }, 0x54 },
	[0x55] = { "*** cb BIT 2,L 2,8 Z01-", NULL, 2, { 8, 8 }, 0x55 },
	[0x56] = { "*** cb BIT 2,[HL] 2,12 Z01-", NULL, 2, { 12, 12 }, 0x56 },
	[0x57] = { "*** cb BIT 2,A 2,8 Z01-", NULL, 2, { 8, 8 }, 0x57 },
	[0x58] = { "*** cb BIT 3,B 2,8 Z01-", NULL, 2, { 8, 8 }, 0x58 },
	[0x59] = { "*** cb BIT 3,C 2,8 Z01-", NULL, 2, { 8, 8 }, 0x59 },
	[0x5A] = { "*** cb BIT 3,D 2,8 Z01-", NULL, 2, { 8, 8 }, 0x5A },
	[0x5B] = { "*** cb BIT 3,E 2,8 Z01-", NULL, 2, { 8, 8 }, 0x5B },
	[0x5C] = { "*** cb BIT 3,H 2,8 Z01-", NULL, 2, { 8, 8 }, 0x5C },
	[0x5D] = { "*** cb BIT 3,L 2,8 Z01-", NULL, 2, { 8, 8 }, 0x5D },
	[0x5E] = { "*** cb BIT 3,[HL] 2,12 Z01-", NULL, 2, { 12, 12 }, 0x5E },
	[0x5F] = { "*** cb BIT 3,A 2,8 Z01-", NULL, 2, { 8, 8 }, 0x5F },
	[0x60] = { "*** cb BIT 4,B 2,8 Z01-", NULL, 2, { 8, 8 }, 0x60 },
	[0x61] = { "*** cb BIT 4,C 2,8 Z01-", NULL, 2, { 8, 8 }, 0x61 },
	[0x62] = { "*** cb BIT 4,D 2,8 Z01-", NULL, 2, { 8, 8 }, 0x62 },
	[0x63] = { "*** cb BIT 4,E 2,8 Z01-", NULL, 2, { 8, 8 }, 0x63 },
	[0x64] = { "*** cb BIT 4,H 2,8 Z01-", NULL, 2, { 8, 8 }, 0x64 },
	[0x65] = { "*** cb BIT 4,L 2,8 Z01-", NULL, 2, { 8, 8 }, 0x65 },
	[0x66] = { "*** cb BIT 4,[HL] 2,12 Z01-", NULL, 2, { 12, 12 }, 0x66 },
	[0x67] = { "*** cb BIT 4,A 2,8 Z01-", NULL, 2, { 8, 8 }, 0x67 },
	[0x68] = { "*** cb BIT 5,B 2,8 Z01-", NULL, 2, { 8, 8 }, 0x68 },
	[0x69] = { "*** cb BIT 5,C 2,8 Z01-", NULL, 2, { 8, 8 }, 0x69 },
	[0x6A] = { "*** cb BIT 5,D 2,8 Z01-", NULL, 2, { 8, 8 }, 0x6A },
	[0x6B] = { "*** cb BIT 5,E 2,8 Z01-", NULL, 2, { 8, 8 }, 0x6B },
	[0x6C] = { "*** cb BIT 5,H 2,8 Z01-", NULL, 2, { 8, 8 }, 0x6C },
	[0x6D] = { "*** cb BIT 5,L 2,8 Z01-", NULL, 2, { 8, 8 }, 0x6D },
	[0x6E] = { "*** cb BIT 5,[HL] 2,12 Z01-", NULL, 2, { 12, 12 }, 0x6E },
	[0x6F] = { "*** cb BIT 5,A 2,8 Z01-", NULL, 2, { 8, 8 }, 0x6F },
	[0x70] = { "*** cb BIT 6,B 2,8 Z01-", NULL, 2, { 8, 8 }, 0x70 },
	[0x71] = { "*** cb BIT 6,C 2,8 Z01-", NULL, 2, { 8, 8 }, 0x71 },
	[0x72] = { "*** cb BIT 6,D 2,8 Z01-", NULL, 2, { 8, 8 }, 0x72 },
	[0x73] = { "*** cb BIT 6,E 2,8 Z01-", NULL, 2, { 8, 8 }, 0x73 },
	[0x74] = { "*** cb BIT 6,H 2,8 Z01-", NULL, 2, { 8, 8 }, 0x74 },
	[0x75] = { "*** cb BIT 6,L 2,8 Z01-", NULL, 2, { 8, 8 }, 0x75 },
	[0x76] = { "*** cb BIT 6,[HL] 2,12 Z01-", NULL, 2, { 12, 12 }, 0x76 },
	[0x77] = { "*** cb BIT 6,A 2,8 Z01-", NULL, 2, { 8, 8 }, 0x77 },
	[0x78] = { "*** cb BIT 7,B 2,8 Z01-", NULL, 2, { 8, 8 }, 0x78 },
	[0x79] = { "*** cb BIT 7,C 2,8 Z01-", NULL, 2, { 8, 8 }, 0x79 },
	[0x7A] = { "*** cb BIT 7,D 2,8 Z01-", NULL, 2, { 8, 8 }, 0x7A },
	[0x7B] = { "*** cb BIT 7,E 2,8 Z01-", NULL, 2, { 8, 8 }, 0x7B },
	[0x7C] = { "*** cb BIT 7,H 2,8 Z01-", NULL, 2, { 8, 8 }, 0x7C },
	[0x7D] = { "*** cb BIT 7,L 2,8 Z01-", NULL, 2, { 8, 8 }, 0x7D },
	[0x7E] = { "*** cb BIT 7,[HL] 2,12 Z01-",
		   cb_bit_7_$HL,
		   2,
		   { 12, 12 },
		   0x7E },
	[0x7F] = { "*** cb BIT 7,A 2,8 Z01-", NULL, 2, { 8, 8 }, 0x7F },
	[0x80] = { "*** cb RES 0,B 2,8", NULL, 2, { 8, 8 }, 0x80 },
	[0x81] = { "*** cb RES 0,C 2,8", NULL, 2, { 8, 8 }, 0x81 },
	[0x82] = { "*** cb RES 0,D 2,8", NULL, 2, { 8, 8 }, 0x82 },
	[0x83] = { "*** cb RES 0,E 2,8", NULL, 2, { 8, 8 }, 0x83 },
	[0x84] = { "*** cb RES 0,H 2,8", NULL, 2, { 8, 8 }, 0x84 },
	[0x85] = { "*** cb RES 0,L 2,8", NULL, 2, { 8, 8 }, 0x85 },
	[0x86] = { "*** cb RES 0,[HL] 2,16", NULL, 2, { 16, 16 }, 0x86 },
	[0x87] = { "*** cb RES 0,A 2,8", NULL, 2, { 8, 8 }, 0x87 },
	[0x88] = { "*** cb RES 1,B 2,8", NULL, 2, { 8, 8 }, 0x88 },
	[0x89] = { "*** cb RES 1,C 2,8", NULL, 2, { 8, 8 }, 0x89 },
	[0x8A] = { "*** cb RES 1,D 2,8", NULL, 2, { 8, 8 }, 0x8A },
	[0x8B] = { "*** cb RES 1,E 2,8", NULL, 2, { 8, 8 }, 0x8B },
	[0x8C] = { "*** cb RES 1,H 2,8", NULL, 2, { 8, 8 }, 0x8C },
	[0x8D] = { "*** cb RES 1,L 2,8", NULL, 2, { 8, 8 }, 0x8D },
	[0x8E] = { "*** cb RES 1,[HL] 2,16", NULL, 2, { 16, 16 }, 0x8E },
	[0x8F] = { "*** cb RES 1,A 2,8", NULL, 2, { 8, 8 }, 0x8F },
	[0x90] = { "*** cb RES 2,B 2,8", NULL, 2, { 8, 8 }, 0x90 },
	[0x91] = { "*** cb RES 2,C 2,8", NULL, 2, { 8, 8 }, 0x91 },
	[0x92] = { "*** cb RES 2,D 2,8", NULL, 2, { 8, 8 }, 0x92 },
	[0x93] = { "*** cb RES 2,E 2,8", NULL, 2, { 8, 8 }, 0x93 },
	[0x94] = { "*** cb RES 2,H 2,8", NULL, 2, { 8, 8 }, 0x94 },
	[0x95] = { "*** cb RES 2,L 2,8", NULL, 2, { 8, 8 }, 0x95 },
	[0x96] = { "*** cb RES 2,[HL] 2,16", NULL, 2, { 16, 16 }, 0x96 },
	[0x97] = { "*** cb RES 2,A 2,8", NULL, 2, { 8, 8 }, 0x97 },
	[0x98] = { "*** cb RES 3,B 2,8", NULL, 2, { 8, 8 }, 0x98 },
	[0x99] = { "*** cb RES 3,C 2,8", NULL, 2, { 8, 8 }, 0x99 },
	[0x9A] = { "*** cb RES 3,D 2,8", NULL, 2, { 8, 8 }, 0x9A },
	[0x9B] = { "*** cb RES 3,E 2,8", NULL, 2, { 8, 8 }, 0x9B },
	[0x9C] = { "*** cb RES 3,H 2,8", NULL, 2, { 8, 8 }, 0x9C },
	[0x9D] = { "*** cb RES 3,L 2,8", NULL, 2, { 8, 8 }, 0x9D },
	[0x9E] = { "*** cb RES 3,[HL] 2,16", NULL, 2, { 16, 16 }, 0x9E },
	[0x9F] = { "*** cb RES 3,A 2,8", NULL, 2, { 8, 8 }, 0x9F },
	[0xA0] = { "*** cb RES 4,B 2,8", NULL, 2, { 8, 8 }, 0xA0 },
	[0xA1] = { "*** cb RES 4,C 2,8", NULL, 2, { 8, 8 }, 0xA1 },
	[0xA2] = { "*** cb RES 4,D 2,8", NULL, 2, { 8, 8 }, 0xA2 },
	[0xA3] = { "*** cb RES 4,E 2,8", NULL, 2, { 8, 8 }, 0xA3 },
	[0xA4] = { "*** cb RES 4,H 2,8", NULL, 2, { 8, 8 }, 0xA4 },
	[0xA5] = { "*** cb RES 4,L 2,8", NULL, 2, { 8, 8 }, 0xA5 },
	[0xA6] = { "*** cb RES 4,[HL] 2,16", NULL, 2, { 16, 16 }, 0xA6 },
	[0xA7] = { "*** cb RES 4,A 2,8", NULL, 2, { 8, 8 }, 0xA7 },
	[0xA8] = { "*** cb RES 5,B 2,8", NULL, 2, { 8, 8 }, 0xA8 },
	[0xA9] = { "*** cb RES 5,C 2,8", NULL, 2, { 8, 8 }, 0xA9 },
	[0xAA] = { "*** cb RES 5,D 2,8", NULL, 2, { 8, 8 }, 0xAA },
	[0xAB] = { "*** cb RES 5,E 2,8", NULL, 2, { 8, 8 }, 0xAB },
	[0xAC] = { "*** cb RES 5,H 2,8", NULL, 2, { 8, 8 }, 0xAC },
	[0xAD] = { "*** cb RES 5,L 2,8", NULL, 2, { 8, 8 }, 0xAD },
	[0xAE] = { "*** cb RES 5,[HL] 2,16", NULL, 2, { 16, 16 }, 0xAE },
	[0xAF] = { "*** cb RES 5,A 2,8", NULL, 2, { 8, 8 }, 0xAF },
	[0xB0] = { "*** cb RES 6,B 2,8", NULL, 2, { 8, 8 }, 0xB0 },
	[0xB1] = { "*** cb RES 6,C 2,8", NULL, 2, { 8, 8 }, 0xB1 },
	[0xB2] = { "*** cb RES 6,D 2,8", NULL, 2, { 8, 8 }, 0xB2 },
	[0xB3] = { "*** cb RES 6,E 2,8", NULL, 2, { 8, 8 }, 0xB3 },
	[0xB4] = { "*** cb RES 6,H 2,8", NULL, 2, { 8, 8 }, 0xB4 },
	[0xB5] = { "*** cb RES 6,L 2,8", NULL, 2, { 8, 8 }, 0xB5 },
	[0xB6] = { "*** cb RES 6,[HL] 2,16", NULL, 2, { 16, 16 }, 0xB6 },
	[0xB7] = { "*** cb RES 6,A 2,8", NULL, 2, { 8, 8 }, 0xB7 },
	[0xB8] = { "*** cb RES 7,B 2,8", NULL, 2, { 8, 8 }, 0xB8 },
	[0xB9] = { "*** cb RES 7,C 2,8", NULL, 2, { 8, 8 }, 0xB9 },
	[0xBA] = { "*** cb RES 7,D 2,8", NULL, 2, { 8, 8 }, 0xBA },
	[0xBB] = { "*** cb RES 7,E 2,8", NULL, 2, { 8, 8 }, 0xBB },
	[0xBC] = { "*** cb RES 7,H 2,8", NULL, 2, { 8, 8 }, 0xBC },
	[0xBD] = { "*** cb RES 7,L 2,8", NULL, 2, { 8, 8 }, 0xBD },
	[0xBE] = { "*** cb RES 7,[HL] 2,16", NULL, 2, { 16, 16 }, 0xBE },
	[0xBF] = { "*** cb RES 7,A 2,8", NULL, 2, { 8, 8 }, 0xBF },
	[0xC0] = { "*** cb SET 0,B 2,8", NULL, 2, { 8, 8 }, 0xC0 },
	[0xC1] = { "*** cb SET 0,C 2,8", NULL, 2, { 8, 8 }, 0xC1 },
	[0xC2] = { "*** cb SET 0,D 2,8", NULL, 2, { 8, 8 }, 0xC2 },
	[0xC3] = { "*** cb SET 0,E 2,8", NULL, 2, { 8, 8 }, 0xC3 },
	[0xC4] = { "*** cb SET 0,H 2,8", NULL, 2, { 8, 8 }, 0xC4 },
	[0xC5] = { "*** cb SET 0,L 2,8", NULL, 2, { 8, 8 }, 0xC5 },
	[0xC6] = { "*** cb SET 0,[HL] 2,16", cb_set_0_$HL, 2, { 16, 16 }, 0xC6 },
	[0xC7] = { "*** cb SET 0,A 2,8", NULL, 2, { 8, 8 }, 0xC7 },
	[0xC8] = { "*** cb SET 1,B 2,8", NULL, 2, { 8, 8 }, 0xC8 },
	[0xC9] = { "*** cb SET 1,C 2,8", NULL, 2, { 8, 8 }, 0xC9 },
	[0xCA] = { "*** cb SET 1,D 2,8", NULL, 2, { 8, 8 }, 0xCA },
	[0xCB] = { "*** cb SET 1,E 2,8", NULL, 2, { 8, 8 }, 0xCB },
	[0xCC] = { "*** cb SET 1,H 2,8", NULL, 2, { 8, 8 }, 0xCC },
	[0xCD] = { "*** cb SET 1,L 2,8", NULL, 2, { 8, 8 }, 0xCD },
	[0xCE] = { "*** cb SET 1,[HL] 2,16", NULL, 2, { 16, 16 }, 0xCE },
	[0xCF] = { "*** cb SET 1,A 2,8", NULL, 2, { 8, 8 }, 0xCF },
	[0xD0] = { "*** cb SET 2,B 2,8", NULL, 2, { 8, 8 }, 0xD0 },
	[0xD1] = { "*** cb SET 2,C 2,8", NULL, 2, { 8, 8 }, 0xD1 },
	[0xD2] = { "*** cb SET 2,D 2,8", NULL, 2, { 8, 8 }, 0xD2 },
	[0xD3] = { "*** cb SET 2,E 2,8", NULL, 2, { 8, 8 }, 0xD3 },
	[0xD4] = { "*** cb SET 2,H 2,8", NULL, 2, { 8, 8 }, 0xD4 },
	[0xD5] = { "*** cb SET 2,L 2,8", NULL, 2, { 8, 8 }, 0xD5 },
	[0xD6] = { "*** cb SET 2,[HL] 2,16", NULL, 2, { 16, 16 }, 0xD6 },
	[0xD7] = { "*** cb SET 2,A 2,8", NULL, 2, { 8, 8 }, 0xD7 },
	[0xD8] = { "*** cb SET 3,B 2,8", NULL, 2, { 8, 8 }, 0xD8 },
	[0xD9] = { "*** cb SET 3,C 2,8", NULL, 2, { 8, 8 }, 0xD9 },
	[0xDA] = { "*** cb SET 3,D 2,8", NULL, 2, { 8, 8 }, 0xDA },
	[0xDB] = { "*** cb SET 3,E 2,8", NULL, 2, { 8, 8 }, 0xDB },
	[0xDC] = { "*** cb SET 3,H 2,8", NULL, 2, { 8, 8 }, 0xDC },
	[0xDD] = { "*** cb SET 3,L 2,8", NULL, 2, { 8, 8 }, 0xDD },
	[0xDE] = { "*** cb SET 3,[HL] 2,16", NULL, 2, { 16, 16 }, 0xDE },
	[0xDF] = { "*** cb SET 3,A 2,8", NULL, 2, { 8, 8 }, 0xDF },
	[0xE0] = { "*** cb SET 4,B 2,8", NULL, 2, { 8, 8 }, 0xE0 },
	[0xE1] = { "*** cb SET 4,C 2,8", NULL, 2, { 8, 8 }, 0xE1 },
	[0xE2] = { "*** cb SET 4,D 2,8", NULL, 2, { 8, 8 }, 0xE2 },
	[0xE3] = { "*** cb SET 4,E 2,8", NULL, 2, { 8, 8 }, 0xE3 },
	[0xE4] = { "*** cb SET 4,H 2,8", NULL, 2, { 8, 8 }, 0xE4 },
	[0xE5] = { "*** cb SET 4,L 2,8", NULL, 2, { 8, 8 }, 0xE5 },
	[0xE6] = { "*** cb SET 4,[HL] 2,16", NULL, 2, { 16, 16 }, 0xE6 },
	[0xE7] = { "*** cb SET 4,A 2,8", NULL, 2, { 8, 8 }, 0xE7 },
	[0xE8] = { "*** cb SET 5,B 2,8", NULL, 2, { 8, 8 }, 0xE8 },
	[0xE9] = { "*** cb SET 5,C 2,8", NULL, 2, { 8, 8 }, 0xE9 },
	[0xEA] = { "*** cb SET 5,D 2,8", NULL, 2, { 8, 8 }, 0xEA },
	[0xEB] = { "*** cb SET 5,E 2,8", NULL, 2, { 8, 8 }, 0xEB },
	[0xEC] = { "*** cb SET 5,H 2,8", NULL, 2, { 8, 8 }, 0xEC },
	[0xED] = { "*** cb SET 5,L 2,8", NULL, 2, { 8, 8 }, 0xED },
	[0xEE] = { "*** cb SET 5,[HL] 2,16", NULL, 2, { 16, 16 }, 0xEE },
	[0xEF] = { "*** cb SET 5,A 2,8", NULL, 2, { 8, 8 }, 0xEF },
	[0xF0] = { "*** cb SET 6,B 2,8", NULL, 2, { 8, 8 }, 0xF0 },
	[0xF1] = { "*** cb SET 6,C 2,8", NULL, 2, { 8, 8 }, 0xF1 },
	[0xF2] = { "*** cb SET 6,D 2,8", NULL, 2, { 8, 8 }, 0xF2 },
	[0xF3] = { "*** cb SET 6,E 2,8", NULL, 2, { 8, 8 }, 0xF3 },
	[0xF4] = { "*** cb SET 6,H 2,8", NULL, 2, { 8, 8 }, 0xF4 },
	[0xF5] = { "*** cb SET 6,L 2,8", NULL, 2, { 8, 8 }, 0xF5 },
	[0xF6] = { "*** cb SET 6,[HL] 2,16", NULL, 2, { 16, 16 }, 0xF6 },
	[0xF7] = { "*** cb SET 6,A 2,8", NULL, 2, { 8, 8 }, 0xF7 },
	[0xF8] = { "*** cb SET 7,B 2,8", NULL, 2, { 8, 8 }, 0xF8 },
	[0xF9] = { "*** cb SET 7,C 2,8", NULL, 2, { 8, 8 }, 0xF9 },
	[0xFA] = { "*** cb SET 7,D 2,8", NULL, 2, { 8, 8 }, 0xFA },
	[0xFB] = { "*** cb SET 7,E 2,8", NULL, 2, { 8, 8 }, 0xFB },
	[0xFC] = { "*** cb SET 7,H 2,8", NULL, 2, { 8, 8 }, 0xFC },
	[0xFD] = { "*** cb SET 7,L 2,8", NULL, 2, { 8, 8 }, 0xFD },
	[0xFE] = { "*** cb SET 7,[HL] 2,16", NULL, 2, { 16, 16 }, 0xFE },
	[0xFF] = { "*** cb SET 7,A 2,8", NULL, 2, { 8, 8 }, 0xFF },
};

/*
uint8_t op_cb(struct CPU *cpu)
{
	uint8_t cbdots = cpu->instr->dots[0];
	fprintf(stderr, "\n blach ==============\n");
	cpu->op = cpu->rom[cpu->pc++];
	cpu->instr = &cbtbl[cpu->op];

	return cbdots + cpu->instr->exec(cpu);
}
*/

struct instr optbl[256] = {
	[0x00] = { "NOP 1,4", NULL, 1, { 4, 4 }, 0x00 },
	[0x01] = { "LD BC,n16 3,12", NULL, 3, { 12, 12 }, 0x01 },
	[0x02] = { "LD [BC],A 1,8", NULL, 1, { 8, 8 }, 0x02 },
	[0x03] = { "INC BC 1,8", NULL, 1, { 8, 8 }, 0x03 },
	[0x04] = { "INC B 1,4 Z0H-", NULL, 1, { 4, 4 }, 0x04 },
	[0x05] = { "DEC B 1,4 Z1H-", NULL, 1, { 4, 4 }, 0x05 },
	[0x06] = { "LD B,n8 2,8", NULL, 2, { 8, 8 }, 0x06 },
	[0x07] = { "RLCA 1,4 000C", NULL, 1, { 4, 4 }, 0x07 },
	[0x08] = { "LD [a16],SP 3,20", NULL, 3, { 20, 20 }, 0x08 },
	[0x09] = { "ADD HL,BC 1,8 -0HC", NULL, 1, { 8, 8 }, 0x09 },
	[0x0A] = { "LD A,[BC] 1,8", NULL, 1, { 8, 8 }, 0x0A },
	[0x0B] = { "DEC BC 1,8", NULL, 1, { 8, 8 }, 0x0B },
	[0x0C] = { "INC C 1,4 Z0H-", NULL, 1, { 4, 4 }, 0x0C },
	[0x0D] = { "DEC C 1,4 Z1H-", NULL, 1, { 4, 4 }, 0x0D },
	[0x0E] = { "LD C,n8 2,8", NULL, 2, { 8, 8 }, 0x0E },
	[0x0F] = { "RRCA 1,4 000C", NULL, 1, { 4, 4 }, 0x0F },
	[0x10] = { "STOP n8 2,4", NULL, 2, { 4, 4 }, 0x10 },
	[0x11] = { "LD DE,n16 3,12", NULL, 3, { 12, 12 }, 0x11 },
	[0x12] = { "LD [DE],A 1,8", NULL, 1, { 8, 8 }, 0x12 },
	[0x13] = { "INC DE 1,8", NULL, 1, { 8, 8 }, 0x13 },
	[0x14] = { "INC D 1,4 Z0H-", NULL, 1, { 4, 4 }, 0x14 },
	[0x15] = { "DEC D 1,4 Z1H-", NULL, 1, { 4, 4 }, 0x15 },
	[0x16] = { "LD D,n8 2,8", NULL, 2, { 8, 8 }, 0x16 },
	[0x17] = { "RLA 1,4 000C", NULL, 1, { 4, 4 }, 0x17 },
	[0x18] = { "JR e8 2,12", NULL, 2, { 12, 12 }, 0x18 },
	[0x19] = { "ADD HL,DE 1,8 -0HC", NULL, 1, { 8, 8 }, 0x19 },
	[0x1A] = { "LD A,[DE] 1,8", NULL, 1, { 8, 8 }, 0x1A },
	[0x1B] = { "DEC DE 1,8", NULL, 1, { 8, 8 }, 0x1B },
	[0x1C] = { "INC E 1,4 Z0H-", NULL, 1, { 4, 4 }, 0x1C },
	[0x1D] = { "DEC E 1,4 Z1H-", NULL, 1, { 4, 4 }, 0x1D },
	[0x1E] = { "LD E,n8 2,8", NULL, 2, { 8, 8 }, 0x1E },
	[0x1F] = { "RRA 1,4 000C", NULL, 1, { 4, 4 }, 0x1F },
	[0x20] = { "JR NZ,e8 2,12/8", op_jr_NZ_e8, 2, { 12, 8 }, 0x20 },
	[0x21] = { "LD HL,n16 3,12", op_ld_HL_n16, 3, { 12, 12 }, 0x21 },
	[0x22] = { "LD [HL],A 1,8", NULL, 1, { 8, 8 }, 0x22 },
	[0x23] = { "INC HL 1,8", NULL, 1, { 8, 8 }, 0x23 },
	[0x24] = { "INC H 1,4 Z0H-", NULL, 1, { 4, 4 }, 0x24 },
	[0x25] = { "DEC H 1,4 Z1H-", NULL, 1, { 4, 4 }, 0x25 },
	[0x26] = { "LD H,n8 2,8", NULL, 2, { 8, 8 }, 0x26 },
	[0x27] = { "DAA 1,4 Z-0C", NULL, 1, { 4, 4 }, 0x27 },
	[0x28] = { "JR Z,e8 2,12/8", NULL, 2, { 12, 8 }, 0x28 },
	[0x29] = { "ADD HL,HL 1,8 -0HC", NULL, 1, { 8, 8 }, 0x29 },
	[0x2A] = { "LD A,[HL] 1,8", NULL, 1, { 8, 8 }, 0x2A },
	[0x2B] = { "DEC HL 1,8", NULL, 1, { 8, 8 }, 0x2B },
	[0x2C] = { "INC L 1,4 Z0H-", NULL, 1, { 4, 4 }, 0x2C },
	[0x2D] = { "DEC L 1,4 Z1H-", NULL, 1, { 4, 4 }, 0x2D },
	[0x2E] = { "LD L,n8 2,8", NULL, 2, { 8, 8 }, 0x2E },
	[0x2F] = { "CPL 1,4 -11-", NULL, 1, { 4, 4 }, 0x2F },
	[0x30] = { "JR NC,e8 2,12/8", NULL, 2, { 12, 8 }, 0x30 },
	[0x31] = { "LD SP,n16 3,12", op_ld_sp_n16, 3, { 12, 12 }, 0x31 },
	[0x32] = { "LD [HL],A 1,8", NULL, 1, { 8, 8 }, 0x32 },
	[0x33] = { "INC SP 1,8", NULL, 1, { 8, 8 }, 0x33 },
	[0x34] = { "INC [HL] 1,12 Z0H-", NULL, 1, { 12, 12 }, 0x34 },
	[0x35] = { "DEC [HL] 1,12 Z1H-", NULL, 1, { 12, 12 }, 0x35 },
	[0x36] = { "LD [HL],n8 2,12", NULL, 2, { 12, 12 }, 0x36 },
	[0x37] = { "SCF 1,4 -001", NULL, 1, { 4, 4 }, 0x37 },
	[0x38] = { "JR C,e8 2,12/8", NULL, 2, { 12, 8 }, 0x38 },
	[0x39] = { "ADD HL,SP 1,8 -0HC", NULL, 1, { 8, 8 }, 0x39 },
	[0x3A] = { "LD A,[HL] 1,8", NULL, 1, { 8, 8 }, 0x3A },
	[0x3B] = { "DEC SP 1,8", NULL, 1, { 8, 8 }, 0x3B },
	[0x3C] = { "INC A 1,4 Z0H-", NULL, 1, { 4, 4 }, 0x3C },
	[0x3D] = { "DEC A 1,4 Z1H-", NULL, 1, { 4, 4 }, 0x3D },
	[0x3E] = { "LD A,n8 2,8", op_ld_A_n8, 2, { 8, 8 }, 0x3E },
	[0x3F] = { "CCF 1,4 -00C", NULL, 1, { 4, 4 }, 0x3F },
	[0x40] = { "LD B,B 1,4", NULL, 1, { 4, 4 }, 0x40 },
	[0x41] = { "LD B,C 1,4", NULL, 1, { 4, 4 }, 0x41 },
	[0x42] = { "LD B,D 1,4", NULL, 1, { 4, 4 }, 0x42 },
	[0x43] = { "LD B,E 1,4", NULL, 1, { 4, 4 }, 0x43 },
	[0x44] = { "LD B,H 1,4", NULL, 1, { 4, 4 }, 0x44 },
	[0x45] = { "LD B,L 1,4", NULL, 1, { 4, 4 }, 0x45 },
	[0x46] = { "LD B,[HL] 1,8", NULL, 1, { 8, 8 }, 0x46 },
	[0x47] = { "LD B,A 1,4", NULL, 1, { 4, 4 }, 0x47 },
	[0x48] = { "LD C,B 1,4", NULL, 1, { 4, 4 }, 0x48 },
	[0x49] = { "LD C,C 1,4", NULL, 1, { 4, 4 }, 0x49 },
	[0x4A] = { "LD C,D 1,4", NULL, 1, { 4, 4 }, 0x4A },
	[0x4B] = { "LD C,E 1,4", NULL, 1, { 4, 4 }, 0x4B },
	[0x4C] = { "LD C,H 1,4", NULL, 1, { 4, 4 }, 0x4C },
	[0x4D] = { "LD C,L 1,4", NULL, 1, { 4, 4 }, 0x4D },
	[0x4E] = { "LD C,[HL] 1,8", NULL, 1, { 8, 8 }, 0x4E },
	[0x4F] = { "LD C,A 1,4", NULL, 1, { 4, 4 }, 0x4F },
	[0x50] = { "LD D,B 1,4", NULL, 1, { 4, 4 }, 0x50 },
	[0x51] = { "LD D,C 1,4", NULL, 1, { 4, 4 }, 0x51 },
	[0x52] = { "LD D,D 1,4", NULL, 1, { 4, 4 }, 0x52 },
	[0x53] = { "LD D,E 1,4", NULL, 1, { 4, 4 }, 0x53 },
	[0x54] = { "LD D,H 1,4", NULL, 1, { 4, 4 }, 0x54 },
	[0x55] = { "LD D,L 1,4", NULL, 1, { 4, 4 }, 0x55 },
	[0x56] = { "LD D,[HL] 1,8", NULL, 1, { 8, 8 }, 0x56 },
	[0x57] = { "LD D,A 1,4", NULL, 1, { 4, 4 }, 0x57 },
	[0x58] = { "LD E,B 1,4", NULL, 1, { 4, 4 }, 0x58 },
	[0x59] = { "LD E,C 1,4", NULL, 1, { 4, 4 }, 0x59 },
	[0x5A] = { "LD E,D 1,4", NULL, 1, { 4, 4 }, 0x5A },
	[0x5B] = { "LD E,E 1,4", NULL, 1, { 4, 4 }, 0x5B },
	[0x5C] = { "LD E,H 1,4", NULL, 1, { 4, 4 }, 0x5C },
	[0x5D] = { "LD E,L 1,4", NULL, 1, { 4, 4 }, 0x5D },
	[0x5E] = { "LD E,[HL] 1,8", NULL, 1, { 8, 8 }, 0x5E },
	[0x5F] = { "LD E,A 1,4", NULL, 1, { 4, 4 }, 0x5F },
	[0x60] = { "LD H,B 1,4", NULL, 1, { 4, 4 }, 0x60 },
	[0x61] = { "LD H,C 1,4", NULL, 1, { 4, 4 }, 0x61 },
	[0x62] = { "LD H,D 1,4", NULL, 1, { 4, 4 }, 0x62 },
	[0x63] = { "LD H,E 1,4", NULL, 1, { 4, 4 }, 0x63 },
	[0x64] = { "LD H,H 1,4", NULL, 1, { 4, 4 }, 0x64 },
	[0x65] = { "LD H,L 1,4", NULL, 1, { 4, 4 }, 0x65 },
	[0x66] = { "LD H,[HL] 1,8", NULL, 1, { 8, 8 }, 0x66 },
	[0x67] = { "LD H,A 1,4", NULL, 1, { 4, 4 }, 0x67 },
	[0x68] = { "LD L,B 1,4", NULL, 1, { 4, 4 }, 0x68 },
	[0x69] = { "LD L,C 1,4", NULL, 1, { 4, 4 }, 0x69 },
	[0x6A] = { "LD L,D 1,4", NULL, 1, { 4, 4 }, 0x6A },
	[0x6B] = { "LD L,E 1,4", NULL, 1, { 4, 4 }, 0x6B },
	[0x6C] = { "LD L,H 1,4", NULL, 1, { 4, 4 }, 0x6C },
	[0x6D] = { "LD L,L 1,4", NULL, 1, { 4, 4 }, 0x6D },
	[0x6E] = { "LD L,[HL] 1,8", NULL, 1, { 8, 8 }, 0x6E },
	[0x6F] = { "LD L,A 1,4", NULL, 1, { 4, 4 }, 0x6F },
	[0x70] = { "LD [HL],B 1,8", NULL, 1, { 8, 8 }, 0x70 },
	[0x71] = { "LD [HL],C 1,8", NULL, 1, { 8, 8 }, 0x71 },
	[0x72] = { "LD [HL],D 1,8", NULL, 1, { 8, 8 }, 0x72 },
	[0x73] = { "LD [HL],E 1,8", NULL, 1, { 8, 8 }, 0x73 },
	[0x74] = { "LD [HL],H 1,8", NULL, 1, { 8, 8 }, 0x74 },
	[0x75] = { "LD [HL],L 1,8", NULL, 1, { 8, 8 }, 0x75 },
	[0x76] = { "HALT 1,4", NULL, 1, { 4, 4 }, 0x76 },
	[0x77] = { "LD [HL],A 1,8", NULL, 1, { 8, 8 }, 0x77 },
	[0x78] = { "LD A,B 1,4", NULL, 1, { 4, 4 }, 0x78 },
	[0x79] = { "LD A,C 1,4", NULL, 1, { 4, 4 }, 0x79 },
	[0x7A] = { "LD A,D 1,4", NULL, 1, { 4, 4 }, 0x7A },
	[0x7B] = { "LD A,E 1,4", NULL, 1, { 4, 4 }, 0x7B },
	[0x7C] = { "LD A,H 1,4", NULL, 1, { 4, 4 }, 0x7C },
	[0x7D] = { "LD A,L 1,4", NULL, 1, { 4, 4 }, 0x7D },
	[0x7E] = { "LD A,[HL] 1,8", NULL, 1, { 8, 8 }, 0x7E },
	[0x7F] = { "LD A,A 1,4", NULL, 1, { 4, 4 }, 0x7F },
	[0x80] = { "ADD A,B 1,4 Z0HC", NULL, 1, { 4, 4 }, 0x80 },
	[0x81] = { "ADD A,C 1,4 Z0HC", NULL, 1, { 4, 4 }, 0x81 },
	[0x82] = { "ADD A,D 1,4 Z0HC", NULL, 1, { 4, 4 }, 0x82 },
	[0x83] = { "ADD A,E 1,4 Z0HC", NULL, 1, { 4, 4 }, 0x83 },
	[0x84] = { "ADD A,H 1,4 Z0HC", NULL, 1, { 4, 4 }, 0x84 },
	[0x85] = { "ADD A,L 1,4 Z0HC", NULL, 1, { 4, 4 }, 0x85 },
	[0x86] = { "ADD A,[HL] 1,8 Z0HC", NULL, 1, { 8, 8 }, 0x86 },
	[0x87] = { "ADD A,A 1,4 Z0HC", NULL, 1, { 4, 4 }, 0x87 },
	[0x88] = { "ADC A,B 1,4 Z0HC", NULL, 1, { 4, 4 }, 0x88 },
	[0x89] = { "ADC A,C 1,4 Z0HC", NULL, 1, { 4, 4 }, 0x89 },
	[0x8A] = { "ADC A,D 1,4 Z0HC", NULL, 1, { 4, 4 }, 0x8A },
	[0x8B] = { "ADC A,E 1,4 Z0HC", NULL, 1, { 4, 4 }, 0x8B },
	[0x8C] = { "ADC A,H 1,4 Z0HC", NULL, 1, { 4, 4 }, 0x8C },
	[0x8D] = { "ADC A,L 1,4 Z0HC", NULL, 1, { 4, 4 }, 0x8D },
	[0x8E] = { "ADC A,[HL] 1,8 Z0HC", NULL, 1, { 8, 8 }, 0x8E },
	[0x8F] = { "ADC A,A 1,4 Z0HC", NULL, 1, { 4, 4 }, 0x8F },
	[0x90] = { "SUB A,B 1,4 Z1HC", NULL, 1, { 4, 4 }, 0x90 },
	[0x91] = { "SUB A,C 1,4 Z1HC", NULL, 1, { 4, 4 }, 0x91 },
	[0x92] = { "SUB A,D 1,4 Z1HC", NULL, 1, { 4, 4 }, 0x92 },
	[0x93] = { "SUB A,E 1,4 Z1HC", NULL, 1, { 4, 4 }, 0x93 },
	[0x94] = { "SUB A,H 1,4 Z1HC", NULL, 1, { 4, 4 }, 0x94 },
	[0x95] = { "SUB A,L 1,4 Z1HC", NULL, 1, { 4, 4 }, 0x95 },
	[0x96] = { "SUB A,[HL] 1,8 Z1HC", NULL, 1, { 8, 8 }, 0x96 },
	[0x97] = { "SUB A,A 1,4 1100", NULL, 1, { 4, 4 }, 0x97 },
	[0x98] = { "SBC A,B 1,4 Z1HC", NULL, 1, { 4, 4 }, 0x98 },
	[0x99] = { "SBC A,C 1,4 Z1HC", NULL, 1, { 4, 4 }, 0x99 },
	[0x9A] = { "SBC A,D 1,4 Z1HC", NULL, 1, { 4, 4 }, 0x9A },
	[0x9B] = { "SBC A,E 1,4 Z1HC", NULL, 1, { 4, 4 }, 0x9B },
	[0x9C] = { "SBC A,H 1,4 Z1HC", NULL, 1, { 4, 4 }, 0x9C },
	[0x9D] = { "SBC A,L 1,4 Z1HC", NULL, 1, { 4, 4 }, 0x9D },
	[0x9E] = { "SBC A,[HL] 1,8 Z1HC", NULL, 1, { 8, 8 }, 0x9E },
	[0x9F] = { "SBC A,A 1,4 Z1H-", NULL, 1, { 4, 4 }, 0x9F },
	[0xA0] = { "AND A,B 1,4 Z010", NULL, 1, { 4, 4 }, 0xA0 },
	[0xA1] = { "AND A,C 1,4 Z010", NULL, 1, { 4, 4 }, 0xA1 },
	[0xA2] = { "AND A,D 1,4 Z010", NULL, 1, { 4, 4 }, 0xA2 },
	[0xA3] = { "AND A,E 1,4 Z010", NULL, 1, { 4, 4 }, 0xA3 },
	[0xA4] = { "AND A,H 1,4 Z010", NULL, 1, { 4, 4 }, 0xA4 },
	[0xA5] = { "AND A,L 1,4 Z010", NULL, 1, { 4, 4 }, 0xA5 },
	[0xA6] = { "AND A,[HL] 1,8 Z010", NULL, 1, { 8, 8 }, 0xA6 },
	[0xA7] = { "AND A,A 1,4 Z010", NULL, 1, { 4, 4 }, 0xA7 },
	[0xA8] = { "XOR A,B 1,4 Z000", NULL, 1, { 4, 4 }, 0xA8 },
	[0xA9] = { "XOR A,C 1,4 Z000", NULL, 1, { 4, 4 }, 0xA9 },
	[0xAA] = { "XOR A,D 1,4 Z000", NULL, 1, { 4, 4 }, 0xAA },
	[0xAB] = { "XOR A,E 1,4 Z000", NULL, 1, { 4, 4 }, 0xAB },
	[0xAC] = { "XOR A,H 1,4 Z000", NULL, 1, { 4, 4 }, 0xAC },
	[0xAD] = { "XOR A,L 1,4 Z000", NULL, 1, { 4, 4 }, 0xAD },
	[0xAE] = { "XOR A,[HL] 1,8 Z000", NULL, 1, { 8, 8 }, 0xAE },
	[0xAF] = { "XOR A,A 1,4 1000", NULL, 1, { 4, 4 }, 0xAF },
	[0xB0] = { "OR A,B 1,4 Z000", NULL, 1, { 4, 4 }, 0xB0 },
	[0xB1] = { "OR A,C 1,4 Z000", NULL, 1, { 4, 4 }, 0xB1 },
	[0xB2] = { "OR A,D 1,4 Z000", NULL, 1, { 4, 4 }, 0xB2 },
	[0xB3] = { "OR A,E 1,4 Z000", NULL, 1, { 4, 4 }, 0xB3 },
	[0xB4] = { "OR A,H 1,4 Z000", NULL, 1, { 4, 4 }, 0xB4 },
	[0xB5] = { "OR A,L 1,4 Z000", NULL, 1, { 4, 4 }, 0xB5 },
	[0xB6] = { "OR A,[HL] 1,8 Z000", NULL, 1, { 8, 8 }, 0xB6 },
	[0xB7] = { "OR A,A 1,4 Z000", op_or_A_A, 1, { 4, 4 }, 0xB7 },
	[0xB8] = { "CP A,B 1,4 Z1HC", NULL, 1, { 4, 4 }, 0xB8 },
	[0xB9] = { "CP A,C 1,4 Z1HC", NULL, 1, { 4, 4 }, 0xB9 },
	[0xBA] = { "CP A,D 1,4 Z1HC", NULL, 1, { 4, 4 }, 0xBA },
	[0xBB] = { "CP A,E 1,4 Z1HC", NULL, 1, { 4, 4 }, 0xBB },
	[0xBC] = { "CP A,H 1,4 Z1HC", NULL, 1, { 4, 4 }, 0xBC },
	[0xBD] = { "CP A,L 1,4 Z1HC", NULL, 1, { 4, 4 }, 0xBD },
	[0xBE] = { "CP A,[HL] 1,8 Z1HC", NULL, 1, { 8, 8 }, 0xBE },
	[0xBF] = { "CP A,A 1,4 1100", NULL, 1, { 4, 4 }, 0xBF },
	[0xC0] = { "RET NZ 1,20/8", op_ret_NZ, 1, { 20, 8 }, 0xC0 },
	[0xC1] = { "POP BC 1,12", NULL, 1, { 12, 12 }, 0xC1 },
	[0xC2] = { "JP NZ,a16 3,16/12", NULL, 3, { 16, 12 }, 0xC2 },
	[0xC3] = { "JP a16 3,16", NULL, 3, { 16, 16 }, 0xC3 },
	[0xC4] = { "CALL NZ,a16 3,24/12", NULL, 3, { 24, 12 }, 0xC4 },
	[0xC5] = { "PUSH BC 1,16", NULL, 1, { 16, 16 }, 0xC5 },
	[0xC6] = { "ADD A,n8 2,8 Z0HC", NULL, 2, { 8, 8 }, 0xC6 },
	[0xC7] = { "RST $00 1,16", NULL, 1, { 16, 16 }, 0xC7 },
	[0xC8] = { "RET Z 1,20/8", op_ret_Z, 1, { 20, 8 }, 0xC8 },
	[0xC9] = { "RET 1,16", NULL, 1, { 16, 16 }, 0xC9 },
	[0xCA] = { "JP Z,a16 3,16/12", NULL, 3, { 16, 12 }, 0xCA },
	[0xCB] = { "PREFIX 1,4", NULL, 1, { 4, 4 }, 0xCB },
	[0xCC] = { "CALL Z,a16 3,24/12", NULL, 3, { 24, 12 }, 0xCC },
	[0xCD] = { "CALL a16 3,24", op_call_a16, 3, { 24, 24 }, 0xCD },
	[0xCE] = { "ADC A,n8 2,8 Z0HC", NULL, 2, { 8, 8 }, 0xCE },
	[0xCF] = { "RST $08 1,16", NULL, 1, { 16, 16 }, 0xCF },
	[0xD0] = { "RET NC 1,20/8", NULL, 1, { 20, 8 }, 0xD0 },
	[0xD1] = { "POP DE 1,12", NULL, 1, { 12, 12 }, 0xD1 },
	[0xD2] = { "JP NC,a16 3,16/12", NULL, 3, { 16, 12 }, 0xD2 },
	[0xD3] = { "ILLEGAL_D3 1,4", NULL, 1, { 4, 4 }, 0xD3 },
	[0xD4] = { "CALL NC,a16 3,24/12", NULL, 3, { 24, 12 }, 0xD4 },
	[0xD5] = { "PUSH DE 1,16", NULL, 1, { 16, 16 }, 0xD5 },
	[0xD6] = { "SUB A,n8 2,8 Z1HC", NULL, 2, { 8, 8 }, 0xD6 },
	[0xD7] = { "RST $10 1,16", NULL, 1, { 16, 16 }, 0xD7 },
	[0xD8] = { "RET C 1,20/8", NULL, 1, { 20, 8 }, 0xD8 },
	[0xD9] = { "RETI 1,16", NULL, 1, { 16, 16 }, 0xD9 },
	[0xDA] = { "JP C,a16 3,16/12", NULL, 3, { 16, 12 }, 0xDA },
	[0xDB] = { "ILLEGAL_DB 1,4", NULL, 1, { 4, 4 }, 0xDB },
	[0xDC] = { "CALL C,a16 3,24/12", NULL, 3, { 24, 12 }, 0xDC },
	[0xDD] = { "ILLEGAL_DD 1,4", NULL, 1, { 4, 4 }, 0xDD },
	[0xDE] = { "SBC A,n8 2,8 Z1HC", NULL, 2, { 8, 8 }, 0xDE },
	[0xDF] = { "RST $18 1,16", NULL, 1, { 16, 16 }, 0xDF },
	[0xE0] = { "LDH [a8],A 2,12", NULL, 2, { 12, 12 }, 0xE0 },
	[0xE1] = { "POP HL 1,12", NULL, 1, { 12, 12 }, 0xE1 },
	[0xE2] = { "LDH [C],A 1,8", NULL, 1, { 8, 8 }, 0xE2 },
	[0xE3] = { "ILLEGAL_E3 1,4", NULL, 1, { 4, 4 }, 0xE3 },
	[0xE4] = { "ILLEGAL_E4 1,4", NULL, 1, { 4, 4 }, 0xE4 },
	[0xE5] = { "PUSH HL 1,16", NULL, 1, { 16, 16 }, 0xE5 },
	[0xE6] = { "AND A,n8 2,8 Z010", NULL, 2, { 8, 8 }, 0xE6 },
	[0xE7] = { "RST $20 1,16", NULL, 1, { 16, 16 }, 0xE7 },
	[0xE8] = { "ADD SP,e8 2,16 00HC", NULL, 2, { 16, 16 }, 0xE8 },
	[0xE9] = { "JP HL 1,4", NULL, 1, { 4, 4 }, 0xE9 },
	[0xEA] = { "LD [a16],A 3,16", op_ld_$a16_A, 3, { 16, 16 }, 0xEA },
	[0xEB] = { "ILLEGAL_EB 1,4", NULL, 1, { 4, 4 }, 0xEB },
	[0xEC] = { "ILLEGAL_EC 1,4", NULL, 1, { 4, 4 }, 0xEC },
	[0xED] = { "ILLEGAL_ED 1,4", NULL, 1, { 4, 4 }, 0xED },
	[0xEE] = { "XOR A,n8 2,8 Z000", NULL, 2, { 8, 8 }, 0xEE },
	[0xEF] = { "RST $28 1,16", NULL, 1, { 16, 16 }, 0xEF },
	[0xF0] = { "LDH A,[a8] 2,12", NULL, 2, { 12, 12 }, 0xF0 },
	[0xF1] = { "POP AF 1,12 ZNHC", NULL, 1, { 12, 12 }, 0xF1 },
	[0xF2] = { "LDH A,[C] 1,8", NULL, 1, { 8, 8 }, 0xF2 },
	[0xF3] = { "DI 1,4", op_di, 1, { 4, 4 }, 0xF3 },
	[0xF4] = { "ILLEGAL_F4 1,4", NULL, 1, { 4, 4 }, 0xF4 },
	[0xF5] = { "PUSH AF 1,16", NULL, 1, { 16, 16 }, 0xF5 },
	[0xF6] = { "OR A,n8 2,8 Z000", NULL, 2, { 8, 8 }, 0xF6 },
	[0xF7] = { "RST $30 1,16", NULL, 1, { 16, 16 }, 0xF7 },
	[0xF8] = { "LD HL,SP,e8 2,12 00HC", NULL, 2, { 12, 12 }, 0xF8 },
	[0xF9] = { "LD SP,HL 1,8", NULL, 1, { 8, 8 }, 0xF9 },
	[0xFA] = { "LD A,[a16] 3,16", op_ld_A_$a16, 3, { 16, 16 }, 0xFA },
	[0xFB] = { "EI 1,4", NULL, 1, { 4, 4 }, 0xFB },
	[0xFC] = { "ILLEGAL_FC 1,4", NULL, 1, { 4, 4 }, 0xFC },
	[0xFD] = { "ILLEGAL_FD 1,4", NULL, 1, { 4, 4 }, 0xFD },
	[0xFE] = { "CP A,n8 2,8 Z1HC", op_cp_A_n8, 2, { 8, 8 }, 0xFE },
	[0xFF] = { "RST $38 1,16", NULL, 1, { 16, 16 }, 0xFF },
};

void initreg(struct CPU *cpu)
{
	cpu->rom[P1] = 0xcf;
	cpu->rom[SB] = 0x00;
	cpu->rom[SC] = 0x7e;
	cpu->rom[DIV] = 0x18;

	cpu->rom[TIMA] = 0;
	cpu->rom[TMA] = 0;
	cpu->rom[TAC] = 0xf8;
	cpu->rom[IF] = 0xe1;

	cpu->rom[NR10] = 0x80;
	cpu->rom[NR11] = 0xbf;
	cpu->rom[NR12] = 0xf3;
	cpu->rom[NR13] = 0xff;
	cpu->rom[NR14] = 0xbf;
	cpu->rom[NR21] = 0x3f;
	cpu->rom[NR22] = 0x00;
	cpu->rom[NR23] = 0xff;
	cpu->rom[NR24] = 0xbf;

	cpu->rom[NR30] = 0x7f;
	cpu->rom[NR31] = 0xff;
	cpu->rom[NR32] = 0x9f;
	cpu->rom[NR33] = 0xff;
	cpu->rom[NR34] = 0xbf;
	cpu->rom[NR41] = 0xff;
	cpu->rom[NR42] = 0x00;
	cpu->rom[NR43] = 0x00;
	cpu->rom[NR44] = 0xbf;
	cpu->rom[NR50] = 0x77;
	cpu->rom[NR51] = 0xf3;
	cpu->rom[NR52] = 0xf1;

	cpu->rom[LCDC] = 0x91;
	cpu->rom[STAT] = 0x81;
	cpu->rom[SCY] = 0x00;
	cpu->rom[SCX] = 0x00;
	cpu->rom[LY] = 0x91;
	cpu->rom[LYC] = 0x00;
	cpu->rom[DMA] = 0xff;
	cpu->rom[BGP] = 0xfc;
	cpu->rom[WY] = 0x00;
	cpu->rom[WX] = 0x00;
	cpu->rom[IE] = 0x00;
}

void initreg_cgb(struct CPU *cpu)
{
	// overwrite dmg values
	cpu->rom[SC] = 0x7f;
	cpu->rom[DMA] = 0;

	// exclusive cgb
	cpu->rom[KEY1] = 0x7e;
	cpu->rom[VBK] = 0xfe;
	cpu->rom[HDMA1] = 0xff;
	cpu->rom[HDMA2] = 0xff;
	cpu->rom[HDMA3] = 0xff;
	cpu->rom[HDMA4] = 0xff;
	cpu->rom[HDMA5] = 0xff;
	cpu->rom[RP] = 0x3e;
	cpu->rom[SVBK] = 0xf8;
}

void cpu_fetch(struct CPU *cpu)
{
	cpu->op = cpu->rom[cpu->pc++];
	if (cpu->op == 0xcb) {
		cpu->op = cpu->rom[cpu->pc++];
		cpu->instr = &cbtbl[cpu->op];
		cpu->prefix = true;
		return;
	}

	cpu->instr = &optbl[cpu->op];
}

void cpu_ldrom(struct CPU *cpu, char *rom)
{
	size_t sz = 2097152;
	int fd = open(rom, O_RDONLY);
	if (fd < 0) {
		perror("Could not open metal gear solid\n");
		exit(EXIT_FAILURE);
	}
	cpu->rom = malloc(sz);
	ssize_t n = read(fd, cpu->rom, sz);
	if (n < sz) {
		perror("Did not read 2097152 bytes\n");
		exit(EXIT_FAILURE);
	}

	// initial values. figure out where i got these
	// these are from gameboy cpu manual.
	cpu->af = 0x11b0;
	cpu->bc = 0x0013;
	cpu->de = 0x00d8;
	cpu->hl = 0x014d;
	cpu->sp = 0xfffe;

	// pan doc values. assume these are correct but verify against values above
	// after investigating. values above are dmg defaults.
	cpu->bc = 0x0013; //cgb dmgmode 0x0000
	cpu->de = 0xff56; //cgb dmg mode 0x0008
	cpu->hl = 0x000d; // cgb dmg ????

	cpu->ime = cpu->dblspd = cpu->prefix = false;
	cpu->pc = 0x150; // assume valid rom. (cpu->pc = 0x100)
	cpu_fetch(cpu);

	fprintf(stderr,
		"af 0x%04x (addr 0x%p), a 0x%02x (addr 0x%p), f 0x%02x (addr 0x%p)\n",
		cpu->af, &cpu->af, cpu->a, &cpu->a, cpu->f, &cpu->f);
	fprintf(stderr,
		"hl 0x%04x (addr 0x%p), h 0x%02x (addr 0x%p), l 0x%02x (addr 0x%p)\n",
		cpu->hl, &cpu->hl, cpu->h, &cpu->h, cpu->l, &cpu->l);

	initreg(cpu);
	initreg_cgb(cpu);
}

void printInstr(struct CPU *cpu)
{
	if (cpu->prefix) {
		fprintf(stderr, "0x%04x: cb %02x -- %s ", cpu->pc - 2,
			cpu->instr->op, cpu->instr->mnem);
		return;
	}
	fprintf(stderr, "0x%04x: op %02x -- %s ", cpu->pc - 1, cpu->instr->op,
		cpu->instr->mnem);
}

int cpu_exec(struct CPU *cpu)
{
	if (cpu->prefix) {
		fprintf(stderr, "\n========================================\n");
	}

	printInstr(cpu);
	if (!cpu->instr->exec) {
		fprintf(stderr, " !!! ERROR: NOT IMPLEMENTED\n\n");
		exit(1);
	}

	uint8_t result = cpu->instr->exec(cpu);
	cpu->prefix = false;
	return result;
}

int cpu_step(struct CPU *cpu)
{
	int result = cpu_exec(cpu);
	cpu_fetch(cpu);
	return result;
}
