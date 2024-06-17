#pragma once

#include "pch.h"

namespace llz80emu {
	/* Z80 CPU pin bit number (not corresponding to actual pin numbers!) */
	#define Z80_PIN_A_BASE						0 // A0..15
	#define Z80_PIN_A0							(Z80_PIN_A_BASE + 0)
	#define Z80_PIN_A1							(Z80_PIN_A_BASE + 1)
	#define Z80_PIN_A2							(Z80_PIN_A_BASE + 2)
	#define Z80_PIN_A3							(Z80_PIN_A_BASE + 3)
	#define Z80_PIN_A4							(Z80_PIN_A_BASE + 4)
	#define Z80_PIN_A5							(Z80_PIN_A_BASE + 5)
	#define Z80_PIN_A6							(Z80_PIN_A_BASE + 6)
	#define Z80_PIN_A7							(Z80_PIN_A_BASE + 7)
	#define Z80_PIN_A8							(Z80_PIN_A_BASE + 8)
	#define Z80_PIN_A9							(Z80_PIN_A_BASE + 9)
	#define Z80_PIN_A10							(Z80_PIN_A_BASE + 10)
	#define Z80_PIN_A11							(Z80_PIN_A_BASE + 11)
	#define Z80_PIN_A12							(Z80_PIN_A_BASE + 12)
	#define Z80_PIN_A13							(Z80_PIN_A_BASE + 13)
	#define Z80_PIN_A14							(Z80_PIN_A_BASE + 14)
	#define Z80_PIN_A15							(Z80_PIN_A_BASE + 15)
	#define Z80_PIN_D_BASE						16 // D0..7	
	#define Z80_PIN_D0							(Z80_PIN_D_BASE + 0)
	#define Z80_PIN_D1							(Z80_PIN_D_BASE + 1)
	#define Z80_PIN_D2							(Z80_PIN_D_BASE + 2)
	#define Z80_PIN_D3							(Z80_PIN_D_BASE + 3)
	#define Z80_PIN_D4							(Z80_PIN_D_BASE + 4)
	#define Z80_PIN_D5							(Z80_PIN_D_BASE + 5)
	#define Z80_PIN_D6							(Z80_PIN_D_BASE + 6)
	#define Z80_PIN_D7							(Z80_PIN_D_BASE + 7)
	#define Z80_PIN_M1							24
	#define Z80_PIN_MREQ						25
	#define Z80_PIN_IORQ						26
	#define Z80_PIN_RD							27
	#define Z80_PIN_WR							28
	#define Z80_PIN_RFSH						29
	#define Z80_PIN_HALT						30
	#define Z80_PIN_INT							31
	#define Z80_PIN_NMI							32
	#define Z80_PIN_WAIT						33
	#define Z80_PIN_BUSACK						34
	#define Z80_PIN_BUSREQ						35
	#define Z80_PIN_RESET						36
// #define Z80_PIN_CLK							37 // clock pin is handled separately

