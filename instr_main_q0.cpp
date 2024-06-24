#include "instr_decoder.h"
#include "z80emu.h"

using namespace llz80emu;

void z80_instr_decoder::exec_inc_r8() {
	uint8_t* r = reg8(_y); // select register to increment
	if (!r) {
		/* (HL) */
		switch (_step) {
		case 0: // initiate read from HL to Z
			if (!process_hlptr()) return;
			_ctx.start_mem_read_cycle(_hl_ptr, _regs.REG_Z);
			return;
		case 1: // set r to Z instead
			r = &_regs.REG_Z;
			_ctx.start_bogus_cycle(1); // also run one bogus cycle before we write the value back
			break;
		case 2: // write Z back to HL
			_ctx.start_mem_write_cycle(_hl_ptr, _regs.REG_Z);
			return;
		default: // reset 
			reset();
			return;
		}
	}
	(*r)++;
	_regs.REG_F =
		(_regs.REG_F & Z80_FLAG_C) // preserve carry flag
		| (*r & (Z80_FLAG_S | Z80_FLAG_F3 | Z80_FLAG_F5)) // copy sign bit and bits 3 and 5 from result
		| (!*r << Z80_FLAGBIT_Z)
		| (((bool)!(*r & 0x0F)) << Z80_FLAGBIT_H) // if r = xxxx0000 then it was xxxx1111 before the operation, and that there was a carry from bit 3 to 4 when we incremented it
		| ((*r == 0x80) << Z80_FLAGBIT_PV); // 0x7F (127) + 1 = 0x80 (-128) -> overflow
	if (_y != 0b110) reset(); // not (HL) - go back to fetching now
}

void z80_instr_decoder::exec_dec_r8() {
	uint8_t* r = reg8(_y); // select register to decrement
	if (!r) {
		/* (HL) */
		switch (_step) {
		case 0: // initiate read from HL to Z
			if (!process_hlptr()) return;
			_ctx.start_mem_read_cycle(_regs.REG_HL, _regs.REG_Z);
			return;
		case 1: // set r to Z instead
			r = &_regs.REG_Z;
			_ctx.start_bogus_cycle(1); // also run one bogus cycle before we write the value back
			break;
		case 2: // write Z back to HL
			_ctx.start_mem_write_cycle(_hl_ptr, _regs.REG_Z);
			return;
		default: // reset 
			reset();
			return;
		}
	}
	(*r)--;
	_regs.REG_F =
		(_regs.REG_F & Z80_FLAG_C) // preserve carry flag
		| (*r & (Z80_FLAG_S | Z80_FLAG_F3 | Z80_FLAG_F5)) // copy sign bit and bits 3 and 5 from result
		| (!*r << Z80_FLAGBIT_Z)
		| (((*r & 0x0F) == 0x0F) << Z80_FLAGBIT_H) // if r = xxxx1111 then it was xxxx0000 before the operation, and that there was a borrow from bit 4 to 3 when we decremented it
		| ((*r == 0x7F) << Z80_FLAGBIT_PV) // 0x80 (-128) - 1 = 0x7F (127) -> underflow
		| Z80_FLAG_N; // subtract operation
	if (_y != 0b110) reset(); // not (HL) - go back to fetching now
}

void z80_instr_decoder::exec_incdec_r16() {
	if (!_step) {
		if (_y & 1) (*reg16(_y >> 1))--; // DEC r16
		else (*reg16(_y >> 1))++; // INC r16
		_ctx.start_bogus_cycle(2); // insert two bogus cycles
	} else reset();
}



