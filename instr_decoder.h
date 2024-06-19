#pragma once

namespace llz80emu {
	class z80emu;

	class z80_instr_decoder {
	public:
		z80_instr_decoder(z80emu& ctx, z80_registers_t& regs);

		void start(); // start decoding and executing the instruction stored in _regs
		void reset(); // stop instruction execution and start fetch cycle
		void next_step(); // transition to next step or end execution and go back to fetching
	private:
		z80emu& _ctx; // context (for state transitions)
		z80_registers_t& _regs; // CPU registers

		int _step = 0; // execution step
		enum {
			Z80_SUBSET_NONE, // no prefixes
			Z80_SUBSET_CB, // CB prefix
			Z80_SUBSET_ED, // ED prefix
		} _subset = Z80_SUBSET_NONE; // instruction subset
		enum {
			Z80_MOD_NONE, // no modifier
			Z80_MOD_DD, // DD prefix (IX)
			Z80_MOD_FD, // FD prefix (IY)
		} _mod = Z80_MOD_NONE; // modifier prefix

		uint8_t _x = 0xFF, _y = 0xFF, _z = 0xFF; // broken down parts of the opcode (xx yyy zzz)

		/* instruction executor helpers */
		uint8_t* _reg8[8]; // 8-bit registers (used by main quadrant 1 and 2)
		uint16_t* _reg16[4]; // 16-bit registers (used by some main quadrant 0 instructions)

		/* instruction executors */

		void exec_main(); // main instruction subset (no prefixes)

		/* main quadrant 0 (xx = 00) */
		void exec_main_q0();
		void exec_inc_r8();
		void exec_dec_r8();
		void exec_incdec_r16();
		void exec_ld16_p16(); // also shared with ED quadrant 1
		void exec_ld8_p16();
		void exec_ld_i16();
		void exec_add_hl_r16();
		void exec_ld_i8();
		void exec_jr_stub(bool take_branch = true); // JR/DJNZ stub (read displacement byte to Z, then perform relative jump if take_branch is true)
		void exec_shift_a();

		/* main quadrant 1 (xx = 01) - LD and HALT */
		void exec_main_q1();

		/* main quadrant 2 (xx = 10) - ALU operations */
		void exec_main_q2();
	};
}

