#include "instr_decoder.h"
#include "z80emu.h"

using namespace llz80emu;

void z80_instr_decoder::exec_cb() {
	switch (_x) {
	case 0b00:
		exec_shift_rot();
		break;
	case 0b01:
		exec_bit();
		break;
	case 0b10:
		exec_res();
		break;
	case 0b11:
		exec_set();
		break;
	}
}

void z80_instr_decoder::exec_shift_rot() {
	uint8_t* reg = reg8(_z); // NULL for (HL)
	int s = _step;
	if (!_step) {
		if (!reg || _mod != Z80_MOD_NONE) {
			/* read (HL) into Z - we'll also do that regardless of register for DD/FD prefixes */
			_ctx.start_mem_read_cycle((_mod == Z80_MOD_NONE) ? _regs.REG_HL : _hl_ptr, _regs.REG_Z);
			return;
		}
		else _regs.REG_Z = *reg; // copy register to Z to work on
	}

	if (!reg) s--; // back by 1 step (1st step is reading into Z)

	if (!s) {
		uint8_t carry = (_regs.REG_F >> Z80_FLAGBIT_C) & 1; // old carry bit
		switch (_y) {
		case 0b000: // RLC
			_regs.REG_F = (_regs.REG_Z >> 7) << Z80_FLAGBIT_C; // copy bit 7 to C (we don't preserve flag bits)
			_regs.REG_Z = (_regs.REG_Z << 1) | (_regs.REG_Z >> 7);
			break;
		case 0b001: // RRC
			_regs.REG_F = (_regs.REG_Z & 1) << Z80_FLAGBIT_C; // copy bit 0 to C
			_regs.REG_Z = (_regs.REG_Z >> 1) | (_regs.REG_Z << 7);
			break;
		case 0b010: // RL
			_regs.REG_F = (_regs.REG_Z >> 7) << Z80_FLAGBIT_C; // copy bit 7 to C
			_regs.REG_Z = (_regs.REG_Z << 1) | carry;
			break;
		case 0b011: // RR
			_regs.REG_F = (_regs.REG_Z & 1) << Z80_FLAGBIT_C; // copy bit 0 to C
			_regs.REG_Z = (_regs.REG_Z >> 1) | (carry << 7);
			break;
		case 0b100: // SLA
			_regs.REG_F = (_regs.REG_Z >> 7) << Z80_FLAGBIT_C; // copy bit 7 to C
			_regs.REG_Z <<= 1;
			break;
		case 0b101: // SRA
			_regs.REG_F = (_regs.REG_Z & 1) << Z80_FLAGBIT_C; // copy bit 0 to C
			_regs.REG_Z = (_regs.REG_Z & (1 << 7)) | (_regs.REG_Z >> 1); // preserve bit 7
			break;
		case 0b110: // SLL
			_regs.REG_F = (_regs.REG_Z >> 7) << Z80_FLAGBIT_C; // copy bit 7 to C
			_regs.REG_Z = (_regs.REG_Z << 1) | 1;
			break;
		case 0b111: // SRL
			_regs.REG_F = (_regs.REG_Z & 1) << Z80_FLAGBIT_C; // copy bit 0 to C
			_regs.REG_Z >>= 1;
			break;
		}
		_regs.Q = _regs.REG_F =
			(_regs.REG_F & Z80_FLAG_C) // preserve carry flag we just set above
			| (parity(_regs.REG_Z) << Z80_FLAGBIT_PV)
			| (_regs.REG_Z & (Z80_FLAG_F3 | Z80_FLAG_F5 | Z80_FLAG_S))
			| ((bool)!_regs.REG_Z << Z80_FLAGBIT_Z);

		if (reg) *reg = _regs.REG_Z; // save to destination register
		if (!reg || _mod != Z80_MOD_NONE) {
			/* insert 1 bogus cycle if our instruction involves (HL/IX+d/IY+d) */
			_ctx.start_bogus_cycle(1);
			return;
		}
	}
	else if (s == 1 && (!reg || _mod != Z80_MOD_NONE)) {
		/* write back to (HL/IX+d/IY+d) */
		_ctx.start_mem_write_cycle((_mod == Z80_MOD_NONE) ? _regs.REG_HL : _hl_ptr, _regs.REG_Z);
		return;
	}
	reset();
}