void z80_instr_decoder::exec_ld16_p16() {
	uint16_t* reg = reg16(_y >> 1); // ptr to register to read from/write to
	switch (_step) {
	case 0:
		_ctx.start_mem_read_cycle(_regs.REG_PC++, _regs.REG_Z); // ptr low byte
		break;
	case 1:
		_ctx.start_mem_read_cycle(_regs.REG_PC++, _regs.REG_W); // ptr high byte
		break;
	case 2: // read/write reg low byte
		if (_y & 1) _ctx.start_mem_read_cycle(_regs.REG_WZ + 0, *LB_PTR(reg));
		else _ctx.start_mem_write_cycle(_regs.REG_WZ + 0, *LB_PTR(reg));
		break;
	case 3: // read/write reg high byte
		if (_y & 1) _ctx.start_mem_read_cycle(_regs.REG_WZ + 1, *HB_PTR(reg));
		else _ctx.start_mem_write_cycle(_regs.REG_WZ + 1, *HB_PTR(reg));
		break;
	case 4: // restart
		reset();
		break;
	}
}

void z80_instr_decoder::exec_ld8_p16() {
	int s = _step; // for streamlining
	uint16_t* ptr = reg16(_y >> 1); // ptr to 16-bit address to read from/write to
	if ((_y & 0b110) == 0b110) {
		/* LD (nn),A / LD A,(nn) */
		switch (_step) {
		case 0:
			_ctx.start_mem_read_cycle(_regs.REG_PC++, _regs.REG_Z); // low byte
			return;
		case 1:
			_ctx.start_mem_read_cycle(_regs.REG_PC++, _regs.REG_W); // high byte
			return;
		default:
			ptr = &_regs.REG_WZ; // change ptr to our temp register
			s -= 2; // our first 2 steps are reading nn
			break;
		}
	}

	if (!s) {
		if (_y & 1) _ctx.start_mem_read_cycle(*ptr, _regs.REG_A); // LD A,(BC/DE/nn)
		else _ctx.start_mem_write_cycle(*ptr, _regs.REG_A); // LD (BC/DE/nn),A
	}
	else reset(); // restart
}

void z80_instr_decoder::exec_ld_i16() {
	uint16_t* r = reg16(_y >> 1); // ptr to register to load to
	switch (_step) {
	case 0:
		_ctx.start_mem_read_cycle(_regs.REG_PC++, *LB_PTR(r));
		break;
	case 1:
		_ctx.start_mem_read_cycle(_regs.REG_PC++, *HB_PTR(r));
		break;
	default:
		reset();
		break;
	}
}

void z80_instr_decoder::exec_add_hl_r16() {
	if (!_step) {
		uint16_t reg = *reg16(_y >> 1); // register to add to HL
		uint32_t tmp = _regs.REG_HL + reg; // temporary result register
		_regs.REG_F =
			(_regs.REG_F & (Z80_FLAG_S | Z80_FLAG_Z | Z80_FLAG_PV)) // S, Z and P/V are unaffected
			| ((tmp >> 8) & (Z80_FLAG_F3 | Z80_FLAG_F5)) // copy bits 3 and 5 from high byte
			| (((bool)(tmp & 0xFFFF0000)) << Z80_FLAGBIT_C) // C contains whether there's a carry (unsigned overflow)
			| (0 << Z80_FLAGBIT_N) // reset subtract flag
			| (((bool)(((_regs.REG_HL & 0x0FFF) + (reg & 0x0FFF)) & 0xF000)) << Z80_FLAGBIT_H); // crude way to detect half carry
		_regs.REG_HL = (uint16_t)tmp; // commit result to HL

		_ctx.start_bogus_cycle(7); // insert 7 bogus cycles here (since we did everything in one cycle now)
	} else reset();
}

void z80_instr_decoder::exec_ld_i8() {
	uint8_t* r = reg8(_y); // register to write to
	switch (_step) {
	case 0:
		if (!r && !process_hlptr(0, false)) return; // we need _hl_ptr but it's not ready yet (note that we don't want process_hlptr to inject bogus cycles nor set _hlptr_ready here - we'll do that ourselves)
		_ctx.start_mem_read_cycle(_regs.REG_PC++, (r) ? *r : _regs.REG_Z); // read directly into the register if it's not (HL); otherwise, we read into a temporary one
		break;
	case 1:
		if (r) {
			reset();
			return; // not (HL) - we can end here
		}
		if (_mod != Z80_MOD_NONE && !_hlptr_ready) {
			/* inject 2 extra clock cycles to emulate d offset calculation */
			_ctx.start_bogus_cycle(2);
			_hlptr_ready = true;
			_step--;
			return;
		}

		_ctx.start_mem_write_cycle(_hl_ptr, _regs.REG_Z); // write what we just read from the prev step into (HL)
		break;
	default:
		reset();
		break;
	}
}

