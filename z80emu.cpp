#include "pch.h"
#include "z80emu.h"

using namespace llz80emu;

z80emu::z80emu(bool clk) : _clkpin(clk), _fetch_cycle(_pins, _regs), _mem_read_cycle(_pins), _mem_write_cycle(_pins), _io_read_cycle(_pins), _io_write_cycle(_pins), _bogus_cycle(_pins, _regs), _intack_cycle(_pins, _regs), _instr(*this, _regs) {

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
		memset(&_regs, 0, sizeof(_regs)); _regs.REG_SP = _regs.REG_AF = 0xFFFF; // reset registers
	}
	else if (_clkpin && _por && !_cycle) {
		/* rising edge and still in reset - get out of reset now (and also synchronise with clock pin) */
		start_fetch_cycle();
	}

	if (_cycle) {
		if (_clkpin) _intpin = !(_pins.state & Z80_INT); // sample INT pin

		/* operate cycle */
		if (_cycle->clock(_clkpin)) {
			/* cycle has finished */
			if (!_instr.started()) _instr.start(); // exiting fetch/interrupt acknowledgment cycle - start decoding and executing new instruction
			else _instr.next_step(); // run next step of instruction execution

			if (!_instr.started()) {
				/* instruction execution complete */

				if (_nmiff && !_nmi_skip) {
					/* NMI triggered */
					_regs.iff2 = _regs.iff1; _regs.iff1 = false; // disable interrupt while keeping former IFF1 state in IFF2
					_nmiff = false; _nmi_pending = true; // clear NMI flip-flop (so it can be re-activated at some other point), then stage NMI servicing
					if (!(_pins.state & Z80_HALT)) _regs.REG_PC++; // if we're halting and an interrupt occurred, we'll need to bring ourselves out of the HALT instruction
					return _pins; // after this, a fetch cycle will be issued as normal, but it won't be followed by a normal instruction decode/execution
				}

				if (_intpin && _regs.iff1 && !_int_skip) {
					/* INT triggered and can be accepted */
					_regs.iff1 = false; // disable interrupt
					if (!_regs.int_mode) start_intack_cycle(_regs.instr); // mode 0: read to instruction ptr (this will be handled as normal)
					else { // mode 1/2
						start_intack_cycle(_regs.REG_Z); // read to Z (mode 1 can ignore, mode 2 can use this to calculate vector)
						_int_pending = true; // mark as handling INT so instr_decoder can work on the rest
						if (!(_pins.state & Z80_HALT)) _regs.REG_PC++; // if we're halting and an interrupt occurred, we'll need to bring ourselves out of the HALT instruction
						// mode 1: extra clock cycle + push PC + jump to 0x0038
						// mode 2: extra clock cycle + push PC + read new PC from vector
					}
					return _pins;
				}

				_nmi_skip = _int_skip = false;
			}
		}
	}

	return _pins;
}

z80_pins_t z80emu::get_pins() {
	return _pins;
}

void z80emu::trigger_nmi() {
	_nmiff = true;
}

void z80emu::start_fetch_cycle(bool halt) {
	_int_pending = _nmi_pending = false; // now that we're back to normal operation
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

void z80emu::start_intack_cycle(uint8_t& val_out) {
	_intack_cycle.reset(val_out);
	_cycle = &_intack_cycle;
}

z80_registers_t z80emu::get_regs() {
	return _regs;
}

void z80emu::set_regs(const z80_registers_t& regs) {
	_regs = regs;
}

bool z80emu::is_nmi_pending() const {
	return _nmi_pending;
}

void z80emu::skip_int_handling() {
	_int_skip = true;
}

void z80emu::skip_nmi_handling() {
	_nmi_skip = true;
}

bool z80emu::is_int_pending() const {
	return _int_pending;
}