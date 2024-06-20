#include "pch.h"
#include "instr_decoder.h"

using namespace llz80emu;

static inline bool check_branch_condition(uint8_t flags, uint8_t y) {
	switch (y) {
	case 0b000: return !(flags & Z80_FLAG_Z);
	case 0b001: return (flags & Z80_FLAG_Z);
	case 0b010: return !(flags & Z80_FLAG_C);
	case 0b011: return (flags & Z80_FLAG_C);
	case 0b100: return !(flags & Z80_FLAG_PV);
	case 0b101: return (flags & Z80_FLAG_PV);
	case 0b110: return !(flags & Z80_FLAG_S);
	case 0b111: return (flags & Z80_FLAG_S);
	default: return false;
	}
}

void z80_instr_decoder::exec_cond_ret() {
	switch (_step) {
	case 0:
		_ctx.start_bogus_cycle(1); // 1 clock cycle before branching (possibly to check condition?)
		break;
	case 1:
		if (!check_branch_condition(_regs.REG_F, _y)) reset(); // not taking branch
		else _ctx.start_io_read_cycle(_regs.REG_SP++, _regs.REG_PCL);
		break;
	case 2:
		_ctx.start_io_read_cycle(_regs.REG_SP++, _regs.REG_PCH);
		break;
	default:
		reset(); // done
		break;
	}
}

void z80_instr_decoder::exec_uncond_ret() {
	switch (_step) {
	case 0:
		_ctx.start_io_read_cycle(_regs.REG_SP++, _regs.REG_PCL);
		break;
	case 1:
		_ctx.start_io_read_cycle(_regs.REG_SP++, _regs.REG_PCH);
		break;
	default:
		reset();
		break;
	}
}

void z80_instr_decoder::exec_jp(bool cond) {
	switch (_step) {
	case 0:
		_ctx.start_mem_read_cycle(_regs.REG_PC++, _regs.REG_Z); // read low byte
		break;
	case 1:
		_ctx.start_mem_read_cycle(_regs.REG_PC++, _regs.REG_W); // read high byte
		break;
	default:
		if (!cond || check_branch_condition(_regs.REG_F, _y)) _regs.REG_PC = _regs.REG_WZ; // do the branch
		reset();
		break;
	}
}

void z80_instr_decoder::exec_call(bool cond) {
	switch (_step) {
	case 0:
		_ctx.start_mem_read_cycle(_regs.REG_PC++, _regs.REG_Z); // read low byte
		break;
	case 1:
		_ctx.start_mem_read_cycle(_regs.REG_PC++, _regs.REG_W); // read high byte
		break;
	case 2:
		if (!cond || check_branch_condition(_regs.REG_F, _y)) _ctx.start_bogus_cycle(1); // insert 1 extra cycle if we take the branch
		else reset();
		break;
	case 3:
		_ctx.start_io_write_cycle(--_regs.REG_SP, _regs.REG_PCH); // push high byte of PC
		break;
	case 4:
		_ctx.start_io_write_cycle(--_regs.REG_SP, _regs.REG_PCL); // push low byte of PC
		break;
	default:
		_regs.PC = _regs.WZ; // finally branch
		reset();
		break;
	}
}

void z80_instr_decoder::exec_push() {
	switch (_step) {
	case 0:
		_ctx.start_bogus_cycle(1); // insert 1 extra clock cycle before doing our thing
		break;
	case 1:
		_ctx.start_io_write_cycle(--_regs.REG_SP, *HB_PTR(_reg16_alt[_y >> 1])); // push high byte first
		break;
	case 2:
		_ctx.start_io_write_cycle(--_regs.REG_SP, *LB_PTR(_reg16_alt[_y >> 1])); // then the low byte
		break;
	default:
		reset();
		break;
	}
}

void z80_instr_decoder::exec_pop() {
	switch (_step) {
	case 0:
		_ctx.start_io_read_cycle(_regs.REG_SP++, *LB_PTR(_reg16_alt[_y >> 1]));
		break;
	case 1:
		_ctx.start_io_read_cycle(_regs.REG_SP++, *HB_PTR(_reg16_alt[_y >> 1]));
		break;
	default:
		reset();
		break;
	}
}

