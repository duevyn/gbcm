#include <stdlib.h>
#include <stdio.h>
#include <fcntl.h>
#include <unistd.h>
#include "CPU.h"
#include "registers.h"

struct instr optbl[256] = {
	[0x00] = { "NOP", 1, 4, NULL },
	[0x01] = { "LD BC, n16", 3, 12, NULL },
	[0x02] = { "LD [BC], A", 1, 8, NULL },
	[0x03] = { "INC BC", 1, 8, NULL },
	[0x04] = { "INC B Z0H-", 1, 4, NULL },
	[0x05] = { "DEC B Z1H-", 1, 4, NULL },
	[0x06] = { "LD B, n8", 2, 8, NULL },
	[0x07] = { "RLCA 000C", 1, 4, NULL },
	[0x08] = { "LD [a16], SP", 3, 20, NULL },
	[0x09] = { "ADD HL, BC -0HC", 1, 8, NULL },
	[0x0A] = { "LD A, [BC]", 1, 8, NULL },
	[0x0B] = { "DEC BC", 1, 8, NULL },
	[0x0C] = { "INC C Z0H-", 1, 4, NULL },
	[0x0D] = { "DEC C Z1H-", 1, 4, NULL },
	[0x0E] = { "LD C, n8", 2, 8, NULL },
	[0x0F] = { "RRCA 000C", 1, 4, NULL },
	[0x10] = { "STOP n8", 2, 4, NULL },
	[0x11] = { "LD DE, n16", 3, 12, NULL },
	[0x12] = { "LD [DE], A", 1, 8, NULL },
	[0x13] = { "INC DE", 1, 8, NULL },
	[0x14] = { "INC D Z0H-", 1, 4, NULL },
	[0x15] = { "DEC D Z1H-", 1, 4, NULL },
	[0x16] = { "LD D, n8", 2, 8, NULL },
	[0x17] = { "RLA 000C", 1, 4, NULL },
	[0x18] = { "JR e8", 2, 12, NULL },
	[0x19] = { "ADD HL, DE -0HC", 1, 8, NULL },
	[0x1A] = { "LD A, [DE]", 1, 8, NULL },
	[0x1B] = { "DEC DE", 1, 8, NULL },
	[0x1C] = { "INC E Z0H-", 1, 4, NULL },
	[0x1D] = { "DEC E Z1H-", 1, 4, NULL },
	[0x1E] = { "LD E, n8", 2, 8, NULL },
	[0x1F] = { "RRA 000C", 1, 4, NULL },
	[0x20] = { "JR NZ, e8", 2, 12, NULL },
	[0x21] = { "LD HL, n16", 3, 12, NULL },
	[0x22] = { "LD [HL], A", 1, 8, NULL },
	[0x23] = { "INC HL", 1, 8, NULL },
	[0x24] = { "INC H Z0H-", 1, 4, NULL },
	[0x25] = { "DEC H Z1H-", 1, 4, NULL },
	[0x26] = { "LD H, n8", 2, 8, NULL },
	[0x27] = { "DAA Z-0C", 1, 4, NULL },
	[0x28] = { "JR Z, e8", 2, 12, NULL },
	[0x29] = { "ADD HL, HL -0HC", 1, 8, NULL },
	[0x2A] = { "LD A, [HL]", 1, 8, NULL },
	[0x2B] = { "DEC HL", 1, 8, NULL },
	[0x2C] = { "INC L Z0H-", 1, 4, NULL },
	[0x2D] = { "DEC L Z1H-", 1, 4, NULL },
	[0x2E] = { "LD L, n8", 2, 8, NULL },
	[0x2F] = { "CPL -11-", 1, 4, NULL },
	[0x30] = { "JR NC, e8", 2, 12, NULL },
	[0x31] = { "LD SP, n16", 3, 12, NULL },
	[0x32] = { "LD [HL], A", 1, 8, NULL },
	[0x33] = { "INC SP", 1, 8, NULL },
	[0x34] = { "INC [HL] Z0H-", 1, 12, NULL },
	[0x35] = { "DEC [HL] Z1H-", 1, 12, NULL },
	[0x36] = { "LD [HL], n8", 2, 12, NULL },
	[0x37] = { "SCF -001", 1, 4, NULL },
	[0x38] = { "JR C, e8", 2, 12, NULL },
	[0x39] = { "ADD HL, SP -0HC", 1, 8, NULL },
	[0x3A] = { "LD A, [HL]", 1, 8, NULL },
	[0x3B] = { "DEC SP", 1, 8, NULL },
	[0x3C] = { "INC A Z0H-", 1, 4, NULL },
	[0x3D] = { "DEC A Z1H-", 1, 4, NULL },
	[0x3E] = { "LD A, n8", 2, 8, NULL },
	[0x3F] = { "CCF -00C", 1, 4, NULL },
	[0x40] = { "LD B, B", 1, 4, NULL },
	[0x41] = { "LD B, C", 1, 4, NULL },
	[0x42] = { "LD B, D", 1, 4, NULL },
	[0x43] = { "LD B, E", 1, 4, NULL },
	[0x44] = { "LD B, H", 1, 4, NULL },
	[0x45] = { "LD B, L", 1, 4, NULL },
	[0x46] = { "LD B, [HL]", 1, 8, NULL },
	[0x47] = { "LD B, A", 1, 4, NULL },
	[0x48] = { "LD C, B", 1, 4, NULL },
	[0x49] = { "LD C, C", 1, 4, NULL },
	[0x4A] = { "LD C, D", 1, 4, NULL },
	[0x4B] = { "LD C, E", 1, 4, NULL },
	[0x4C] = { "LD C, H", 1, 4, NULL },
	[0x4D] = { "LD C, L", 1, 4, NULL },
	[0x4E] = { "LD C, [HL]", 1, 8, NULL },
	[0x4F] = { "LD C, A", 1, 4, NULL },
	[0x50] = { "LD D, B", 1, 4, NULL },
	[0x51] = { "LD D, C", 1, 4, NULL },
	[0x52] = { "LD D, D", 1, 4, NULL },
	[0x53] = { "LD D, E", 1, 4, NULL },
	[0x54] = { "LD D, H", 1, 4, NULL },
	[0x55] = { "LD D, L", 1, 4, NULL },
	[0x56] = { "LD D, [HL]", 1, 8, NULL },
	[0x57] = { "LD D, A", 1, 4, NULL },
	[0x58] = { "LD E, B", 1, 4, NULL },
	[0x59] = { "LD E, C", 1, 4, NULL },
	[0x5A] = { "LD E, D", 1, 4, NULL },
	[0x5B] = { "LD E, E", 1, 4, NULL },
	[0x5C] = { "LD E, H", 1, 4, NULL },
	[0x5D] = { "LD E, L", 1, 4, NULL },
	[0x5E] = { "LD E, [HL]", 1, 8, NULL },
	[0x5F] = { "LD E, A", 1, 4, NULL },
	[0x60] = { "LD H, B", 1, 4, NULL },
	[0x61] = { "LD H, C", 1, 4, NULL },
	[0x62] = { "LD H, D", 1, 4, NULL },
	[0x63] = { "LD H, E", 1, 4, NULL },
	[0x64] = { "LD H, H", 1, 4, NULL },
	[0x65] = { "LD H, L", 1, 4, NULL },
	[0x66] = { "LD H, [HL]", 1, 8, NULL },
	[0x67] = { "LD H, A", 1, 4, NULL },
	[0x68] = { "LD L, B", 1, 4, NULL },
	[0x69] = { "LD L, C", 1, 4, NULL },
	[0x6A] = { "LD L, D", 1, 4, NULL },
	[0x6B] = { "LD L, E", 1, 4, NULL },
	[0x6C] = { "LD L, H", 1, 4, NULL },
	[0x6D] = { "LD L, L", 1, 4, NULL },
	[0x6E] = { "LD L, [HL]", 1, 8, NULL },
	[0x6F] = { "LD L, A", 1, 4, NULL },
	[0x70] = { "LD [HL], B", 1, 8, NULL },
	[0x71] = { "LD [HL], C", 1, 8, NULL },
	[0x72] = { "LD [HL], D", 1, 8, NULL },
	[0x73] = { "LD [HL], E", 1, 8, NULL },
	[0x74] = { "LD [HL], H", 1, 8, NULL },
	[0x75] = { "LD [HL], L", 1, 8, NULL },
	[0x76] = { "HALT", 1, 4, NULL },
	[0x77] = { "LD [HL], A", 1, 8, NULL },
	[0x78] = { "LD A, B", 1, 4, NULL },
	[0x79] = { "LD A, C", 1, 4, NULL },
	[0x7A] = { "LD A, D", 1, 4, NULL },
	[0x7B] = { "LD A, E", 1, 4, NULL },
	[0x7C] = { "LD A, H", 1, 4, NULL },
	[0x7D] = { "LD A, L", 1, 4, NULL },
	[0x7E] = { "LD A, [HL]", 1, 8, NULL },
	[0x7F] = { "LD A, A", 1, 4, NULL },
	[0x80] = { "ADD A, B Z0HC", 1, 4, NULL },
	[0x81] = { "ADD A, C Z0HC", 1, 4, NULL },
	[0x82] = { "ADD A, D Z0HC", 1, 4, NULL },
	[0x83] = { "ADD A, E Z0HC", 1, 4, NULL },
	[0x84] = { "ADD A, H Z0HC", 1, 4, NULL },
	[0x85] = { "ADD A, L Z0HC", 1, 4, NULL },
	[0x86] = { "ADD A, [HL] Z0HC", 1, 8, NULL },
	[0x87] = { "ADD A, A Z0HC", 1, 4, NULL },
	[0x88] = { "ADC A, B Z0HC", 1, 4, NULL },
	[0x89] = { "ADC A, C Z0HC", 1, 4, NULL },
	[0x8A] = { "ADC A, D Z0HC", 1, 4, NULL },
	[0x8B] = { "ADC A, E Z0HC", 1, 4, NULL },
	[0x8C] = { "ADC A, H Z0HC", 1, 4, NULL },
	[0x8D] = { "ADC A, L Z0HC", 1, 4, NULL },
	[0x8E] = { "ADC A, [HL] Z0HC", 1, 8, NULL },
	[0x8F] = { "ADC A, A Z0HC", 1, 4, NULL },
	[0x90] = { "SUB A, B Z1HC", 1, 4, NULL },
	[0x91] = { "SUB A, C Z1HC", 1, 4, NULL },
	[0x92] = { "SUB A, D Z1HC", 1, 4, NULL },
	[0x93] = { "SUB A, E Z1HC", 1, 4, NULL },
	[0x94] = { "SUB A, H Z1HC", 1, 4, NULL },
	[0x95] = { "SUB A, L Z1HC", 1, 4, NULL },
	[0x96] = { "SUB A, [HL] Z1HC", 1, 8, NULL },
	[0x97] = { "SUB A, A 1100", 1, 4, NULL },
	[0x98] = { "SBC A, B Z1HC", 1, 4, NULL },
	[0x99] = { "SBC A, C Z1HC", 1, 4, NULL },
	[0x9A] = { "SBC A, D Z1HC", 1, 4, NULL },
	[0x9B] = { "SBC A, E Z1HC", 1, 4, NULL },
	[0x9C] = { "SBC A, H Z1HC", 1, 4, NULL },
	[0x9D] = { "SBC A, L Z1HC", 1, 4, NULL },
	[0x9E] = { "SBC A, [HL] Z1HC", 1, 8, NULL },
	[0x9F] = { "SBC A, A Z1H-", 1, 4, NULL },
	[0xA0] = { "AND A, B Z010", 1, 4, NULL },
	[0xA1] = { "AND A, C Z010", 1, 4, NULL },
	[0xA2] = { "AND A, D Z010", 1, 4, NULL },
	[0xA3] = { "AND A, E Z010", 1, 4, NULL },
	[0xA4] = { "AND A, H Z010", 1, 4, NULL },
	[0xA5] = { "AND A, L Z010", 1, 4, NULL },
	[0xA6] = { "AND A, [HL] Z010", 1, 8, NULL },
	[0xA7] = { "AND A, A Z010", 1, 4, NULL },
	[0xA8] = { "XOR A, B Z000", 1, 4, NULL },
	[0xA9] = { "XOR A, C Z000", 1, 4, NULL },
	[0xAA] = { "XOR A, D Z000", 1, 4, NULL },
	[0xAB] = { "XOR A, E Z000", 1, 4, NULL },
	[0xAC] = { "XOR A, H Z000", 1, 4, NULL },
	[0xAD] = { "XOR A, L Z000", 1, 4, NULL },
	[0xAE] = { "XOR A, [HL] Z000", 1, 8, NULL },
	[0xAF] = { "XOR A, A 1000", 1, 4, NULL },
	[0xB0] = { "OR A, B Z000", 1, 4, NULL },
	[0xB1] = { "OR A, C Z000", 1, 4, NULL },
	[0xB2] = { "OR A, D Z000", 1, 4, NULL },
	[0xB3] = { "OR A, E Z000", 1, 4, NULL },
	[0xB4] = { "OR A, H Z000", 1, 4, NULL },
	[0xB5] = { "OR A, L Z000", 1, 4, NULL },
	[0xB6] = { "OR A, [HL] Z000", 1, 8, NULL },
	[0xB7] = { "OR A, A Z000", 1, 4, NULL },
	[0xB8] = { "CP A, B Z1HC", 1, 4, NULL },
	[0xB9] = { "CP A, C Z1HC", 1, 4, NULL },
	[0xBA] = { "CP A, D Z1HC", 1, 4, NULL },
	[0xBB] = { "CP A, E Z1HC", 1, 4, NULL },
	[0xBC] = { "CP A, H Z1HC", 1, 4, NULL },
	[0xBD] = { "CP A, L Z1HC", 1, 4, NULL },
	[0xBE] = { "CP A, [HL] Z1HC", 1, 8, NULL },
	[0xBF] = { "CP A, A 1100", 1, 4, NULL },
	[0xC0] = { "RET NZ", 1, 20, NULL },
	[0xC1] = { "POP BC", 1, 12, NULL },
	[0xC2] = { "JP NZ, a16", 3, 16, NULL },
	[0xC3] = { "JP a16", 3, 16, NULL },
	[0xC4] = { "CALL NZ, a16", 3, 24, NULL },
	[0xC5] = { "PUSH BC", 1, 16, NULL },
	[0xC6] = { "ADD A, n8 Z0HC", 2, 8, NULL },
	[0xC7] = { "RST $00", 1, 16, NULL },
	[0xC8] = { "RET Z", 1, 20, NULL },
	[0xC9] = { "RET", 1, 16, NULL },
	[0xCA] = { "JP Z, a16", 3, 16, NULL },
	[0xCB] = { "PREFIX", 1, 4, NULL },
	[0xCC] = { "CALL Z, a16", 3, 24, NULL },
	[0xCD] = { "CALL a16", 3, 24, NULL },
	[0xCE] = { "ADC A, n8 Z0HC", 2, 8, NULL },
	[0xCF] = { "RST $08", 1, 16, NULL },
	[0xD0] = { "RET NC", 1, 20, NULL },
	[0xD1] = { "POP DE", 1, 12, NULL },
	[0xD2] = { "JP NC, a16", 3, 16, NULL },
	[0xD3] = { "ILLEGAL_D3", 1, 4, NULL },
	[0xD4] = { "CALL NC, a16", 3, 24, NULL },
	[0xD5] = { "PUSH DE", 1, 16, NULL },
	[0xD6] = { "SUB A, n8 Z1HC", 2, 8, NULL },
	[0xD7] = { "RST $10", 1, 16, NULL },
	[0xD8] = { "RET C", 1, 20, NULL },
	[0xD9] = { "RETI", 1, 16, NULL },
	[0xDA] = { "JP C, a16", 3, 16, NULL },
	[0xDB] = { "ILLEGAL_DB", 1, 4, NULL },
	[0xDC] = { "CALL C, a16", 3, 24, NULL },
	[0xDD] = { "ILLEGAL_DD", 1, 4, NULL },
	[0xDE] = { "SBC A, n8 Z1HC", 2, 8, NULL },
	[0xDF] = { "RST $18", 1, 16, NULL },
	[0xE0] = { "LDH [a8], A", 2, 12, NULL },
	[0xE1] = { "POP HL", 1, 12, NULL },
	[0xE2] = { "LDH [C], A", 1, 8, NULL },
	[0xE3] = { "ILLEGAL_E3", 1, 4, NULL },
	[0xE4] = { "ILLEGAL_E4", 1, 4, NULL },
	[0xE5] = { "PUSH HL", 1, 16, NULL },
	[0xE6] = { "AND A, n8 Z010", 2, 8, NULL },
	[0xE7] = { "RST $20", 1, 16, NULL },
	[0xE8] = { "ADD SP, e8 00HC", 2, 16, NULL },
	[0xE9] = { "JP HL", 1, 4, NULL },
	[0xEA] = { "LD [a16], A", 3, 16, NULL },
	[0xEB] = { "ILLEGAL_EB", 1, 4, NULL },
	[0xEC] = { "ILLEGAL_EC", 1, 4, NULL },
	[0xED] = { "ILLEGAL_ED", 1, 4, NULL },
	[0xEE] = { "XOR A, n8 Z000", 2, 8, NULL },
	[0xEF] = { "RST $28", 1, 16, NULL },
	[0xF0] = { "LDH A, [a8]", 2, 12, NULL },
	[0xF1] = { "POP AF ZNHC", 1, 12, NULL },
	[0xF2] = { "LDH A, [C]", 1, 8, NULL },
	[0xF3] = { "DI", 1, 4, NULL },
	[0xF4] = { "ILLEGAL_F4", 1, 4, NULL },
	[0xF5] = { "PUSH AF", 1, 16, NULL },
	[0xF6] = { "OR A, n8 Z000", 2, 8, NULL },
	[0xF7] = { "RST $30", 1, 16, NULL },
	[0xF8] = { "LD HL, SP, e8 00HC", 2, 12, NULL },
	[0xF9] = { "LD SP, HL", 1, 8, NULL },
	[0xFA] = { "LD A, [a16]", 3, 16, NULL },
	[0xFB] = { "EI", 1, 4, NULL },
	[0xFC] = { "ILLEGAL_FC", 1, 4, NULL },
	[0xFD] = { "ILLEGAL_FD", 1, 4, NULL },
	[0xFE] = { "CP A, n8 Z1HC", 2, 8, NULL },
	[0xFF] = { "RST $38", 1, 16, NULL },
};

