#pragma once

#include "pins.h"
#include "registers.h"

namespace llz80emu {
	class z80_cycle {
	public:
		z80_cycle(z80_pins_t& pins);
		const int& t = _t; // T cycle number (constant - for access by z80emu)

		virtual bool clock(bool clk); // clock the CPU by one half-cycle (rising edge or falling edge) - this will be called by z80emu::clock(), and will return true if the cycle has finished
	protected:
		void reset(); // prepare cycle instance for invocation
		z80_pins_t& _pins; // the pins of the Z80 CPU
		int _t = -1; // T cycle number

		/* bus release handling (not needed for bogus cycles) */
		void sample_busreq(); // to be called on rising edge of last T cycle
		bool handle_bus_release(bool clk); // to be called on T cycles after the last one; return false if there was nothing to be done
		bool _bus_release = false; // set if the CPU is staged to release the bus on the next T cycle
	};

	class z80_fetch_cycle : public z80_cycle {
	public:
		z80_fetch_cycle(z80_pins_t& pins, z80_registers_t& regs);

		void reset(bool halt);
		bool clock(bool clk) override;
	private:
		z80_registers_t& _regs; // CPU registers
		bool _wait = false; // set if there's a WAIT state to be inserted in the next cycle (i.e. long T2)
		bool _halt = false; // set if the CPU is performing a HALT instruction
	};

	class z80_read_cycle : public z80_cycle {
	public:
		z80_read_cycle(z80_pins_t& pins);

		void reset(uint16_t addr, uint8_t& val_out);
	protected:
		bool _wait = false; // set if there's a WAIT state to be inserted in the next cycle (i.e. long T2)
		uint16_t _addr; // address to read from
		uint8_t* _val_out = nullptr; // value to read into
	};

	class z80_write_cycle : public z80_cycle {
	public:
		z80_write_cycle(z80_pins_t& pins);

		void reset(uint16_t addr, uint8_t val);
	protected:
		bool _wait = false; // set if there's a WAIT state to be inserted in the next cycle (i.e. long T2)
		uint16_t _addr; // address to read from
		uint8_t _val; // value to write
	};

	class z80_mem_read_cycle : public z80_read_cycle {
	public:
		z80_mem_read_cycle(z80_pins_t& pins);
		bool clock(bool clk) override;
	};

	class z80_mem_write_cycle : public z80_write_cycle {
	public:
		z80_mem_write_cycle(z80_pins_t& pins);
		bool clock(bool clk) override;
	};

	class z80_io_read_cycle : public z80_read_cycle {
	public:
		z80_io_read_cycle(z80_pins_t& pins);
		bool clock(bool clk) override;
	};

	class z80_io_write_cycle : public z80_write_cycle {
	public:
		z80_io_write_cycle(z80_pins_t& pins);
		bool clock(bool clk) override;
	};

	//typedef void (*z80_bogus_cycle_cb_t)(z80_registers_t& regs, z80_pins_t& pins); // callback for bogus cycle - called on the last half of the last cycle (used to implement instructions' quirks)
	class z80_bogus_cycle : public z80_cycle {
	public:
		z80_bogus_cycle(z80_pins_t& pins, z80_registers_t& regs);

		void reset(int cycles);
		bool clock(bool clk) override;
	private:
		z80_registers_t& _regs; // CPU registers
		int _cycles = 0; // number of cycles remaining
		//z80_bogus_cycle_cb_t* _last_half_cb = nullptr; // callback for the last half of the last cycle (null = no callback)
	};

	class z80_intack_cycle : public z80_cycle {
	public:
		z80_intack_cycle(z80_pins_t& pins, z80_registers_t& regs);

		void reset(uint8_t& val_out);
		bool clock(bool clk) override;
	private:
		z80_registers_t& _regs; // CPU registers
		uint8_t* _out = nullptr; // register to save data bus output
		bool _wait = false; // set if there's a WAIT state to be inserted in the next cycle (i.e. long TW2)
	};
}