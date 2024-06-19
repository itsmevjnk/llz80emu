#pragma once

#include "pch.h"

/* macro for getting pointer of low/high byte of a 16-bit register given its pointer */
#if __BYTE_ORDER__ == __ORDER_LITTLE_ENDIAN__
#define LB_PTR(p)				((uint8_t*)((uintptr_t)(p) + 0))
#define HB_PTR(p)				((uint8_t*)((uintptr_t)(p) + 1))
#else
#define LB_PTR(p)				((uint8_t*)((uintptr_t)(p) + 1))
#define HB_PTR(p)				((uint8_t*)((uintptr_t)(p) + 0))
#endif

namespace llz80emu {
	/* 8-bit register pair */
#pragma pack(push, 1)
	typedef union {
		struct {
#if __BYTE_ORDER__ == __ORDER_LITTLE_ENDIAN__
			uint8_t lo;
			uint8_t hi;
#else
			uint8_t hi;
			uint8_t lo;
#endif
		} bytes;
		uint16_t word;
	} z80_regpair_t;
#pragma pack(pop)

	typedef struct {
		/* main registers */
#define REG_AF					AF.word
#define REG_A					AF.bytes.hi	
#define REG_F					AF.bytes.lo
		z80_regpair_t AF;
#define REG_BC					BC.word
#define REG_B					BC.bytes.hi	
#define REG_C					BC.bytes.lo
		z80_regpair_t BC;
#define REG_DE					DE.word
#define REG_D					DE.bytes.hi	
#define REG_E					DE.bytes.lo
		z80_regpair_t DE;
#define REG_HL					HL.word
#define REG_H					HL.bytes.hi	
#define REG_L					HL.bytes.lo
		z80_regpair_t HL;

		/* shadow registers */
#define REG_AF_S				AF_s.word
#define REG_A_S					AF_s.bytes.hi	
#define REG_F_S					AF_s.bytes.lo
		z80_regpair_t AF_s;
#define REG_BC_S				BC_s.word
#define REG_B_S					BC_s.bytes.hi	
#define REG_C_S					BC_s.bytes.lo
		z80_regpair_t BC_s;
#define REG_DE_S				DE_s.word
#define REG_D_S					DE_s.bytes.hi	
#define REG_E_S					DE_s.bytes.lo
		z80_regpair_t DE_s;
#define REG_HL_S				HL_s.word
#define REG_H_S					HL_s.bytes.hi	
#define REG_L_S					HL_s.bytes.lo
		z80_regpair_t HL_s;

		/* index registers */
#define REG_IX					IX.word
#define REG_IXH					IX.bytes.hi
#define REG_IXL					IX.bytes.lo
		z80_regpair_t IX;
#define REG_IY					IY.word
#define REG_IYH					IY.bytes.hi
#define REG_IYL					IY.bytes.lo
		z80_regpair_t IY;

		/* stack pointer */
#define REG_SP					SP.word
#define REG_SPH					SP.bytes.hi
#define REG_SPL					SP.bytes.lo
		z80_regpair_t SP;

		/* program counter */
#define REG_PC					PC.word
#define REG_PCH					PC.bytes.hi
#define REG_PCL					PC.bytes.lo
		z80_regpair_t PC;

		/* other registers */
#define REG_IR					IR.word
#define REG_I					IR.bytes.hi
#define REG_R					IR.bytes.lo
		z80_regpair_t IR;
#define REG_WZ					WZ.word
#define REG_W					WZ.bytes.hi
#define REG_Z					WZ.bytes.lo
		z80_regpair_t WZ;

		/* instruction fetch */
		uint8_t instr; // last instruction byte
		// TODO: prefixes
	} z80_registers_t;

	/* flag bits */
	#define Z80_FLAGBIT_S			7
	#define Z80_FLAGBIT_Z			6
	#define Z80_FLAGBIT_F5			5
	#define Z80_FLAGBIT_H			4
	#define Z80_FLAGBIT_F3			3
	#define Z80_FLAGBIT_PV			2
	#define Z80_FLAGBIT_N			1
	#define Z80_FLAGBIT_C			0

	/* flag bitmasks */
	#define Z80_FLAG_S				(1 << Z80_FLAGBIT_S)
	#define Z80_FLAG_Z				(1 << Z80_FLAGBIT_Z)
	#define Z80_FLAG_F5				(1 << Z80_FLAGBIT_F5)
	#define Z80_FLAG_H				(1 << Z80_FLAGBIT_H)
	#define Z80_FLAG_F3				(1 << Z80_FLAGBIT_F3)
	#define Z80_FLAG_PV				(1 << Z80_FLAGBIT_PV)
	#define Z80_FLAG_N				(1 << Z80_FLAGBIT_N)
	#define Z80_FLAG_C				(1 << Z80_FLAGBIT_C)
}
