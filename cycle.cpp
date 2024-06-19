#include "pch.h"
#include "cycle.h"

using namespace llz80emu;

z80_cycle::z80_cycle(z80_pins_t& pins) : _pins(pins) {

}

void z80_cycle::reset() {
	_t = -1; // upon next clock, we will increment this to 0
}

bool z80_cycle::clock(bool clk) {
	if (clk) _t++; // increment T cycle
	return true; // always return true, as we don't know if the cycle has finished
}

void z80_cycle::sample_busreq() {
	_bus_release = !(_pins.state & Z80_BUSREQ);
}

bool z80_cycle::handle_bus_release(bool clk) {
	if (clk) {
		if (!_bus_release) return false; // bus release was not staged

		sample_busreq(); // resample BUSREQ for next cycle
		_pins = Z80_PINS_BUSREL;
	}
	return true;
}