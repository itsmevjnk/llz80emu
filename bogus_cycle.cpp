#include "cycle.h"

using namespace llz80emu;

z80_bogus_cycle::z80_bogus_cycle(z80_pins_t& pins, z80_registers_t& regs) : z80_cycle(pins), _regs(regs) {

}

void z80_bogus_cycle::reset(int cycles) {
	_cycles = cycles;
	//_last_half_cb = last_half_cb;
}

bool z80_bogus_cycle::clock(bool clk) {
	z80_cycle::clock(clk);

	if (!clk) {
		_cycles--;
		if (_cycles == 0) {
			/* falling edge of last cycle */
			//if (_last_half_cb) (*_last_half_cb)(_regs, _pins);
			return true;
		}
	}

	return false;
}