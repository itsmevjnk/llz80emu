#include "cycle.h"
#include <stdexcept>

using namespace llz80emu;

z80_fetch_cycle::z80_fetch_cycle(z80_pins_t& pins, z80_registers_t& regs) : z80_cycle(pins), _regs(regs) {

}

void z80_fetch_cycle::reset(bool halt) {
	z80_cycle::reset();
	_halt = halt;
}

bool z80_fetch_cycle::clock(bool clk) {
	z80_cycle::clock(clk); // call this first to increment our T cycle

	int t_half = (_t << 1) | !clk; // T half-cycle
	switch (t_half) {
	case 0: // T1 high
		_pins = Z80_PINS_NOMINAL; // reset pins to nominal state
		_pins.state =
			(_pins.state & ~(Z80_A_ALL | Z80_M1)) // clear all address lines and M1 pin
			| ((z80_pinbits_t)_regs.REG_PC << Z80_PIN_A_BASE); // set address lines to PC
		if (_halt) _pins.state &= ~Z80_HALT; // clear HALT if we're in a HALT state
		break;
	case 1: // T1 low
		_pins.state &= ~(Z80_MREQ | Z80_RD); // start memory read
		break;
	case 2: // T2 high
		break; // nothing to do here
	case 3: // T2 low
		break; // nothing to do here (WAIT and Dx pin sampling are to be done at the start of T3 high)
	case 4: // T3 high
		_wait = !(_pins.state & Z80_WAIT); // sample WAIT pin (true = WAIT state activated)
		if (_wait) _t--; // stay in T2
		else {
			if (!_halt) _regs.instr = (uint8_t)((_pins.state & Z80_D_ALL) >> Z80_PIN_D_BASE); // sample Dx pins and store them in the instruction register (only if we're not halting)
			else _regs.instr = 0x00; // continue halting (by executing NOPs)
			_pins.state =
				((_pins.state | (Z80_MREQ | Z80_RD | Z80_M1)) // set MREQ, RD, M1
				& ~(Z80_RFSH | Z80_A_ALL)) // clear RFSH and address lines
				| ((z80_pinbits_t)_regs.REG_IR << Z80_PIN_A_BASE); // and set address bus to I + R (refresh reg)
		}
		break;
	case 5: // T3 low
		_regs.REG_R = (_regs.REG_R + 1) & 0x7F; // increment refresh address, masking the MSB off
		_pins.state &= ~Z80_MREQ; // pull MREQ low for refresh
		break;
	case 6: // T4 high
		sample_busreq();
		break;
	case 7: // T4 low
		_pins.state =
			(_pins.state | Z80_MREQ) //  set MREQ (ending refresh)
			& ~(0xFF << Z80_PIN_A_BASE); // clear low address lines (seems to be unexplained)
		if (!_halt) _regs.REG_PC++;
		// address line and RFSH must be reset by the next cycle
		if (!_bus_release) return true;
		break;
	default:
		if (!handle_bus_release(clk)) throw std::runtime_error("Invalid T cycle - no transition has occurred from fetch cycle?");
		else if (!clk && !_bus_release) return true;
		break;
	}

	return false;
}