	/* Z80 CPU pin bitmask */
	typedef uint64_t z80_pinbits_t;
	#define Z80_A0								(1ULL << Z80_PIN_A0)
	#define Z80_A1								(1ULL << Z80_PIN_A1)
	#define Z80_A2								(1ULL << Z80_PIN_A2)
	#define Z80_A3								(1ULL << Z80_PIN_A3)
	#define Z80_A4								(1ULL << Z80_PIN_A4)
	#define Z80_A5								(1ULL << Z80_PIN_A5)
	#define Z80_A6								(1ULL << Z80_PIN_A6)
	#define Z80_A7								(1ULL << Z80_PIN_A7)
	#define Z80_A8								(1ULL << Z80_PIN_A8)
	#define Z80_A9								(1ULL << Z80_PIN_A9)
	#define Z80_A10								(1ULL << Z80_PIN_A10)
	#define Z80_A11								(1ULL << Z80_PIN_A11)
	#define Z80_A12								(1ULL << Z80_PIN_A12)
	#define Z80_A13								(1ULL << Z80_PIN_A13)
	#define Z80_A14								(1ULL << Z80_PIN_A14)
	#define Z80_A15								(1ULL << Z80_PIN_A15)
	#define Z80_D0								(1ULL << Z80_PIN_D0)
	#define Z80_D1								(1ULL << Z80_PIN_D1)
	#define Z80_D2								(1ULL << Z80_PIN_D2)
	#define Z80_D3								(1ULL << Z80_PIN_D3)
	#define Z80_D4								(1ULL << Z80_PIN_D4)
	#define Z80_D5								(1ULL << Z80_PIN_D5)	
	#define Z80_D6								(1ULL << Z80_PIN_D6)	
	#define Z80_D7								(1ULL << Z80_PIN_D7)
	#define Z80_M1								(1ULL << Z80_PIN_M1)
	#define Z80_MREQ							(1ULL << Z80_PIN_MREQ)
	#define Z80_IORQ							(1ULL << Z80_PIN_IORQ)
	#define Z80_RD								(1ULL << Z80_PIN_RD)
	#define Z80_WR								(1ULL << Z80_PIN_WR)
	#define Z80_RFSH							(1ULL << Z80_PIN_RFSH)
	#define Z80_HALT							(1ULL << Z80_PIN_HALT)
	#define Z80_INT								(1ULL << Z80_PIN_INT)
	#define Z80_NMI								(1ULL << Z80_PIN_NMI)
	#define Z80_WAIT							(1ULL << Z80_PIN_WAIT)
	#define Z80_BUSACK							(1ULL << Z80_PIN_BUSACK)
	#define Z80_BUSREQ							(1ULL << Z80_PIN_BUSREQ)
	#define Z80_RESET							(1ULL << Z80_PIN_RESET)
	// #define Z80_CLK								(1ULL << Z80_PIN_CLK)

	#define Z80_A_ALL							((1ULL << (Z80_PIN_A_BASE + 16)) - 1)
	#define Z80_D_ALL							(((1ULL << (Z80_PIN_D_BASE + 8)) - 1) & ~((1ULL << Z80_PIN_D_BASE) - 1))

	/* complete pin state (logic state and direction) */
	typedef struct {
		z80_pinbits_t state; // 1 = high, 0 = low
		z80_pinbits_t dir; // 1 = output, 0 = input
	} z80_pins_t;

	/* initial Z80 pin state (prior to POR) */
	const z80_pins_t Z80_PINS_INIT = {
		Z80_BUSACK | Z80_HALT | Z80_M1, // BUSACK, HALT and M1 high (TODO: verify with real HW)
		Z80_BUSACK | Z80_HALT | Z80_M1 // BUSACK, HALT and M1 will always be outputs
	};

	/* nominal operation Z80 pin state (ie. not releasing bus) */
	const z80_pins_t Z80_PINS_NOMINAL = {
		Z80_BUSACK | Z80_M1 | Z80_MREQ | Z80_IORQ | Z80_RD | Z80_WR | Z80_HALT | Z80_RFSH, // NOTE: HALT pin status must be added by the emulator, and we reset RFSH here since this line is only pulled low in T3 and T4 of fetch cycle
		Z80_BUSACK | Z80_M1 | Z80_MREQ | Z80_IORQ | Z80_RD | Z80_WR | Z80_A_ALL | Z80_HALT | Z80_RFSH // address lines outputting, data lines inputting (floating)
	};

	/* Z80 pin state during bus release */
	const z80_pins_t Z80_PINS_BUSREL = {
		Z80_M1 | Z80_HALT, // BUSACK low, M1 high (TODO: verify?), all other pins are floating (NOTE: HALT pin status must be added by the emulator)
		Z80_BUSACK | Z80_M1 | Z80_HALT
	};
}