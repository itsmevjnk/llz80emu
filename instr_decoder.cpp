#include "pch.h"
#include "instr_decoder.h"

using namespace llz80emu;

z80_instr_decoder::z80_instr_decoder(z80emu& ctx, z80_registers_t& regs) : _ctx(ctx), _regs(regs) {
	uint8_t* r8[] = { &regs.REG_B, &regs.REG_C, &regs.REG_D, &regs.REG_E, &regs.REG_H, &regs.REG_L, nullptr /* (HL) */, &regs.REG_A };
	memcpy(_reg8, r8, sizeof(r8));
	uint16_t* r16[] = { &regs.REG_BC, &regs.REG_DE, &regs.REG_HL, &regs.REG_SP };
	memcpy(_reg16, r16, sizeof(r16));
	r16[3] = &regs.REG_AF; // difference between _reg16 and _reg16_alt
	memcpy(_reg16_alt, r16, sizeof(r16));
}

void z80_instr_decoder::start() {
	if (!_ctx.is_nmi_pending()) { // only care about the instruction register if it's not an NMI going on
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

		//_fetch = false; // just in case
		_x = (_regs.instr & 0b11000000) >> 6; // decode opcode into octals
		_y = (_regs.instr & 0b00111000) >> 3;
		_z = (_regs.instr & 0b00000111);
	}

	_step = 0; // reset step counter
	next_step(); // start execution
}

void z80_instr_decoder::next_step() {
	if (_ctx.is_nmi_pending()) {
		/* NMI handling - after opcode fetch */
		switch (_step) {
		case 0:
			_regs.REG_PC--; // decrement PC again to position ourselves at the instruction we just ignored
			_ctx.start_bogus_cycle(1);
			break;
		case 1: // push current PC into stack
			_ctx.start_io_write_cycle(--_regs.REG_SP, _regs.REG_PCH);
			break;
		case 2:
			_ctx.start_io_write_cycle(--_regs.REG_SP, _regs.REG_PCL);
			break;
		default: // restart at 0x66
			_regs.REG_PC = 0x0066;
			reset();
			break;
		}

		goto end;
	}

	else if (_ctx.is_int_pending()) {
		/* INT handling - after acknowledgment */
		switch (_step) {
		case 0:
			_ctx.start_bogus_cycle(1); // one extra cycle
			break;
		case 1: // push current PC into stack
			_ctx.start_io_write_cycle(--_regs.REG_SP, _regs.REG_PCH);
			break;
		case 2:
			_ctx.start_io_write_cycle(--_regs.REG_SP, _regs.REG_PCL);
			break;
		case 3:
			if (_regs.int_mode == 1) {
				/* mode 1 - jump to 0x0038 */
				_regs.REG_PC = 0x0038;
				reset();
			} else {
				/* mode 2 - calculate new vector and read PC from there */
				_regs.REG_W = _regs.REG_I; // Z contains the vector's low byte, so now WZ stores the address to read PC from
				_ctx.start_mem_read_cycle(_regs.REG_WZ + 0, _regs.REG_PCL);
			}
			break;
		case 4: // mode 2 only - read high byte of PC
			_ctx.start_mem_read_cycle(_regs.REG_WZ + 1, _regs.REG_PCH);
			break;
		default:
			reset();
			break;
		}

		goto end;
	}

	/* normal execution */
	switch (_subset) {
	case Z80_SUBSET_NONE:
		exec_main();
		break;
	default:
		reset();
		break;
	}

end:
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
	case 0b11:
		exec_main_q3();
		break;
	default:
		reset();
		break;
	}
}