void z80_instr_decoder::exec_io_i8(bool out, uint8_t& reg) {
	switch (_step) {
	case 0:
		_ctx.start_mem_read_cycle(_regs.REG_PC++, _regs.REG_Z); // read address to Z
		break;
	case 1:
		if (out) _ctx.start_io_write_cycle(_regs.REG_Z, reg);
		else _ctx.start_io_read_cycle(_regs.REG_Z, reg);
		break;
	default:
		reset();
		break;
	}
}

void z80_instr_decoder::exec_rst() {
	switch (_step) {
	case 0:
		_ctx.start_bogus_cycle(1);
		break;
	case 1:
		_ctx.start_io_write_cycle(--_regs.REG_SP, _regs.REG_PCH); // push high byte of PC
		break;
	case 2:
		_ctx.start_io_write_cycle(--_regs.REG_SP, _regs.REG_PCL); // push low byte of PC
		break;
	case 3:
		_regs.REG_PC = _y << 3; // set PC to the selected vector
		reset();
		break;
	}
}

void z80_instr_decoder::exec_ex_stack_hl() {
	switch (_step) {
	case 0: // first pop to WZ
		_ctx.start_io_read_cycle(_regs.REG_SP++, _regs.REG_Z);
		break;
	case 1:
		_ctx.start_io_read_cycle(_regs.REG_SP++, _regs.REG_W);
		break;
	case 2: // then push HL to stack
		_ctx.start_io_write_cycle(--_regs.REG_SP, _regs.REG_H);
		break;
	case 3:
		_ctx.start_io_write_cycle(--_regs.REG_SP, _regs.REG_L);
		break;
	default:
		_regs.REG_HL = _regs.REG_WZ; // do the exchange
		reset();
		break;
	}
}

void z80_instr_decoder::exec_main_q3() {
	switch (_z) {
	case 0b000: // RET cc
		exec_cond_ret();
		break;
	case 0b001:
		if (!(_y & 1)) exec_pop(); // POP BC/DE/HL/AF
		else switch (_y >> 1) {
		case 0b00: // RET
			exec_uncond_ret();
			break;
		case 0b01: // EXX
			swap(_regs.REG_BC, _regs.REG_BC_S);
			swap(_regs.REG_DE, _regs.REG_DE_S);
			swap(_regs.REG_HL, _regs.REG_HL_S);
			reset();
			break;
		case 0b10: // JP HL
			_regs.REG_PC = _regs.REG_HL;
			reset();
			break;
		case 0b11: // LD SP, HL
			if (!_step) _ctx.start_bogus_cycle(2); // 2 bogus cycles following opcode fetch
			else {
				_regs.REG_SP = _regs.REG_HL;
				reset();
			}
			break;
		}
		break;
	case 0b010: // JP cc,nn
		exec_jp(true);
		break;
	case 0b011:
		switch (_y) {
		case 0b000: // JP nn
			exec_jp(false);
			break;
		case 0b010: // OUT (n),A
			exec_io_i8(true, _regs.REG_A);
			break;
		case 0b011: // IN A
			exec_io_i8(false, _regs.REG_A);
			break;
		case 0b100: // EX (SP),HL
			exec_ex_stack_hl();
			break;
		case 0b101: // EX DE,HL
			swap(_regs.REG_DE, _regs.REG_HL);
			reset();
			break;
		case 0b110: // DI
			_regs.iff1 = _regs.iff2 = false;
			reset();
			break;
		case 0b111: // EI
			_regs.iff1 = _regs.iff2 = true;
			_ctx.skip_int_handling(); // defer interrupt sampling/handling until the next instruction
			reset();
			break;
		default:
			reset();
			break;
		}
		break;
	case 0b100: // CALL cc,nn
		exec_call(true);
		break;
	case 0b101:
		if (!(_y & 1)) exec_push(); // PUSH BC/DE/HL/AF
		else exec_call(false); // CALL nn (_y = 0b000) - the other options are prefix bytes which are already parsed
		break;
	case 0b110: // ALU operation on i8 - too short for its own method
		if (!_step) _ctx.start_mem_read_cycle(_regs.REG_PC++, _regs.REG_Z); // read next byte into Z
		else exec_alu_stub();
		break;
	case 0b111: // RST
		exec_rst();
		break;
	}
}