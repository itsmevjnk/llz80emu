#include "pch.h"
#include "instr_decoder.h"

using namespace llz80emu;

void z80_instr_decoder::exec_io_r8(bool out) {
	switch (_step) {
	case 0:
		if (out) _ctx.start_io_write_cycle(_regs.REG_BC, (_y == 0b110) ? 0 : *reg8(_y));
		else _ctx.start_io_read_cycle(_regs.REG_BC, _regs.REG_Z); // we'll copy the result to the destination register later
		break;
	default:
		if (!out) {
			/* IN - affect flags */
			_regs.REG_F =
				(_regs.REG_F & Z80_FLAG_C) // all other flags are modified
				| (_regs.REG_Z & (Z80_FLAG_F3 | Z80_FLAG_F5 | Z80_FLAG_S))
				| ((bool)!_regs.REG_Z << Z80_FLAGBIT_Z)
				| (parity(_regs.REG_Z) << Z80_FLAGBIT_PV);
			if (_y != 0b110) *reg8(_y) = _regs.REG_Z; // copy result
		}
		reset();
		break;
	}
}

void z80_instr_decoder::exec_adc_sbc_hl_r16() {
	if (!_step) {
		uint32_t tmp = 0;
		uint16_t addend = *reg16(_y >> 1);
		if (_y & 1) {
			/* ADC */
			_regs.REG_F &= ~Z80_FLAG_PV; // clear overflow flag first
			if (_regs.REG_F & 1) { // add carry bit to temp register
				addend++;
				if (!(addend & 0x7F00)) _regs.REG_F |= Z80_FLAG_PV; // overflow (0x7Fxx -> 0x80xx or 0xFFxx -> 0x00xx) - set overflow flag here
			}
			tmp = _regs.REG_HL + addend;
			_regs.REG_F =
				((tmp >> 8) & (Z80_FLAG_S | Z80_FLAG_F3 | Z80_FLAG_F5)) // S = MSB of result, and copy bits 3 and 5 (from high byte)
				| (((bool)!(tmp & 0xFFFF)) << Z80_FLAGBIT_Z) // Z contains whether the result is zero
				| ((_regs.REG_F & Z80_FLAG_PV) | ((_regs.REG_A & (1 << 15)) == (addend & (1 << 15)) && (_regs.REG_A & (1 << 15)) != (tmp & (1 << 15))) << Z80_FLAGBIT_PV) // PV contains whether an overflow occurs
				| (((bool)(tmp & 0xFFFF0000)) << Z80_FLAGBIT_C) // C contains whether there's a carry (unsigned overflow)
				| (0 << Z80_FLAGBIT_N) // reset subtract flag
				| (((bool)(((_regs.REG_A & 0x0F00) + (addend & 0x0F00)) & ~0x0F00)) << Z80_FLAGBIT_H); // crude way to detect half carry
		}
		else {
			/* SBC */
			_regs.REG_F &= ~Z80_FLAG_PV; // clear overflow flag first
			if (_regs.REG_F & 1) { // add carry bit to temp register
				addend++;
				if (!(addend & 0x7F00)) _regs.REG_F |= Z80_FLAG_PV; // overflow (0x7Fxx -> 0x80xx or 0xFFxx -> 0x00xx) - set overflow flag here
			}
			tmp = _regs.REG_A - addend;
			_regs.REG_F =
				((tmp >> 8) & (Z80_FLAG_S | Z80_FLAG_F3 | Z80_FLAG_F5)) // S = MSB of result, and copy bits 3 and 5 (from high byte)
				| (((bool)!(tmp & 0xFFFF)) << Z80_FLAGBIT_Z) // Z contains whether the result is zero
				| ((_regs.REG_F & Z80_FLAG_PV) | (((_regs.REG_A & (1 << 15)) != (addend & (1 << 15)) && (_regs.REG_A & (1 << 15)) != (tmp & (1 << 15))) << Z80_FLAGBIT_PV)) // PV contains whether an overflow occurs
				| ((_regs.REG_A < addend) << Z80_FLAGBIT_C) // C contains whether there's a borrow (unsigned underflow)
				| (1 << Z80_FLAGBIT_N) // set subtract flag
				| (((_regs.REG_A & 0x0F00) < (addend & 0x0F00)) << Z80_FLAGBIT_H); // crude way to detect half borrow
		}

		_ctx.start_bogus_cycle(7);
	}
	else reset();
}

