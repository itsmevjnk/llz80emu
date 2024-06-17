cmake_minimum_required(VERSION 3.7)

project(
	llz80emu
	DESCRIPTION "Low (signal) level cycle-accurate Z80 CPU emulator"
	LANGUAGES CXX
)

add_library(
	llz80emu_static STATIC
	z80emu.cpp
	cycle.cpp fetch_cycle.cpp rw_cycle_base.cpp mem_cycle.cpp io_cycle.cpp bogus_cycle.cpp
	z80emu.h cycle.h pch.h pins.h registers.h
)
target_include_directories(llz80emu_static PUBLIC .)

add_library(
	llz80emu SHARED
	z80emu.cpp
	cycle.cpp fetch_cycle.cpp rw_cycle_base.cpp mem_cycle.cpp io_cycle.cpp bogus_cycle.cpp
	z80emu.h cycle.h pch.h pins.h registers.h
)
target_include_directories(llz80emu PUBLIC .)