void z80_instr_decoder::exec_bit() {
	uint8_t* reg = reg8(_z); // NULL for (HL)
	int s = _step;
	if (!_step) {
		if (!reg || _mod != Z80_MOD_NONE) {
			/* read (HL) into Z */
			_ctx.start_mem_read_cycle((_mod == Z80_MOD_NONE) ? _regs.REG_HL : _hl_ptr, _regs.REG_Z);
			return;
		}
		else _regs.REG_Z = *reg; // copy register to Z to work on
	}

	if (!reg) s--; // back by 1 step (1st step is reading into Z)

	if (!s) {
		_regs.Q =
			(_regs.REG_F & Z80_FLAG_C) // preserve C flag
			| Z80_FLAG_H
			| (_regs.REG_Z & (Z80_FLAG_F3 | Z80_FLAG_F5)); // copy bit 3 and 5 from operand instead of result
		_regs.REG_Z &= (1 << _y);
		_regs.Q = _regs.REG_F =
			_regs.Q
			| ((!_regs.REG_Z) ? (Z80_FLAG_Z | Z80_FLAG_PV) : 0)
			| (_regs.REG_Z & Z80_FLAG_S);
		if (!reg || _mod != Z80_MOD_NONE) {
			_ctx.start_bogus_cycle(1); // run 1 bogus cycle for (HL)
			return;
		}
	}
	reset();
}

void z80_instr_decoder::exec_res() {
	_regs.Q = 0; // not setting flags
	uint8_t* reg = reg8(_z); // NULL for (HL)
	int s = _step;
	if (!_step) {
		if (!reg || _mod != Z80_MOD_NONE) {
			/* read (HL) into Z */
			_ctx.start_mem_read_cycle((_mod == Z80_MOD_NONE) ? _regs.REG_HL : _hl_ptr, _regs.REG_Z);
			return;
		}
		else _regs.REG_Z = *reg; // copy register to Z to work on
	}

	if (!reg) s--; // back by 1 step (1st step is reading into Z)
	
	if (!s) {
		_regs.REG_Z &= ~(1 << _y);
		if (reg) *reg = _regs.REG_Z; // save to destination register
		if (!reg || _mod != Z80_MOD_NONE) {
			/* insert 1 bogus cycle if our instruction involves (HL/IX+d/IY+d) */
			_ctx.start_bogus_cycle(1);
			return;
		}
	}
	else if (s == 1 && (!reg || _mod != Z80_MOD_NONE)) {
		/* write back to (HL/IX+d/IY+d) */
		_ctx.start_mem_write_cycle((_mod == Z80_MOD_NONE) ? _regs.REG_HL : _hl_ptr, _regs.REG_Z);
		return;
	}
	reset();
}

void z80_instr_decoder::exec_set() {
	_regs.Q = 0; // not setting flags
	uint8_t* reg = reg8(_z); // NULL for (HL)
	int s = _step;
	if (!_step) {
		if (!reg || _mod != Z80_MOD_NONE) {
			/* read (HL) into Z */
			_ctx.start_mem_read_cycle((_mod == Z80_MOD_NONE) ? _regs.REG_HL : _hl_ptr, _regs.REG_Z);
			return;
		}
		else _regs.REG_Z = *reg; // copy register to Z to work on
	}

	if (!reg) s--; // back by 1 step (1st step is reading into Z)

	if (!s) {
		_regs.REG_Z |= (1 << _y);
		if (reg) *reg = _regs.REG_Z; // save to destination register
		if (!reg || _mod != Z80_MOD_NONE) {
			/* insert 1 bogus cycle if our instruction involves (HL/IX+d/IY+d) */
			_ctx.start_bogus_cycle(1);
			return;
		}
	}
	else if (s == 1 && (!reg || _mod != Z80_MOD_NONE)) {
		/* write back to (HL/IX+d/IY+d) */
		_ctx.start_mem_write_cycle((_mod == Z80_MOD_NONE) ? _regs.REG_HL : _hl_ptr, _regs.REG_Z);
		return;
	}
	reset();
}