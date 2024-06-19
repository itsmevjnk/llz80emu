#include "pch.h"
#include "instr_decoder.h"

using namespace llz80emu;

z80_instr_decoder::z80_instr_decoder(z80emu& ctx, z80_registers_t& regs) : _ctx(ctx), _regs(regs) {
	uint8_t* r8[] = { &regs.REG_B, &regs.REG_C, &regs.REG_D, &regs.REG_E, &regs.REG_H, &regs.REG_L, nullptr /* (HL) */, &regs.REG_A };
	memcpy(_reg8, r8, sizeof(r8));
	uint16_t* r16[] = { &regs.REG_BC, &regs.REG_DE, &regs.REG_HL, &regs.REG_SP };
	memcpy(_reg16, r16, sizeof(r16));
}

void z80_instr_decoder::start() {
	if (_subset == Z80_SUBSET_NONE) {
		/* take in prefixes */
		switch (_regs.instr) {
		case 0xDD:
			_mod = Z80_MOD_DD;
			_ctx.start_fetch_cycle();
			return;
		case 0xFD:
			_mod = Z80_MOD_FD;
			_ctx.start_fetch_cycle();
			return;
		case 0xCB:
			_subset = Z80_SUBSET_CB;
			_ctx.start_fetch_cycle();
			return;
		case 0xED:
			_subset = Z80_SUBSET_ED;
			_mod = Z80_MOD_NONE; // 0xED prefix disregards 0xDD/FD prefixes
			_ctx.start_fetch_cycle();
			return;
		}
	}

	_step = 0; // reset step counter
	//_fetch = false; // just in case
	_x = (_regs.instr & 0b11000000) >> 6; // decode opcode into octals
	_y = (_regs.instr & 0b00111000) >> 3;
	_z = (_regs.instr & 0b00000111);
	next_step(); // start execution
}

void z80_instr_decoder::next_step() {
	switch (_subset) {
	case Z80_SUBSET_NONE:
		exec_main();
		break;
	default:
		break;
	}

	_step++;
}

void z80_instr_decoder::reset() {
	_subset = Z80_SUBSET_NONE; _mod = Z80_MOD_NONE;
	_ctx.start_fetch_cycle();
}

void z80_instr_decoder::exec_main() {
	switch (_x) {
	case 0b00:
		exec_main_q0();
		break;
	case 0b01:
		exec_main_q1();
		break;
	case 0b10:
		exec_main_q2();
		break;
	default:
		reset();
		break;
	}
}