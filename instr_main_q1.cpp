#include "pch.h"
#include "instr_decoder.h"

using namespace llz80emu;

void z80_instr_decoder::exec_main_q1() {
	if(_regs.instr == 0x76) {
		if (_mod == Z80_MOD_NONE) {
			// HALT
			_regs.REG_PC--;
			reset(true);
			return;
		}
		else {
			// undefined opcode
			reset();
			return;
		}
	}

	/* decode source and destination register */
	const uint8_t* src = reg8(_z); uint8_t* dst = reg8(_y);
	assert(src || dst); // at least one must be non-null
	if (!src || !dst) {
		/* two-step operation: initiate memory read/write on step=0, then return to fetching on step=1 */
		if (!_step) {
			if (!process_hlptr()) return; // _hl_ptr isn't ready yet
			if(!src) _ctx.start_mem_read_cycle(_hl_ptr, *reg8_nomod(_y)); // read from (HL) - if (HL) is already used then L/H won't be replaced with IXL/IXH or IYL/IYH
			else _ctx.start_mem_write_cycle(_hl_ptr, *reg8_nomod(_z)); // write to (HL)
			return;
		}
	} else *dst = *src; // copy from source to destination (normal business, takes a single step)

	reset(); // return to fetching
}