#include "pch.h"
#include "cycle.h"

using namespace llz80emu;

z80_intack_cycle::z80_intack_cycle(z80_pins_t& pins, z80_registers_t& regs) : z80_cycle(pins), _regs(regs) {

}

void z80_intack_cycle::reset(uint8_t& val_out) {
	z80_cycle::reset();
	_out = &val_out;
}

bool z80_intack_cycle::clock(bool clk) {
	z80_cycle::clock(clk); // call this first to increment our T cycle

	int t_half = (_t << 1) | !clk; // T half-cycle
	switch (t_half) {
	case 0: // T1 high
		_pins = Z80_PINS_NOMINAL; // reset pins to nominal state
		_pins.state =
			(_pins.state & ~(Z80_A_ALL | Z80_M1)) // clear all address lines and M1 pin
			| ((z80_pinbits_t)_regs.REG_PC << Z80_PIN_A_BASE); // set address lines to PC
		break;
	case 1: // T1 low
		break; // nothing to do here
	case 2: // T2 high
		break; // nothing to do here
	case 3: // T2 low
		break; // nothing to do here
	case 4: // TW1 high
		break; // nothing to do here
	case 5: // TW1 low
		_pins.state &= ~Z80_IORQ; // pull IORQ low to signal to interrupt peripheral
		break;
	case 6: // TW2 high
		break; // nothing to do here
	case 7: // TW2 low
		break; // nothing to do here (WAIT pin sampling will be done in the next T half)
	case 8: // T3 high
		_wait = !(_pins.state & Z80_WAIT); // sample WAIT pin (true = WAIT state activated)
		if (_wait) _t--; // stay in TW2
		else {
			*_out = (uint8_t)((_pins.state & Z80_D_ALL) >> Z80_PIN_D_BASE); // sample Dx pins
			_pins.state =
				(_pins.state | Z80_IORQ) // set IORQ
				& ~(Z80_RFSH | Z80_A_ALL) // clear RFSH and address lines
				| ((z80_pinbits_t)_regs.REG_IR << Z80_PIN_A_BASE); // and set address bus to I + R (refresh reg)
		}
		break;
	case 9: // T3 low
		_regs.REG_R = (_regs.REG_R + 1) & 0x7F; // increment refresh address, masking the MSB off
		_pins.state &= ~Z80_MREQ; // pull MREQ low for refresh
		break;
	case 10: // T4 high
		sample_busreq();
		break;
	case 11: // T4 low
		_pins.state |= Z80_MREQ; // set MREQ (ending refresh)
		// address line and RFSH must be reset by the next cycle
		if (!_bus_release) return true;
		break;
	default:
		if (!handle_bus_release(clk)) throw std::runtime_error("Invalid T cycle - no transition has occurred from interrupt acknowledgment cycle?");
		else if (!clk && !_bus_release) return true;
		break;
	}

	return false;
}