struct instr cbtbl[256] = {
	[0x00] = { "RLC B Z00C", 2, 8, NULL },
	[0x01] = { "RLC C Z00C", 2, 8, NULL },
	[0x02] = { "RLC D Z00C", 2, 8, NULL },
	[0x03] = { "RLC E Z00C", 2, 8, NULL },
	[0x04] = { "RLC H Z00C", 2, 8, NULL },
	[0x05] = { "RLC L Z00C", 2, 8, NULL },
	[0x06] = { "RLC [HL] Z00C", 2, 16, NULL },
	[0x07] = { "RLC A Z00C", 2, 8, NULL },
	[0x08] = { "RRC B Z00C", 2, 8, NULL },
	[0x09] = { "RRC C Z00C", 2, 8, NULL },
	[0x0A] = { "RRC D Z00C", 2, 8, NULL },
	[0x0B] = { "RRC E Z00C", 2, 8, NULL },
	[0x0C] = { "RRC H Z00C", 2, 8, NULL },
	[0x0D] = { "RRC L Z00C", 2, 8, NULL },
	[0x0E] = { "RRC [HL] Z00C", 2, 16, NULL },
	[0x0F] = { "RRC A Z00C", 2, 8, NULL },
	[0x10] = { "RL B Z00C", 2, 8, NULL },
	[0x11] = { "RL C Z00C", 2, 8, NULL },
	[0x12] = { "RL D Z00C", 2, 8, NULL },
	[0x13] = { "RL E Z00C", 2, 8, NULL },
	[0x14] = { "RL H Z00C", 2, 8, NULL },
	[0x15] = { "RL L Z00C", 2, 8, NULL },
	[0x16] = { "RL [HL] Z00C", 2, 16, NULL },
	[0x17] = { "RL A Z00C", 2, 8, NULL },
	[0x18] = { "RR B Z00C", 2, 8, NULL },
	[0x19] = { "RR C Z00C", 2, 8, NULL },
	[0x1A] = { "RR D Z00C", 2, 8, NULL },
	[0x1B] = { "RR E Z00C", 2, 8, NULL },
	[0x1C] = { "RR H Z00C", 2, 8, NULL },
	[0x1D] = { "RR L Z00C", 2, 8, NULL },
	[0x1E] = { "RR [HL] Z00C", 2, 16, NULL },
	[0x1F] = { "RR A Z00C", 2, 8, NULL },
	[0x20] = { "SLA B Z00C", 2, 8, NULL },
	[0x21] = { "SLA C Z00C", 2, 8, NULL },
	[0x22] = { "SLA D Z00C", 2, 8, NULL },
	[0x23] = { "SLA E Z00C", 2, 8, NULL },
	[0x24] = { "SLA H Z00C", 2, 8, NULL },
	[0x25] = { "SLA L Z00C", 2, 8, NULL },
	[0x26] = { "SLA [HL] Z00C", 2, 16, NULL },
	[0x27] = { "SLA A Z00C", 2, 8, NULL },
	[0x28] = { "SRA B Z00C", 2, 8, NULL },
	[0x29] = { "SRA C Z00C", 2, 8, NULL },
	[0x2A] = { "SRA D Z00C", 2, 8, NULL },
	[0x2B] = { "SRA E Z00C", 2, 8, NULL },
	[0x2C] = { "SRA H Z00C", 2, 8, NULL },
	[0x2D] = { "SRA L Z00C", 2, 8, NULL },
	[0x2E] = { "SRA [HL] Z00C", 2, 16, NULL },
	[0x2F] = { "SRA A Z00C", 2, 8, NULL },
	[0x30] = { "SWAP B Z000", 2, 8, NULL },
	[0x31] = { "SWAP C Z000", 2, 8, NULL },
	[0x32] = { "SWAP D Z000", 2, 8, NULL },
	[0x33] = { "SWAP E Z000", 2, 8, NULL },
	[0x34] = { "SWAP H Z000", 2, 8, NULL },
	[0x35] = { "SWAP L Z000", 2, 8, NULL },
	[0x36] = { "SWAP [HL] Z000", 2, 16, NULL },
	[0x37] = { "SWAP A Z000", 2, 8, NULL },
	[0x38] = { "SRL B Z00C", 2, 8, NULL },
	[0x39] = { "SRL C Z00C", 2, 8, NULL },
	[0x3A] = { "SRL D Z00C", 2, 8, NULL },
	[0x3B] = { "SRL E Z00C", 2, 8, NULL },
	[0x3C] = { "SRL H Z00C", 2, 8, NULL },
	[0x3D] = { "SRL L Z00C", 2, 8, NULL },
	[0x3E] = { "SRL [HL] Z00C", 2, 16, NULL },
	[0x3F] = { "SRL A Z00C", 2, 8, NULL },
	[0x40] = { "BIT 0, B Z01-", 2, 8, NULL },
	[0x41] = { "BIT 0, C Z01-", 2, 8, NULL },
	[0x42] = { "BIT 0, D Z01-", 2, 8, NULL },
	[0x43] = { "BIT 0, E Z01-", 2, 8, NULL },
	[0x44] = { "BIT 0, H Z01-", 2, 8, NULL },
	[0x45] = { "BIT 0, L Z01-", 2, 8, NULL },
	[0x46] = { "BIT 0, [HL] Z01-", 2, 12, NULL },
	[0x47] = { "BIT 0, A Z01-", 2, 8, NULL },
	[0x48] = { "BIT 1, B Z01-", 2, 8, NULL },
	[0x49] = { "BIT 1, C Z01-", 2, 8, NULL },
	[0x4A] = { "BIT 1, D Z01-", 2, 8, NULL },
	[0x4B] = { "BIT 1, E Z01-", 2, 8, NULL },
	[0x4C] = { "BIT 1, H Z01-", 2, 8, NULL },
	[0x4D] = { "BIT 1, L Z01-", 2, 8, NULL },
	[0x4E] = { "BIT 1, [HL] Z01-", 2, 12, NULL },
	[0x4F] = { "BIT 1, A Z01-", 2, 8, NULL },
	[0x50] = { "BIT 2, B Z01-", 2, 8, NULL },
	[0x51] = { "BIT 2, C Z01-", 2, 8, NULL },
	[0x52] = { "BIT 2, D Z01-", 2, 8, NULL },
	[0x53] = { "BIT 2, E Z01-", 2, 8, NULL },
	[0x54] = { "BIT 2, H Z01-", 2, 8, NULL },
	[0x55] = { "BIT 2, L Z01-", 2, 8, NULL },
	[0x56] = { "BIT 2, [HL] Z01-", 2, 12, NULL },
	[0x57] = { "BIT 2, A Z01-", 2, 8, NULL },
	[0x58] = { "BIT 3, B Z01-", 2, 8, NULL },
	[0x59] = { "BIT 3, C Z01-", 2, 8, NULL },
	[0x5A] = { "BIT 3, D Z01-", 2, 8, NULL },
	[0x5B] = { "BIT 3, E Z01-", 2, 8, NULL },
	[0x5C] = { "BIT 3, H Z01-", 2, 8, NULL },
	[0x5D] = { "BIT 3, L Z01-", 2, 8, NULL },
	[0x5E] = { "BIT 3, [HL] Z01-", 2, 12, NULL },
	[0x5F] = { "BIT 3, A Z01-", 2, 8, NULL },
	[0x60] = { "BIT 4, B Z01-", 2, 8, NULL },
	[0x61] = { "BIT 4, C Z01-", 2, 8, NULL },
	[0x62] = { "BIT 4, D Z01-", 2, 8, NULL },
	[0x63] = { "BIT 4, E Z01-", 2, 8, NULL },
	[0x64] = { "BIT 4, H Z01-", 2, 8, NULL },
	[0x65] = { "BIT 4, L Z01-", 2, 8, NULL },
	[0x66] = { "BIT 4, [HL] Z01-", 2, 12, NULL },
	[0x67] = { "BIT 4, A Z01-", 2, 8, NULL },
	[0x68] = { "BIT 5, B Z01-", 2, 8, NULL },
	[0x69] = { "BIT 5, C Z01-", 2, 8, NULL },
	[0x6A] = { "BIT 5, D Z01-", 2, 8, NULL },
	[0x6B] = { "BIT 5, E Z01-", 2, 8, NULL },
	[0x6C] = { "BIT 5, H Z01-", 2, 8, NULL },
	[0x6D] = { "BIT 5, L Z01-", 2, 8, NULL },
	[0x6E] = { "BIT 5, [HL] Z01-", 2, 12, NULL },
	[0x6F] = { "BIT 5, A Z01-", 2, 8, NULL },
	[0x70] = { "BIT 6, B Z01-", 2, 8, NULL },
	[0x71] = { "BIT 6, C Z01-", 2, 8, NULL },
	[0x72] = { "BIT 6, D Z01-", 2, 8, NULL },
	[0x73] = { "BIT 6, E Z01-", 2, 8, NULL },
	[0x74] = { "BIT 6, H Z01-", 2, 8, NULL },
	[0x75] = { "BIT 6, L Z01-", 2, 8, NULL },
	[0x76] = { "BIT 6, [HL] Z01-", 2, 12, NULL },
	[0x77] = { "BIT 6, A Z01-", 2, 8, NULL },
	[0x78] = { "BIT 7, B Z01-", 2, 8, NULL },
	[0x79] = { "BIT 7, C Z01-", 2, 8, NULL },
	[0x7A] = { "BIT 7, D Z01-", 2, 8, NULL },
	[0x7B] = { "BIT 7, E Z01-", 2, 8, NULL },
	[0x7C] = { "BIT 7, H Z01-", 2, 8, NULL },
	[0x7D] = { "BIT 7, L Z01-", 2, 8, NULL },
	[0x7E] = { "BIT 7, [HL] Z01-", 2, 12, NULL },
	[0x7F] = { "BIT 7, A Z01-", 2, 8, NULL },
	[0x80] = { "RES 0, B", 2, 8, NULL },
	[0x81] = { "RES 0, C", 2, 8, NULL },
	[0x82] = { "RES 0, D", 2, 8, NULL },
	[0x83] = { "RES 0, E", 2, 8, NULL },
	[0x84] = { "RES 0, H", 2, 8, NULL },
	[0x85] = { "RES 0, L", 2, 8, NULL },
	[0x86] = { "RES 0, [HL]", 2, 16, NULL },
	[0x87] = { "RES 0, A", 2, 8, NULL },
	[0x88] = { "RES 1, B", 2, 8, NULL },
	[0x89] = { "RES 1, C", 2, 8, NULL },
	[0x8A] = { "RES 1, D", 2, 8, NULL },
	[0x8B] = { "RES 1, E", 2, 8, NULL },
	[0x8C] = { "RES 1, H", 2, 8, NULL },
	[0x8D] = { "RES 1, L", 2, 8, NULL },
	[0x8E] = { "RES 1, [HL]", 2, 16, NULL },
	[0x8F] = { "RES 1, A", 2, 8, NULL },
	[0x90] = { "RES 2, B", 2, 8, NULL },
	[0x91] = { "RES 2, C", 2, 8, NULL },
	[0x92] = { "RES 2, D", 2, 8, NULL },
	[0x93] = { "RES 2, E", 2, 8, NULL },
	[0x94] = { "RES 2, H", 2, 8, NULL },
	[0x95] = { "RES 2, L", 2, 8, NULL },
	[0x96] = { "RES 2, [HL]", 2, 16, NULL },
	[0x97] = { "RES 2, A", 2, 8, NULL },
	[0x98] = { "RES 3, B", 2, 8, NULL },
	[0x99] = { "RES 3, C", 2, 8, NULL },
	[0x9A] = { "RES 3, D", 2, 8, NULL },
	[0x9B] = { "RES 3, E", 2, 8, NULL },
	[0x9C] = { "RES 3, H", 2, 8, NULL },
	[0x9D] = { "RES 3, L", 2, 8, NULL },
	[0x9E] = { "RES 3, [HL]", 2, 16, NULL },
	[0x9F] = { "RES 3, A", 2, 8, NULL },
	[0xA0] = { "RES 4, B", 2, 8, NULL },
	[0xA1] = { "RES 4, C", 2, 8, NULL },
	[0xA2] = { "RES 4, D", 2, 8, NULL },
	[0xA3] = { "RES 4, E", 2, 8, NULL },
	[0xA4] = { "RES 4, H", 2, 8, NULL },
	[0xA5] = { "RES 4, L", 2, 8, NULL },
	[0xA6] = { "RES 4, [HL]", 2, 16, NULL },
	[0xA7] = { "RES 4, A", 2, 8, NULL },
	[0xA8] = { "RES 5, B", 2, 8, NULL },
	[0xA9] = { "RES 5, C", 2, 8, NULL },
	[0xAA] = { "RES 5, D", 2, 8, NULL },
	[0xAB] = { "RES 5, E", 2, 8, NULL },
	[0xAC] = { "RES 5, H", 2, 8, NULL },
	[0xAD] = { "RES 5, L", 2, 8, NULL },
	[0xAE] = { "RES 5, [HL]", 2, 16, NULL },
	[0xAF] = { "RES 5, A", 2, 8, NULL },
	[0xB0] = { "RES 6, B", 2, 8, NULL },
	[0xB1] = { "RES 6, C", 2, 8, NULL },
	[0xB2] = { "RES 6, D", 2, 8, NULL },
	[0xB3] = { "RES 6, E", 2, 8, NULL },
	[0xB4] = { "RES 6, H", 2, 8, NULL },
	[0xB5] = { "RES 6, L", 2, 8, NULL },
	[0xB6] = { "RES 6, [HL]", 2, 16, NULL },
	[0xB7] = { "RES 6, A", 2, 8, NULL },
	[0xB8] = { "RES 7, B", 2, 8, NULL },
	[0xB9] = { "RES 7, C", 2, 8, NULL },
	[0xBA] = { "RES 7, D", 2, 8, NULL },
	[0xBB] = { "RES 7, E", 2, 8, NULL },
	[0xBC] = { "RES 7, H", 2, 8, NULL },
	[0xBD] = { "RES 7, L", 2, 8, NULL },
	[0xBE] = { "RES 7, [HL]", 2, 16, NULL },
	[0xBF] = { "RES 7, A", 2, 8, NULL },
	[0xC0] = { "SET 0, B", 2, 8, NULL },
	[0xC1] = { "SET 0, C", 2, 8, NULL },
	[0xC2] = { "SET 0, D", 2, 8, NULL },
	[0xC3] = { "SET 0, E", 2, 8, NULL },
	[0xC4] = { "SET 0, H", 2, 8, NULL },
	[0xC5] = { "SET 0, L", 2, 8, NULL },
	[0xC6] = { "SET 0, [HL]", 2, 16, NULL },
	[0xC7] = { "SET 0, A", 2, 8, NULL },
	[0xC8] = { "SET 1, B", 2, 8, NULL },
	[0xC9] = { "SET 1, C", 2, 8, NULL },
	[0xCA] = { "SET 1, D", 2, 8, NULL },
	[0xCB] = { "SET 1, E", 2, 8, NULL },
	[0xCC] = { "SET 1, H", 2, 8, NULL },
	[0xCD] = { "SET 1, L", 2, 8, NULL },
	[0xCE] = { "SET 1, [HL]", 2, 16, NULL },
	[0xCF] = { "SET 1, A", 2, 8, NULL },
	[0xD0] = { "SET 2, B", 2, 8, NULL },
	[0xD1] = { "SET 2, C", 2, 8, NULL },
	[0xD2] = { "SET 2, D", 2, 8, NULL },
	[0xD3] = { "SET 2, E", 2, 8, NULL },
	[0xD4] = { "SET 2, H", 2, 8, NULL },
	[0xD5] = { "SET 2, L", 2, 8, NULL },
	[0xD6] = { "SET 2, [HL]", 2, 16, NULL },
	[0xD7] = { "SET 2, A", 2, 8, NULL },
	[0xD8] = { "SET 3, B", 2, 8, NULL },
	[0xD9] = { "SET 3, C", 2, 8, NULL },
	[0xDA] = { "SET 3, D", 2, 8, NULL },
	[0xDB] = { "SET 3, E", 2, 8, NULL },
	[0xDC] = { "SET 3, H", 2, 8, NULL },
	[0xDD] = { "SET 3, L", 2, 8, NULL },
	[0xDE] = { "SET 3, [HL]", 2, 16, NULL },
	[0xDF] = { "SET 3, A", 2, 8, NULL },
	[0xE0] = { "SET 4, B", 2, 8, NULL },
	[0xE1] = { "SET 4, C", 2, 8, NULL },
	[0xE2] = { "SET 4, D", 2, 8, NULL },
	[0xE3] = { "SET 4, E", 2, 8, NULL },
	[0xE4] = { "SET 4, H", 2, 8, NULL },
	[0xE5] = { "SET 4, L", 2, 8, NULL },
	[0xE6] = { "SET 4, [HL]", 2, 16, NULL },
	[0xE7] = { "SET 4, A", 2, 8, NULL },
	[0xE8] = { "SET 5, B", 2, 8, NULL },
	[0xE9] = { "SET 5, C", 2, 8, NULL },
	[0xEA] = { "SET 5, D", 2, 8, NULL },
	[0xEB] = { "SET 5, E", 2, 8, NULL },
	[0xEC] = { "SET 5, H", 2, 8, NULL },
	[0xED] = { "SET 5, L", 2, 8, NULL },
	[0xEE] = { "SET 5, [HL]", 2, 16, NULL },
	[0xEF] = { "SET 5, A", 2, 8, NULL },
	[0xF0] = { "SET 6, B", 2, 8, NULL },
	[0xF1] = { "SET 6, C", 2, 8, NULL },
	[0xF2] = { "SET 6, D", 2, 8, NULL },
	[0xF3] = { "SET 6, E", 2, 8, NULL },
	[0xF4] = { "SET 6, H", 2, 8, NULL },
	[0xF5] = { "SET 6, L", 2, 8, NULL },
	[0xF6] = { "SET 6, [HL]", 2, 16, NULL },
	[0xF7] = { "SET 6, A", 2, 8, NULL },
	[0xF8] = { "SET 7, B", 2, 8, NULL },
	[0xF9] = { "SET 7, C", 2, 8, NULL },
	[0xFA] = { "SET 7, D", 2, 8, NULL },
	[0xFB] = { "SET 7, E", 2, 8, NULL },
	[0xFC] = { "SET 7, H", 2, 8, NULL },
	[0xFD] = { "SET 7, L", 2, 8, NULL },
	[0xFE] = { "SET 7, [HL]", 2, 16, NULL },
	[0xFF] = { "SET 7, A", 2, 8, NULL },
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

	cpu->ime = cpu->dblspd = false;
	cpu->pc = 0x150; // assume valid rom. (cpu->pc = 0x100)
	fprintf(stderr,
		"af 0x%04x (addr 0x%p), a 0x%02x (addr 0x%p), f 0x%02x (addr 0x%p)\n",
		cpu->af, &cpu->af, cpu->a, &cpu->a, cpu->f, &cpu->f);
	fprintf(stderr,
		"hl 0x%04x (addr 0x%p), h 0x%02x (addr 0x%p), l 0x%02x (addr 0x%p)\n",
		cpu->hl, &cpu->hl, cpu->h, &cpu->h, cpu->l, &cpu->l);

	initreg(cpu);
	initreg_cgb(cpu);
}

int cpu_exec(struct CPU *cpu)
{
	int result = 4;
	return result;
}

void cpu_fetch(struct CPU *cpu)
{
}

int cpu_step(struct CPU *cpu)
{
	int result = cpu_exec(cpu);
	cpu_fetch(cpu);
	return result;
}
