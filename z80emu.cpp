#include "pch.h"
#include "z80emu.h"

using namespace llz80emu;

z80emu::z80emu(bool clk) : _clkpin(clk), _fetch_cycle(_pins, _regs), _mem_read_cycle(_pins), _mem_write_cycle(_pins), _io_read_cycle(_pins), _io_write_cycle(_pins), _bogus_cycle(_pins, _regs), _instr(*this, _regs) {

}

void z80emu::set_clkpin(bool state) {
	_clkpin = state;
}

z80_pins_t z80emu::clock(z80_pinbits_t state) {
	_clkpin = !_clkpin; // toggle clock pin

	_pins.state = (_pins.state & _pins.dir) | (state & ~_pins.dir); // update pin state (only replacing input pin bits)
	if (!(_pins.state & Z80_RESET)) {
		/* reset pin pulled low */
		_por = true;
		_cycle = nullptr; // stop current cycle
		_pins = Z80_PINS_INIT; // reset pins
	}
	else if (_clkpin && _por && !_cycle) {
		/* rising edge and still in reset - get out of reset now (and also synchronise with clock pin) */
		start_fetch_cycle();
	}

	if (_cycle) {
		/* operate cycle */
		if (_cycle->clock(_clkpin)) {
			/* cycle has finished */
			if (dynamic_cast<z80_fetch_cycle*>(_cycle)) _instr.start(); // exiting fetch cycle - start decoding and executing new instruction
			else _instr.next_step(); // run next step of instruction execution
		}
	}

	return _pins;
}

z80_pins_t z80emu::get_pins() {
	return _pins;
}

void z80emu::start_fetch_cycle(bool halt) {
	_fetch_cycle.reset(halt);
	_cycle = &_fetch_cycle;
}

void z80emu::start_mem_read_cycle(uint16_t addr, uint8_t& val_out) {
	_mem_read_cycle.reset(addr, val_out);
	_cycle = &_mem_read_cycle;
}

void z80emu::start_mem_write_cycle(uint16_t addr, uint8_t val) {
	_mem_write_cycle.reset(addr, val);
	_cycle = &_mem_write_cycle;
}

void z80emu::start_io_read_cycle(uint16_t addr, uint8_t& val_out) {
	_io_read_cycle.reset(addr, val_out);
	_cycle = &_io_read_cycle;
}

void z80emu::start_io_write_cycle(uint16_t addr, uint8_t val) {
	_io_write_cycle.reset(addr, val);
	_cycle = &_io_write_cycle;
}

void z80emu::start_bogus_cycle(int cycles) {
	_bogus_cycle.reset(cycles);
	_cycle = &_bogus_cycle;
}

z80_registers_t z80emu::get_regs() {
	return _regs;
}

void z80emu::set_regs(const z80_registers_t& regs) {
	_regs = regs;
}