#include "pch.h"
#include "cycle.h"

using namespace llz80emu;

/* read cycle base class methods */

z80_read_cycle::z80_read_cycle(z80_pins_t& pins) : z80_cycle(pins) {

}

void z80_read_cycle::reset(uint16_t addr, uint8_t& val_out) {
	z80_cycle::reset();
	_addr = addr;
	_val_out = &val_out;
	_wait = false;
}

/* write cycle base class methods */

z80_write_cycle::z80_write_cycle(z80_pins_t& pins) : z80_cycle(pins) {

}

void z80_write_cycle::reset(uint16_t addr, uint8_t val) {
	z80_cycle::reset();
	_addr = addr;
	_val = val;
	_wait = false;
}
