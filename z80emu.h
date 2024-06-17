#pragma once

#include "pch.h"

/* dllexport/dllimport macro for Windows */
#if !defined(LLZ80EMU_API) // allow overriding

#if defined(LLZ80EMU_BUILD_DLL) // compiling DLL
#define LLZ80EMU_API __declspec(dllexport)
#elif !defined(LLZ80EMU_BUILD_STATIC) && (defined(_WIN32) || defined(_WIN64) || defined(__CYGWIN__) || defined(__MINGW32__) || defined(__MINGW64__)) // consuming DLL
#define LLZ80EMU_API __declspec(dllimport)
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

		z80_cycle* _cycle = nullptr; // current cycle (null indicating we're in reset and so nothing can proceed)
	};
}
