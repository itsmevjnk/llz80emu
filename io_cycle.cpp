#include "cycle.h"
#include <stdexcept>

using namespace llz80emu;

/* I/O read */

z80_io_read_cycle::z80_io_read_cycle(z80_pins_t& pins) : z80_read_cycle(pins) {

}

bool z80_io_read_cycle::clock(bool clk) {
	z80_cycle::clock(clk);

	int t_half = (_t << 1) | !clk; // T half-cycle
	switch (t_half) {
	case 0: // T1 high
		_pins = Z80_PINS_NOMINAL; // reset pins to nominal state
		_pins.state =
			(_pins.state & ~Z80_A_ALL) // clear all address lines
			| ((z80_pinbits_t)_addr << Z80_PIN_A_BASE); // set address lines to the address we want to read from (NOTE: we can actually put a 16-bit address in here)
		break;
	case 1: // T1 low
		break; // nothing to do here
	case 2: // T2 high
		_pins.state &= ~(Z80_IORQ | Z80_RD); // start I/O read
		break;
	case 3: // T2 low
		break; // nothing to do here (WAIT and Dx pin sampling are to be done at the start of T3 high)
	case 4: // TW high (implicit wait state)
		break;
	case 5: // TW low (implicit wait state)
		break;
	case 6: // T3 high
		_wait = !(_pins.state & Z80_WAIT); // sample WAIT pin (true = WAIT state activated)
		if (_wait) _t--; // stay in T2
		sample_busreq();
		break;
	case 7: // T3 low
		*_val_out = (uint8_t)((_pins.state & Z80_D_ALL) >> Z80_PIN_D_BASE); // sample Dx pins
		_pins.state |= Z80_IORQ | Z80_RD; // stop I/O read
		if (!_bus_release) return true;
		break;
	default:
		if (!handle_bus_release(clk)) throw std::runtime_error("Invalid T cycle - no transition has occurred from I/O read cycle?");
		else if (!clk && !_bus_release) return true;
		break;
	}

	return false;
}

z80_io_write_cycle::z80_io_write_cycle(z80_pins_t& pins) : z80_write_cycle(pins) {

}

bool z80_io_write_cycle::clock(bool clk) {
	z80_cycle::clock(clk);

	int t_half = (_t << 1) | !clk; // T half-cycle
	switch (t_half) {
	case 0: // T1 high
		_pins = Z80_PINS_NOMINAL; // reset pins to nominal state
		_pins.state =
			(_pins.state & ~Z80_A_ALL) // clear all address lines
			| ((z80_pinbits_t)_addr << Z80_PIN_A_BASE); // set address lines to the address we want to read from (NOTE: we can actually put a 16-bit address in here)
		break;
	case 1: // T1 low
		_pins.dir |= Z80_D_ALL; // start driving data lines
		_pins.state =
			(_pins.state & ~Z80_D_ALL) // clear all data pins
			| ((z80_pinbits_t)_val << Z80_PIN_D_BASE); // set data pins to the value we want to write
		break;
	case 2: // T2 high
		_pins.state &= ~(Z80_IORQ | Z80_WR); // start I/O write
		break;
	case 3: // T2 low
		break; // nothing to do here (WAIT and Dx pin sampling are to be done at the start of T3 high)
	case 4: // TW high (implicit wait state)
		break;
	case 5: // TW low (implicit wait state)
		break;
	case 6: // T3 high
		_wait = !(_pins.state & Z80_WAIT); // sample WAIT pin (true = WAIT state activated)
		if (_wait) _t--; // stay in T2
		sample_busreq();
		break;
	case 7: // T3 low
		_pins.state |= Z80_IORQ | Z80_WR; // stop I/O write
		if (!_bus_release) return true;
		break;
	default:
		if (!handle_bus_release(clk)) throw std::runtime_error("Invalid T cycle - no transition has occurred from I/O write cycle?");
		else if (!clk && !_bus_release) return true;
		break;
	}

	return false;
} 