#include "instr_decoder.h"
#include "z80emu.h"

using namespace llz80emu;

z80_instr_decoder::z80_instr_decoder(z80emu& ctx, z80_registers_t& regs) : _ctx(ctx), _regs(regs) {
	//uint8_t* r8[] = { &regs.REG_B, &regs.REG_C, &regs.REG_D, &regs.REG_E, &regs.REG_H, &regs.REG_L, nullptr /* (HL) */, &regs.REG_A };
	//memcpy(_reg8, r8, sizeof(r8));
	//uint16_t* r16[] = { &regs.REG_BC, &regs.REG_DE, &regs.REG_HL, &regs.REG_SP };
	//memcpy(_reg16, r16, sizeof(r16));
	//r16[3] = &regs.REG_AF; // difference between _reg16 and _reg16_alt
	//memcpy(_reg16_alt, r16, sizeof(r16));
}

void z80_instr_decoder::start() {
	if (!_ctx.is_nmi_pending()) { // only care about the instruction register if it's not an NMI going on
		if (_subset == Z80_SUBSET_NONE) {
			/* take in prefixes */
			switch (_regs.instr) {
			case 0xDD:
				_mod = Z80_MOD_DD;
				_ctx.start_fetch_cycle();
				_ctx.skip_int_handling(); _ctx.skip_nmi_handling(); // skip all interrupts
				return;
			case 0xFD:
				_mod = Z80_MOD_FD;
				_ctx.start_fetch_cycle();
				_ctx.skip_int_handling(); _ctx.skip_nmi_handling();
				return;
			case 0xCB:
				_subset = Z80_SUBSET_CB;
				if (_mod == Z80_MOD_NONE) {
					/* no mod prefixes - continue on as usual */
					_ctx.start_fetch_cycle();
				}
				else {
					/* DD/FD prefix - read d offset, then perform pseudo opcode fetch */
					if (!_mod_d_ready) process_hlptr(0, false); // read displacement byte - we'll defer the HL pointer calculation after the pseudo opcode fetch
					else if (!_mod_cb_fetched) {
						_ctx.start_mem_read_cycle(_regs.REG_PC++, _mod_cb_instr); // pseudo opcode fetch
						_mod_cb_fetched = true;
					}
					else if (process_hlptr(2)) { // 2 extra clock cycles following pseudo opcode fetch
						_regs.instr = _mod_cb_instr;
						break;
					}
				}
				_ctx.skip_int_handling(); _ctx.skip_nmi_handling();
				return;
			case 0xED:
				_subset = Z80_SUBSET_ED;
				_mod = Z80_MOD_NONE; // 0xED prefix disregards 0xDD/FD prefixes
				_ctx.start_fetch_cycle();
				_ctx.skip_int_handling(); _ctx.skip_nmi_handling();
				return;
			}
		}

		//_fetch = false; // just in case
		_x = (_regs.instr & 0b11000000) >> 6; // decode opcode into octals
		_y = (_regs.instr & 0b00111000) >> 3;
		_z = (_regs.instr & 0b00000111);
	}

	_step = 0; // reset step counter
	_started = true;
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
			_ctx.start_mem_write_cycle(--_regs.REG_SP, _regs.REG_PCH);
			break;
		case 2:
			_ctx.start_mem_write_cycle(--_regs.REG_SP, _regs.REG_PCL);
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
			_ctx.start_mem_write_cycle(--_regs.REG_SP, _regs.REG_PCH);
			break;
		case 2:
			_ctx.start_mem_write_cycle(--_regs.REG_SP, _regs.REG_PCL);
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
	case Z80_SUBSET_CB:
		exec_cb();
		break;
	case Z80_SUBSET_ED:
		exec_ed();
		break;
	default:
		reset();
		break;
	}

end:
	_step++;
}

void z80_instr_decoder::reset(bool halt) {
	_subset = Z80_SUBSET_NONE; _mod = Z80_MOD_NONE;
	_mod_d_ready = _hlptr_ready = _mod_cb_fetched = false;
	_started = false;
	_ctx.start_fetch_cycle(halt);
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

void z80_instr_decoder::exec_ed() {
	switch (_x) {
	case 0b01:
		exec_ed_q1();
		break;
	case 0b10:
		exec_ed_q2();
		break;
	default:
		reset();
		break;
	}
}

bool z80_instr_decoder::process_hlptr(int extra_cycles, bool set_hlptr_ready) {
	if (_mod == Z80_MOD_NONE) {
		/* no modifiers - HL is HL */
		_hl_ptr = _regs.REG_HL;
		return true;
	}

	if (!_mod_d_ready) {
		/* displacement byte hasn't been read */
		_ctx.start_mem_read_cycle(_regs.REG_PC++, *((uint8_t*)&_mod_d)); // cast _mod_d from int8_t to uint8_t
		_step--; // go back by 1 step so the next next_step() call will be landed back to where we are
		_mod_d_ready = true; // it'll be ready
		return false;
	}

	if (!_hlptr_ready) {
		/* displacement byte has just been read */
		_hl_ptr = ((_mod == Z80_MOD_DD) ? _regs.REG_IX : _regs.REG_IY) + _mod_d; // calculate IX+d / IY+d
		if (set_hlptr_ready) _hlptr_ready = true;
		if (!extra_cycles) return true; // no extra cycles required
		_ctx.start_bogus_cycle(extra_cycles); // extra cycles for calculating address
		_step--;
		return false;
	}
	
	return true;
}

bool z80_instr_decoder::started() const {
	return _started;
}