void z80_instr_decoder::exec_jr_stub(bool take_branch, int step_start) {
	switch (_step - step_start) {
	case 0:
		_ctx.start_mem_read_cycle(_regs.REG_PC++, _regs.REG_Z); // read displacement byte into Z
		break;
	case 1:
		if (!take_branch) reset(); // not taking branch - restart now
		else {
			_regs.REG_W = (_regs.REG_Z >> 7) * 0xFF; // lazy 8->16bit sign extend operation
			_regs.REG_PC += _regs.REG_WZ;
			_ctx.start_bogus_cycle(5); // insert 5 bogus cycles
		}
		break;
	default:
		reset();
		break;
	}
}

void z80_instr_decoder::exec_shift_a() {
	bool dir = (_y & 0b001), c = !(_y & 0b010); // decode instruction: dir = true for RRCA/RRA (right shift), and c = true if the instruction is RLCA/RRCA

	bool shift_bit = _regs.REG_F & Z80_FLAG_C; // bit to use for bit 7 (RRA/RRCA) or bit 0 (RLA/RLCA), set to old carry bit (for RLA/RRA)
	_regs.REG_F = 
		(_regs.REG_F & ~(Z80_FLAG_C | Z80_FLAG_N | Z80_FLAG_H | Z80_FLAG_F3 | Z80_FLAG_F5)) // reset C, as well as N and H (as described) and F3 and F5 (for copying bits from A)
		| ((dir) ? (_regs.REG_A & 1) : (_regs.REG_A >> 7)) << Z80_FLAGBIT_C; // set carry flag to LSB (RRCA/RRA) or MSB (RLCA/RLA)
	if(c) shift_bit = _regs.REG_F & Z80_FLAG_C; // RLCA/RRCA uses new carry flag bit instead

	if (dir) _regs.REG_A = (_regs.REG_A >> 1) | (shift_bit << 7); // right shift
	else _regs.REG_A = (_regs.REG_A << 1) | ((uint8_t)shift_bit); // left shift
	_regs.REG_F |= _regs.REG_A & (Z80_FLAG_F3 | Z80_FLAG_F5); // copy bits to flag reg

	reset(); // done
}

