cmake_minimum_required(VERSION 3.7)

project(
	llz80emu
	DESCRIPTION "Low (signal) level cycle-accurate Z80 CPU emulator"
	LANGUAGES CXX
)

add_library(
	llz80emu_static STATIC
	z80emu.cpp
	cycle.cpp fetch_cycle.cpp rw_cycle_base.cpp mem_cycle.cpp io_cycle.cpp bogus_cycle.cpp intack_cycle.cpp
	instr_decoder.cpp instr_main_q0.cpp instr_main_q1.cpp instr_main_q2.cpp instr_main_q3.cpp instr_ed_q1.cpp instr_ed_q2.cpp instr_cb.cpp
	z80emu.h cycle.h pins.h registers.h instr_decoder.h
)
target_include_directories(llz80emu_static PUBLIC .)

add_library(
	llz80emu SHARED
	z80emu.cpp
	cycle.cpp fetch_cycle.cpp rw_cycle_base.cpp mem_cycle.cpp io_cycle.cpp bogus_cycle.cpp intack_cycle.cpp
	instr_decoder.cpp instr_main_q0.cpp instr_main_q1.cpp instr_main_q2.cpp instr_main_q3.cpp instr_ed_q1.cpp instr_ed_q2.cpp instr_cb.cpp
	z80emu.h cycle.h pins.h registers.h instr_decoder.h
)
target_include_directories(llz80emu PUBLIC .)