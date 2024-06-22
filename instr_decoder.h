#pragma once

//#include "z80emu.h"
#include "registers.h"

namespace llz80emu {
	class z80emu;

	class z80_instr_decoder {
	public:
		z80_instr_decoder(z80emu& ctx, z80_registers_t& regs);

		void start(); // start decoding and executing the instruction stored in _regs
		void reset(bool halt = false); // stop instruction execution and start fetch cycle
		void next_step(); // transition to next step or end execution and go back to fetching

		bool started() const; // return whether instruction execution has started (as opposed to still awaiting prefix and stuff)
	private:
		bool _started = false;

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

		/* DD/FD prefix displacement byte */
		int8_t _mod_d = 0;
		uint16_t _hl_ptr = 0; // HL/IX+d/IY+d, depending on modifier prefix (initialised by process_hlptr())
		bool _mod_d_ready = false; // set when the displacement byte has been read
		bool _hlptr_ready = false; // set when _hl_ptr is ready
		uint8_t _mod_cb_instr = 0; // instruction byte following DDCB/FDCB+d
		bool _mod_cb_fetched = false; // set when the pseudo opcode fetch following DDCB/FDCB+d has been staged
		bool process_hlptr(int extra_cycles = 5, bool set_hlptr_ready = true); // return false if the current exec step is to be stopped immediately after this (i.e. to read displacement byte); otherwise, _hl_ptr will contain the pointer for use in (HL)

		uint8_t _x = 0xFF, _y = 0xFF, _z = 0xFF; // broken down parts of the opcode (xx yyy zzz)

		/* instruction executor helpers */
		//uint8_t* _reg8[8]; // 8-bit registers (used by main quadrant 1 and 2)
		//uint16_t* _reg16[4]; // 16-bit registers (used by some main quadrant 0 instructions)
		//uint16_t* _reg16_alt[4]; // 16-bit registers (used by PUSH and POP instructions)

		inline uint8_t* reg8_nomod(uint8_t idx) {
			switch (idx) {
			case 0: return &_regs.REG_B;
			case 1: return &_regs.REG_C;
			case 2: return &_regs.REG_D;
			case 3: return &_regs.REG_E;
			case 4: return &_regs.REG_H;
			case 5: return &_regs.REG_L;
			case 7: return &_regs.REG_A;
			default: return nullptr;
			}
		}

		inline uint8_t* reg8(uint8_t idx) {
			switch (idx) {
			case 0: return &_regs.REG_B;
			case 1: return &_regs.REG_C;
			case 2: return &_regs.REG_D;
			case 3: return &_regs.REG_E;
			case 7: return &_regs.REG_A;
			case 4:
				switch (_mod) {
				case Z80_MOD_NONE: return &_regs.REG_H;
				case Z80_MOD_DD: return &_regs.REG_IXH;
				case Z80_MOD_FD: return &_regs.REG_IYH;
				default: return nullptr;
				}
				break;
			case 5:
				switch (_mod) {
				case Z80_MOD_NONE: return &_regs.REG_L;
				case Z80_MOD_DD: return &_regs.REG_IXL;
				case Z80_MOD_FD: return &_regs.REG_IYL;
				default: return nullptr;
				}
				break;
			default: return nullptr;
			}
		}

		inline uint16_t* reg16_nomod(uint8_t idx) {
			switch (idx) {
			case 0: return &_regs.REG_BC;
			case 1: return &_regs.REG_DE;
			case 2: return &_regs.REG_HL;
			case 3: return &_regs.REG_SP;
			default: return nullptr;
			}
		}

		inline uint16_t* reg16(uint8_t idx) {
			switch (idx) {
			case 0: return &_regs.REG_BC;
			case 1: return &_regs.REG_DE;
			case 2:
				switch (_mod) {
				case Z80_MOD_NONE: return &_regs.REG_HL;
				case Z80_MOD_DD: return &_regs.REG_IX;
				case Z80_MOD_FD: return &_regs.REG_IY;
				default: return nullptr;
				}
				break;
			case 3: return &_regs.REG_SP;
			default: return nullptr;
			}
		}

		inline uint16_t* reg16_alt_nomod(uint8_t idx) {
			switch (idx) {
			case 0: return &_regs.REG_BC;
			case 1: return &_regs.REG_DE;
			case 2: return &_regs.REG_HL;
			case 3: return &_regs.REG_AF;
			default: return nullptr;
			}
		}

		inline uint16_t* reg16_alt(uint8_t idx) {
			switch (idx) {
			case 0: return &_regs.REG_BC;
			case 1: return &_regs.REG_DE;
			case 2:
				switch (_mod) {
				case Z80_MOD_NONE: return &_regs.REG_HL;
				case Z80_MOD_DD: return &_regs.REG_IX;
				case Z80_MOD_FD: return &_regs.REG_IY;
				default: return nullptr;
				}
				break;
			case 3: return &_regs.REG_AF;
			default: return nullptr;
			}
		}

		inline uint8_t parity(uint8_t x) {
			x ^= x >> 4; // 4/4
			x ^= x >> 2; // 2/2
			x ^= x >> 1; // 1/1
			return (~x) & 1;
		}

		inline void swap(uint16_t& a, uint16_t& b) {
			uint16_t t = a; a = b; b = t;
		}

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
		void exec_alu_stub(); // run ALU operations with operand in Z register and operation selector in _y (for sharing with main quadrant 3)

		/* main quadrant 3 (xx = 11) */
		void exec_main_q3();
		void exec_cond_ret();
		void exec_uncond_ret();
		void exec_jp(bool cond);
		void exec_call(bool cond);
		void exec_push();
		void exec_pop();
		void exec_io_i8(bool out, uint8_t& reg); // IN r8,(n) / OUT (n),r8
		void exec_rst();
		void exec_ex_stack_hl();

		void exec_ed(); // ED prefix subset

		/* ED quadrant 1 (xx = 01) */
		void exec_ed_q1();
		void exec_io_r8(bool out);
		void exec_adc_sbc_hl_r16();
		void exec_ld_r16_p16();
		void exec_ld_ir(); // LD I,A / LD R,A / LD A,I / LD A,R
		void exec_bcd_rotate(); // RRD / RLD

		/* ED quadrant 2 (xx = 10) - block transfer operations */
		void exec_ed_q2();
		void exec_blk_ld(); // LDI/LDD/LDIR/LDDR 
		void exec_blk_cp(); // CPI/CPD/CPIR/CPDR
		void exec_blk_in(); // INI/IND/INIR/INDR
		void exec_blk_out(); // OUTI/OUTD/OTIR/OTDR

		void exec_cb(); // CB prefix subset (all contained in instr_cb.cpp)
		void exec_shift_rot(); // CB quadrant 0
		void exec_bit(); // CB quadrant 1
		void exec_res(); // CB quadrant 2
		void exec_set(); // CB quadrant 3
	};
}

