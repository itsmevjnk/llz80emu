#pragma once

#include "pins.h"
#include "registers.h"
#include "cycle.h"
#include "instr_decoder.h"

/* dllexport/dllimport macro for Windows */
#if !defined(LLZ80EMU_API) // allow overriding

#if (defined(_WIN32) || defined(_WIN64) || defined(__CYGWIN__) || defined(__MINGW32__) || defined(__MINGW64__)) && (defined(LLZ80EMU_BUILD_SHARED) || defined(LLZ80EMU_USE_SHARED))
#if defined(LLZ80EMU_BUILD_SHARED) // compiling DLL
#define LLZ80EMU_API __declspec(dllexport)
#else // consuming DLL
#define LLZ80EMU_API __declspec(dllimport)
#endif
#else
#define LLZ80EMU_API // dllexport/dllimport not needed
#endif

#endif

namespace llz80emu {
	class z80emu {
	public:
		LLZ80EMU_API z80emu(bool clk);

		LLZ80EMU_API void set_clkpin(bool state); // set the clock pin state (without clocking)
		LLZ80EMU_API z80_pins_t clock(z80_pinbits_t state); // clock the CPU by one half-cycle (rising edge or falling edge)

		LLZ80EMU_API z80_pins_t get_pins(); // get pins without clocking
		LLZ80EMU_API z80_registers_t get_regs(); // get registers
		LLZ80EMU_API void set_regs(const z80_registers_t& regs); // set registers

		LLZ80EMU_API void trigger_nmi(); // trigger NMI pin (to be called on NMI falling edge)

		/* cycle transition methods - not supposed to be called by library consumer! */
		void start_fetch_cycle(bool halt = false);
		void start_mem_read_cycle(uint16_t addr, uint8_t& val_out);
		void start_mem_write_cycle(uint16_t addr, uint8_t val);
		void start_io_read_cycle(uint16_t addr, uint8_t& val_out);
		void start_io_write_cycle(uint16_t addr, uint8_t val);
		void start_bogus_cycle(int cycles);
		void start_intack_cycle(uint8_t& val_out);
		// z80_cycle* const& cycle = _cycle;

		void skip_nmi_handling();
		bool is_nmi_pending() const;

		void skip_int_handling();
		bool is_int_pending() const;
	private:
		bool _clkpin = false; // clock pin state (true = high, false = low) - this is synchronised with the RESET signal
		bool _por = false; // whether power-on reset has been triggered in the CPU's lifetime

		z80_registers_t _regs; // registers
		z80_pins_t _pins = Z80_PINS_INIT; // pins (for feeding into cycles)

		/* pre-allocated cycle state objects to avoid dynamic allocation */
		z80_fetch_cycle _fetch_cycle; // fetch cycle
		z80_mem_read_cycle _mem_read_cycle; // memory read cycle
		z80_mem_write_cycle _mem_write_cycle; // memory write cycle
		z80_io_read_cycle _io_read_cycle; // I/O read cycle
		z80_io_write_cycle _io_write_cycle; // I/O write cycle
		z80_bogus_cycle _bogus_cycle; // bogus cycle
		z80_intack_cycle _intack_cycle; // interrupt acknowledgment cycle

		z80_cycle* _cycle = nullptr; // current cycle (null indicating we're in reset and so nothing can proceed)

		z80_instr_decoder _instr; // instruction decoder and executor

		bool _intpin = false; // sampled state of INT pin (true = active = INT low)
		bool _int_skip = false; // set to skip interrupt handling for the current instruction (for emulating EI behaviour)
		bool _int_pending = false; // set when handling INT (cleared once we're out of the interrupt acknowledgment process)

		bool _nmiff = false; // state of the NMI flip-flop (true = active)
		bool _nmi_skip = false; // set to skip NMI handling (for emulating NONI)
		bool _nmi_pending = false; // set when NMI flip-flop activity has been acknowledged, but the interrupt is not serviced yet (ie. doing bogus fetch + PC stack pushes)
	
		int _reset_cycles = 0; // number of cycles that RESET has been held low
		bool _reset_m1t2 = false; // set if RESET was asserted on M1T2 rising edge (possibly special reset) - this will be confirmed with _reset_cycles
	};
}
