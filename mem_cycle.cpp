#include "cycle.h"

#if !defined(NO_EXCEPTIONS)
#include <stdexcept>
#endif

using namespace llz80emu;

/* memory read */

z80_mem_read_cycle::z80_mem_read_cycle(z80_pins_t& pins) : z80_read_cycle(pins, Z80_MEM_READ_CYCLE) {

}

bool z80_mem_read_cycle::clock(bool clk) {
	z80_cycle::clock(clk);

	int t_half = (_t << 1) | !clk; // T half-cycle
	switch (t_half) {
	case 0: // T1 high
		_pins = Z80_PINS_NOMINAL; // reset pins to nominal state
		_pins.state =
			(_pins.state & ~Z80_A_ALL) // clear all address lines
			| ((z80_pinbits_t)_addr << Z80_PIN_A_BASE); // set address lines to the address we want to read from
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
		sample_busreq();
		break;
	case 5: // T3 low
		*_val_out = (uint8_t)((_pins.state & Z80_D_ALL) >> Z80_PIN_D_BASE); // sample Dx pins
		_pins.state |= Z80_MREQ | Z80_RD; // stop memory read
		if (!_bus_release) return true;
		break;
	default:
		if (!handle_bus_release(clk))
#if defined(NO_EXCEPTIONS)
			abort();
#else
			throw std::runtime_error("Invalid T cycle - no transition has occurred from memory read cycle?");
#endif
		else if (!clk && !_bus_release) return true;
		break;
	}

	return false;
}

z80_mem_write_cycle::z80_mem_write_cycle(z80_pins_t& pins) : z80_write_cycle(pins, Z80_MEM_WRITE_CYCLE) {

}

bool z80_mem_write_cycle::clock(bool clk) {
	z80_cycle::clock(clk);

	int t_half = (_t << 1) | !clk; // T half-cycle
	switch (t_half) {
	case 0: // T1 high
		_pins = Z80_PINS_NOMINAL; // reset pins to nominal state
		_pins.state =
			(_pins.state & ~Z80_A_ALL) // clear all address lines
			| ((z80_pinbits_t)_addr << Z80_PIN_A_BASE); // set address lines to the address we want to write to
		break;
	case 1: // T1 low
		_pins.dir |= Z80_D_ALL; // start driving data lines
		_pins.state =
			(_pins.state & ~(Z80_MREQ | Z80_D_ALL)) // clear data lines and pull MREQ low (but don't start memory write yet, hence WR is kept high)
			| ((z80_pinbits_t)_val << Z80_PIN_D_BASE); // set data lines to the data we want to write
		break;
	case 2: // T2 high
		break; // nothing to do here
	case 3: // T2 low
		_pins.state &= ~Z80_WR; // start memory write
		break;
	case 4: // T3 high
		_wait = !(_pins.state & Z80_WAIT); // sample WAIT pin (true = WAIT state activated)
		if (_wait) _t--; // stay in T2
		sample_busreq();
		break;
	case 5: // T3 low
		_pins.state |= Z80_MREQ | Z80_WR; // stop memory write
		if (!_bus_release) return true;
		break;
	default:
		if (!handle_bus_release(clk))
#if defined(NO_EXCEPTIONS)
			abort();
#else
			throw std::runtime_error("Invalid T cycle - no transition has occurred from memory write cycle?");
#endif
		else if (!clk && !_bus_release) return true;
		break;
	}

	return false;
}