void z80_instr_decoder::exec_main_q0() {
	switch (_z) {
	case 0b000:
		switch (_y) {
		case 0b000: // NOP
			reset();
			break;
		case 0b001: // EX AF, AF'
			swap(_regs.REG_AF, _regs.REG_AF_S);
			reset();
			break;
		case 0b010: // DJNZ d
			if (!_step) {
				_regs.REG_B--; // so we don't decrement B multiple times
				_ctx.start_bogus_cycle(1);
			}
			else exec_jr_stub(_regs.REG_B, 1); // take branch if B is non-zero
			break;
		case 0b011: // JR d (unconditional)
			exec_jr_stub();
			break;
		case 0b100: // JR NZ,d
			exec_jr_stub(!(_regs.REG_F & Z80_FLAG_Z));
			break;
		case 0b101: // JR Z,d
			exec_jr_stub(_regs.REG_F & Z80_FLAG_Z);
			break;
		case 0b110: // JR NC,d
			exec_jr_stub(!(_regs.REG_F & Z80_FLAG_C));
			break;
		case 0b111: // JR C,d
			exec_jr_stub(_regs.REG_F & Z80_FLAG_C);
			break;
		}
		break;
	case 0b001:
		if (_y & 1) exec_add_hl_r16(); // ADD HL,BC/DE/HL/SP
		else exec_ld_i16(); // LD BC/DE/HL/SP,nn
		break;
	case 0b010:
		if ((_y & 0b110) == 0b100) exec_ld16_p16(); // LD (nn),HL / LD HL,(nn) (determined by y bit 0)
		else exec_ld8_p16(); // LD (BC/DE/nn),A / LD A,(BC/DE/nn)
		break;
	case 0b011: // INC/DEC r16
		exec_incdec_r16();
		break;
	case 0b100: // INC r8
		exec_inc_r8();
		break;
	case 0b101: // DEC r8
		exec_dec_r8();
		break;
	case 0b110: // LD B/C/D/E/H/L/(HL)/A,n
		exec_ld_i8();
		break;
	case 0b111:
		switch (_y) {
		case 0b100: // DAA (adapted from https://ehaskins.com/2018-01-30%20Z80%20DAA/)
			_regs.REG_Z = _regs.REG_F; // use Z to store old flags (so we don't have to create another variable)
			_regs.REG_W = 0; // use W for correction value (same reason as above)
			_regs.REG_F &= Z80_FLAG_N;
			if ((_regs.REG_Z & Z80_FLAG_H) || (_regs.REG_A & 0xF) > 9)
				_regs.REG_W |= 0x6;
			if ((_regs.REG_Z & Z80_FLAG_C) || (_regs.REG_A > 0x99)) {
				_regs.REG_W |= 0x60;
				_regs.REG_F |= Z80_FLAG_C;
			}
			// if (_regs.REG_Z & Z80_FLAG_N) _regs.REG_A -= _regs.REG_W; else _regs.REG_A += _regs.REG_W;
			if (_regs.REG_Z & Z80_FLAG_N) {
				_regs.REG_F |= ((_regs.REG_A & 0x0F) < (_regs.REG_W & 0x0F)) << Z80_FLAGBIT_H;
				_regs.REG_A -= _regs.REG_W;
			}
			else {
				_regs.REG_F |= (bool)((_regs.REG_A & 0x0F) + (_regs.REG_W & 0x0F) & 0xF0) << Z80_FLAGBIT_H;
				_regs.REG_A += _regs.REG_W;
			}
			_regs.REG_F |=
				((bool)!_regs.REG_A << Z80_FLAGBIT_Z)
				| (_regs.REG_A & (Z80_FLAG_S | Z80_FLAG_F3 | Z80_FLAG_F5))
				| (parity(_regs.REG_A) << Z80_FLAGBIT_PV);
			break;
		case 0b101: // CPL
			_regs.REG_A ^= 0xFF;
			_regs.REG_F =
				(_regs.REG_F & ~(Z80_FLAG_F3 | Z80_FLAG_F5)) // erase F3 and F5 flags so we can copy them from A
				| (_regs.REG_A & (Z80_FLAG_F3 | Z80_FLAG_F5)) // copy bits 3 and 5 from A
				| Z80_FLAG_H | Z80_FLAG_N; // set H and N flags
			reset();
			break;
		case 0b110: // SCF
			_regs.REG_F =
				(_regs.REG_F & ~(Z80_FLAG_H | Z80_FLAG_N | Z80_FLAG_F3 | Z80_FLAG_F5))
				| (_regs.REG_A & (Z80_FLAG_F3 | Z80_FLAG_F5))
				| Z80_FLAG_C;
			reset();
			break;
		case 0b111: // CCF
			_regs.REG_F =
				(
					(_regs.REG_F & ~(Z80_FLAG_H | Z80_FLAG_N | Z80_FLAG_F3 | Z80_FLAG_F5))
					| (((_regs.REG_F >> Z80_FLAGBIT_C) & 1) << Z80_FLAGBIT_H)
					| (_regs.REG_A & (Z80_FLAG_F3 | Z80_FLAG_F5))
				)
				^ Z80_FLAG_C;
			reset();
			break;
		default: // RLCA/RRCA/RLA/RRA
			exec_shift_a();
			break;
		}
		break;
	}
}