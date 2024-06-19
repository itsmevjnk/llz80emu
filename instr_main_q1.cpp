#include "pch.h"
#include "instr_decoder.h"

using namespace llz80emu;

void z80_instr_decoder::exec_main_q1() {
	if(_regs.instr == 0x76) {
		// HALT
		_ctx.start_fetch_cycle(true);
		return;
	}

	/* decode source and destination register */
	const uint8_t* src = _reg8[_z]; uint8_t* dst = _reg8[_y];
	assert(src || dst); // at least one must be non-null
	if (!src || !dst) {
		/* two-step operation: initiate memory read/write on step=0, then return to fetching on step=1 */
		if (!_step) {
			if(!src) _ctx.start_mem_read_cycle(_regs.REG_HL, *dst); // read from (HL)
			else _ctx.start_mem_write_cycle(_regs.REG_HL, *src); // write to (HL)
			return;
		}
	} else *dst = *src; // copy from source to destination (normal business, takes a single step)

	reset(); // return to fetching
}