void z80_instr_decoder::exec_ld_r16_p16() {
	uint16_t* reg = reg16(_y >> 1);
	switch (_step) {
	case 0:
		_ctx.start_mem_read_cycle(_regs.REG_PC++, _regs.REG_Z);
		break;
	case 1:
		_ctx.start_mem_read_cycle(_regs.REG_PC++, _regs.REG_W);
		break;
	case 2: // read/write low byte
		if (_y & 1) _ctx.start_mem_read_cycle(_regs.REG_WZ + 0, *LB_PTR(reg));
		else _ctx.start_mem_write_cycle(_regs.REG_WZ + 0, *LB_PTR(reg));
		break;
	case 3: // read/write high byte
		if (_y & 1) _ctx.start_mem_read_cycle(_regs.REG_WZ + 1, *HB_PTR(reg));
		else _ctx.start_mem_write_cycle(_regs.REG_WZ + 1, *HB_PTR(reg));
		break;
	default:
		reset();
		break;
	}
}

void z80_instr_decoder::exec_ld_ir() {
	if (!_step) {
		uint8_t& ir = (_y & 1) ? _regs.REG_R : _regs.REG_I;
		if (_y & 0b10) {
			_regs.REG_A = ir; // LD A,I/R
			_regs.REG_F =
				(_regs.REG_F & Z80_FLAG_C)
				| (_regs.iff2 << Z80_FLAGBIT_PV)
				| (_regs.REG_A & (Z80_FLAG_S | Z80_FLAG_F3 | Z80_FLAG_F5))
				| (!_regs.REG_A << Z80_FLAGBIT_Z);
		}
		else ir = _regs.REG_A; // LD I/R,A
		_ctx.start_bogus_cycle(1);
	}
	else reset();
}

void z80_instr_decoder::exec_bcd_rotate() {
	switch (_step) {
	case 0:
		_ctx.start_mem_read_cycle(_regs.REG_HL, _regs.REG_Z);
		break;
	case 1:
		_regs.REG_W = _regs.REG_A;
		if (_y & 1) {
			/* RLD */
			_regs.REG_WZ = (_regs.REG_WZ << 4) | ((_regs.REG_WZ & 0xF000) >> 12);
		}
		else {
			/* RRD */
			_regs.REG_WZ = ((_regs.REG_WZ >> 4) & 0x0FF) | ((_regs.REG_WZ & 0x000F) << 8);
		}
		_regs.REG_A = (_regs.REG_A & 0xF0) | (_regs.REG_W & 0x0F);
		_ctx.start_bogus_cycle(4);
		break;
	case 2:
		_ctx.start_mem_write_cycle(_regs.REG_HL, _regs.REG_Z);
		break;
	default:
		_regs.REG_F =
			(_regs.REG_F & Z80_FLAG_C)
			| (parity(_regs.REG_A) << Z80_FLAGBIT_PV)
			| (_regs.REG_A & (Z80_FLAG_S | Z80_FLAG_F3 | Z80_FLAG_F5))
			| ((bool)!_regs.REG_A << Z80_FLAGBIT_Z);
		reset();
		break;
	}
}

void z80_instr_decoder::exec_ed_q1() {
	switch (_z) {
	case 0b000: // IN r8,(C)
		exec_io_r8(false);
		break;
	case 0b001: // OUT (C),r8
		exec_io_r8(true);
		break;
	case 0b010: // ADC/SBC HL,BC/DE/HL/SP
		exec_adc_sbc_hl_r16();
		break;
	case 0b011: // LD (nn),BC/DE/HL/SP / LD BC/DE/HL/SP,(nn)
		exec_ld_r16_p16();
		break;
	case 0b100: // NEG - reuse exec_alu_stub() for this
		_y = 0b010; // SUB
		_regs.REG_Z = _regs.REG_A; _regs.REG_A = 0; // A = 0 - A
		exec_alu_stub();
		break;
	case 0b101: // RETI/RETN
		if (!_step) _regs.iff1 = _regs.iff2; // restore IFF1
		exec_uncond_ret(); // other than that it's the same thing as RET
		break;
	case 0b110: // IM 0/1/2
		switch (_y & 0b011) {
		case 0b10:
			_regs.int_mode = 1;
			break;
		case 0b11:
			_regs.int_mode = 2;
			break;
		default:
			_regs.int_mode = 0;
			break;
		}
		reset();
		break;
	case 0b111: // assorted instructions
		switch (_y >> 1) {
		case 0b00:
		case 0b01:
			exec_ld_ir();
			break;
		case 0b10:
			exec_bcd_rotate();
			break;
		default:
			reset();
			break;
		}
		break;
	}
}