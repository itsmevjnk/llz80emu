#include "pch.h"
#include "instr_decoder.h"

using namespace llz80emu;

void z80_instr_decoder::exec_alu_stub() {
	/* perform ALU op and save to tmp */
	uint16_t tmp = 0;
	switch (_y) {
	case 0b000: // ADD
		tmp = _regs.REG_A + _regs.REG_Z;
		_regs.REG_F =
			(tmp & (Z80_FLAG_S | Z80_FLAG_F3 | Z80_FLAG_F5)) // S = MSB of result, and copy bits 3 and 5
			| (((bool)!(tmp & 0xFF)) << Z80_FLAGBIT_Z) // Z contains whether the result is zero
			| (((_regs.REG_A & (1 << 7)) == (_regs.REG_Z & (1 << 7)) && (_regs.REG_A & (1 << 7)) != (tmp & (1 << 7))) << Z80_FLAGBIT_PV) // PV contains whether an overflow occurs
			| (((bool)(tmp & 0xFF00)) << Z80_FLAGBIT_C) // C contains whether there's a carry (unsigned overflow)
			| (0 << Z80_FLAGBIT_N) // reset subtract flag
			| (((bool)(((_regs.REG_A & 0x0F) + (_regs.REG_Z & 0x0F)) & ~0x0F)) << Z80_FLAGBIT_H); // crude way to detect half carry
		break;
	case 0b001: // ADC
		_regs.REG_F &= ~Z80_FLAG_PV; // clear overflow flag first
		if (_regs.REG_F & 1) { // add carry bit to temp register
			_regs.REG_Z++;
			if (!(_regs.REG_Z & 0x7F)) _regs.REG_F |= Z80_FLAG_PV; // overflow (0x7F -> 0x80 or 0xFF -> 0x00) - set overflow flag here
		}
		tmp = _regs.REG_A + _regs.REG_Z;
		_regs.REG_F =
			(tmp & (Z80_FLAG_S | Z80_FLAG_F3 | Z80_FLAG_F5)) // S = MSB of result, and copy bits 3 and 5
			| (((bool)!(tmp & 0xFF)) << Z80_FLAGBIT_Z) // Z contains whether the result is zero
			| ((_regs.REG_F & Z80_FLAG_PV) | ((_regs.REG_A & (1 << 7)) == (_regs.REG_Z & (1 << 7)) && (_regs.REG_A & (1 << 7)) != (tmp & (1 << 7))) << Z80_FLAGBIT_PV) // PV contains whether an overflow occurs
			| (((bool)(tmp & 0xFF00)) << Z80_FLAGBIT_C) // C contains whether there's a carry (unsigned overflow)
			| (0 << Z80_FLAGBIT_N) // reset subtract flag
			| (((bool)(((_regs.REG_A & 0x0F) + (_regs.REG_Z & 0x0F)) & ~0x0F)) << Z80_FLAGBIT_H); // crude way to detect half carry
		break;
	case 0b010: // SUB
		tmp = _regs.REG_A - _regs.REG_Z;
		_regs.REG_F =
			(tmp & (Z80_FLAG_S | Z80_FLAG_F3 | Z80_FLAG_F5)) // S = MSB of result, and copy bits 3 and 5
			| (((bool)!(tmp & 0xFF)) << Z80_FLAGBIT_Z) // Z contains whether the result is zero
			| (((_regs.REG_A & (1 << 7)) != (_regs.REG_Z & (1 << 7)) && (_regs.REG_A & (1 << 7)) != (tmp & (1 << 7))) << Z80_FLAGBIT_PV) // PV contains whether an overflow occurs
			| ((_regs.REG_A < _regs.REG_Z) << Z80_FLAGBIT_C) // C contains whether there's a borrow (unsigned underflow)
			| (1 << Z80_FLAGBIT_N) // set subtract flag
			| (((_regs.REG_A & 0x0F) < (_regs.REG_Z & 0x0F)) << Z80_FLAGBIT_H); // crude way to detect half borrow
		break;
	case 0b011: // SBC
		_regs.REG_F &= ~Z80_FLAG_PV; // clear overflow flag first
		if (_regs.REG_F & 1) { // add carry bit to temp register
			_regs.REG_Z++;
			if (!(_regs.REG_Z & 0x7F)) _regs.REG_F |= Z80_FLAG_PV; // overflow (0x7F -> 0x80 or 0xFF -> 0x00) - set overflow flag here (TODO)
		}
		tmp = _regs.REG_A - _regs.REG_Z;
		_regs.REG_F =
			(tmp & (Z80_FLAG_S | Z80_FLAG_F3 | Z80_FLAG_F5)) // S = MSB of result, and copy bits 3 and 5
			| (((bool)!(tmp & 0xFF)) << Z80_FLAGBIT_Z) // Z contains whether the result is zero
			| ((_regs.REG_F & Z80_FLAG_PV) | (((_regs.REG_A & (1 << 7)) != (_regs.REG_Z & (1 << 7)) && (_regs.REG_A & (1 << 7)) != (tmp & (1 << 7))) << Z80_FLAGBIT_PV)) // PV contains whether an overflow occurs
			| ((_regs.REG_A < _regs.REG_Z) << Z80_FLAGBIT_C) // C contains whether there's a borrow (unsigned underflow)
			| (1 << Z80_FLAGBIT_N) // set subtract flag
			| (((_regs.REG_A & 0x0F) < (_regs.REG_Z & 0x0F)) << Z80_FLAGBIT_H); // crude way to detect half borrow
		break;
	case 0b100: // AND
		tmp = _regs.REG_A & _regs.REG_Z;
		_regs.REG_F =
			(tmp & (Z80_FLAG_S | Z80_FLAG_F3 | Z80_FLAG_F5)) // S = MSB of result, and copy bits 3 and 5
			| (((bool)!(tmp & 0xFF)) << Z80_FLAGBIT_Z) // Z contains whether the result is zero
			| Z80_FLAG_H // set H flag to 1
			| (parity((uint8_t)tmp) << Z80_FLAGBIT_PV); // calculate parity of result
		break;
	case 0b101: // XOR
		tmp = _regs.REG_A ^ _regs.REG_Z;
		_regs.REG_F =
			(tmp & (Z80_FLAG_S | Z80_FLAG_F3 | Z80_FLAG_F5)) // S = MSB of result, and copy bits 3 and 5
			| (((bool)!(tmp & 0xFF)) << Z80_FLAGBIT_Z) // Z contains whether the result is zero
			| (parity((uint8_t)tmp) << Z80_FLAGBIT_PV); // calculate parity of result
		break;
	case 0b110: // OR
		tmp = _regs.REG_A | _regs.REG_Z;
		_regs.REG_F =
			(tmp & (Z80_FLAG_S | Z80_FLAG_F3 | Z80_FLAG_F5)) // S = MSB of result, and copy bits 3 and 5
			| (((bool)!(tmp & 0xFF)) << Z80_FLAGBIT_Z) // Z contains whether the result is zero
			| (parity((uint8_t)tmp) << Z80_FLAGBIT_PV); // calculate parity of result
		break;
	case 0b111: // CP
		tmp = _regs.REG_A - _regs.REG_Z;
		_regs.REG_F =
			(tmp & Z80_FLAG_S) // S = MSB of result
			| (_regs.REG_Z & (Z80_FLAG_F3 | Z80_FLAG_F5)) // bits 3 and 5 are copied from the operand
			| (((bool)!(tmp & 0xFF)) << Z80_FLAGBIT_Z) // Z contains whether the result is zero
			| (((_regs.REG_A & (1 << 7)) != (_regs.REG_Z & (1 << 7)) && (_regs.REG_A & (1 << 7)) != (tmp & (1 << 7))) << Z80_FLAGBIT_PV) // PV contains whether an overflow occurs
			| ((_regs.REG_A < _regs.REG_Z) << Z80_FLAGBIT_C) // C contains whether there's a borrow (unsigned underflow)
			| (1 << Z80_FLAGBIT_N) // set subtract flag
			| (((_regs.REG_A & 0x0F) < (_regs.REG_Z & 0x0F)) << Z80_FLAGBIT_H); // crude way to detect half borrow
		break;
	}
	if (_y != 0b111) _regs.REG_A = tmp & 0xFF; // CP discards the result
	reset();
}

void z80_instr_decoder::exec_main_q2() {
	const uint8_t* src = reg8(_z); // decode source register
	if (!src) {
		if (!_step) {
			/* stage memory read from (HL) to one of our temp regs */
			if (!process_hlptr()) return;
			_ctx.start_mem_read_cycle(_hl_ptr, _regs.REG_Z);
			return;
		}
	} else _regs.REG_Z = *src;

	exec_alu_stub();
}