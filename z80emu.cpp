#include "pch.h"
#include "z80emu.h"

using namespace llz80emu;

z80emu::z80emu(bool clk) : _clkpin(clk), _fetch_cycle(_pins, _regs), _mem_read_cycle(_pins), _mem_write_cycle(_pins), _io_read_cycle(_pins), _io_write_cycle(_pins), _bogus_cycle(_pins, _regs) {

}

void z80emu::set_clkpin(bool state) {
	_clkpin = state;
}

z80_pins_t z80emu::clock(z80_pinbits_t state) {
	_clkpin = !_clkpin; // toggle clock pin

	if (!(_pins.state & Z80_RESET)) {
		/* reset pin pulled low */
		_por = true;
		_cycle = nullptr; // stop current cycle
		_pins = Z80_PINS_INIT; // reset pins
	}
	else if (_clkpin && _por && !_cycle) {
		/* rising edge and still in reset - get out of reset now (and also synchronise with clock pin) */
		_fetch_cycle.reset(false); // reset fetch cycle, not halting
		_cycle = &_fetch_cycle; // run fetch cycle
	}

	if (_cycle) {
		/* operate cycle */
		_pins.state = state; // update pin state
		if (_cycle->clock(_clkpin)) {
			/* cycle has finished */
		}
	}

	return _pins;
}
