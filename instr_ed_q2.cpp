#include "pch.h"
#include "instr_decoder.h"

using namespace llz80emu;

void z80_instr_decoder::exec_blk_ld() {
	switch (_step) {
	case 0:
		_ctx.start_mem_read_cycle(_regs.REG_HL, _regs.REG_Z);
		break;
	case 1:
		_ctx.start_mem_write_cycle(_regs.REG_DE, _regs.REG_Z);
		break;
	case 2:
		if (_y & 1) {
			_regs.REG_HL--;
			_regs.REG_DE--;
		}
		else {
			_regs.REG_HL++;
			_regs.REG_DE++;
		}
		_regs.REG_BC--;
		_regs.REG_Z += _regs.REG_A; // for simulating F3 and F5 flag behaviour
		_regs.REG_F =
			(_regs.REG_F & (Z80_FLAG_C | Z80_FLAG_Z | Z80_FLAG_S))
			| ((_regs.REG_Z & (1 << 1)) << Z80_FLAGBIT_F5)
			| (_regs.REG_Z & Z80_FLAG_F3) // we can just copy this one (same bit position)
			| ((bool)_regs.REG_BC << Z80_FLAGBIT_PV);
		_ctx.start_bogus_cycle(2);
		break;
	case 3:
		if (!(_y & 0b010) || !(_regs.REG_F & Z80_FLAGBIT_PV)) reset(); // BC == 0 or non-repeating instruction
		else {
			_regs.REG_PC -= 2; // rewind PC
			_ctx.start_bogus_cycle(5);
		}
		break;
	default:
		reset();
		break;
	}
}

void z80_instr_decoder::exec_blk_cp() {
	switch (_step) {
	case 0:
		_ctx.start_mem_read_cycle(_regs.REG_HL, _regs.REG_Z);
		break;
	case 1:
		{
			if (_y & 1) _regs.REG_HL--; else _regs.REG_HL++;
			_regs.REG_BC--;
			uint8_t carry = _regs.REG_F & Z80_FLAG_C; // save old carry flag since ALU stub modifies it
			uint8_t y = _y; _y = 0b111; exec_alu_stub(); _y = y; // use CP from ALU stub for S, N, Z and H
			_regs.REG_Z = _regs.REG_A - _regs.REG_Z - ((_regs.REG_F >> Z80_FLAGBIT_H) & 1); // for simulating F3 and F5 flag behaviour
			_regs.REG_F =
				(_regs.REG_F & (Z80_FLAG_S | Z80_FLAG_N | Z80_FLAG_Z | Z80_FLAG_H))
				| carry // restore carry flag
				| ((_regs.REG_Z & (1 << 1)) << Z80_FLAGBIT_F5)
				| (_regs.REG_Z & Z80_FLAG_F3) // we can just copy this one (same bit position)
				| ((bool)_regs.REG_BC << Z80_FLAGBIT_PV);
			_ctx.start_bogus_cycle(5);
		}
		break;
	case 2:
		if (!(_y & 0b010) || !(_regs.REG_F & Z80_FLAGBIT_PV)) reset(); // BC == 0 or non-repeating instruction
		else {
			_regs.REG_PC -= 2; // rewind PC
			_ctx.start_bogus_cycle(5);
		}
		break;
	default:
		reset();
		break;
	}
}

void z80_instr_decoder::exec_blk_in() {
	switch (_step) {
	case 0:
		_ctx.start_bogus_cycle(1);
		break;
	case 1:
		_ctx.start_io_read_cycle(_regs.REG_BC, _regs.REG_Z);
		break;
	case 2:
		_ctx.start_mem_write_cycle(_regs.REG_HL, _regs.REG_Z);
		break;
	case 3:
		if (_y & 1) _regs.REG_HL--; else _regs.REG_HL++;
		_regs.REG_B--;
		_regs.REG_F =
			(_regs.REG_F & (Z80_FLAG_C | Z80_FLAG_S | Z80_FLAG_H | Z80_FLAG_PV)) // NOTE: S, H and PV are undefined
			| (_regs.REG_Z & (Z80_FLAG_F3 | Z80_FLAG_F5)) // TODO: check if this is correct
			| ((bool)!_regs.REG_B << Z80_FLAGBIT_Z);
		if (!(_y & 0b010) || !(_regs.REG_F & Z80_FLAGBIT_PV)) reset(); // B == 0 or non-repeating instruction
		else {
			_regs.REG_PC -= 2; // rewind PC
			_ctx.start_bogus_cycle(5);
		}
		break;
	default:
		reset();
		break;
	}
}

void z80_instr_decoder::exec_blk_out() {
	switch (_step) {
	case 0:
		_ctx.start_bogus_cycle(1);
		break;
	case 1:
		_ctx.start_mem_read_cycle(_regs.REG_HL, _regs.REG_Z);
		break;
	case 2:
		_ctx.start_io_write_cycle(_regs.REG_BC, _regs.REG_Z);
		break;
	case 3:
		if (_y & 1) _regs.REG_HL--; else _regs.REG_HL++;
		_regs.REG_B--;
		_regs.REG_F =
			(_regs.REG_F & (Z80_FLAG_C | Z80_FLAG_S | Z80_FLAG_H | Z80_FLAG_PV)) // NOTE: S, H and PV are undefined
			| (_regs.REG_Z & (Z80_FLAG_F3 | Z80_FLAG_F5)) // TODO: check if this is correct
			| ((bool)!_regs.REG_B << Z80_FLAGBIT_Z);
		if (!(_y & 0b010) || !(_regs.REG_F & Z80_FLAGBIT_PV)) reset(); // B == 0 or non-repeating instruction
		else {
			_regs.REG_PC -= 2; // rewind PC
			_ctx.start_bogus_cycle(5);
		}
		break;
	default:
		reset();
		break;
	}
}

void z80_instr_decoder::exec_ed_q2() {
	if (!(_y & 0b100) || (_x & 0b100)) {
		/* empty opcode slots */
		reset();
		return;
	}

	switch (_z) {
	case 0b000:
		exec_blk_ld();
		break;
	case 0b001:
		exec_blk_cp();
		break;
	case 0b010:
		exec_blk_in();
		break;
	case 0b011:
		exec_blk_out();
		break;
	}
}