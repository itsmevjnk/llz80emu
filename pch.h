#pragma once

/* standard types */
#include <stddef.h>
#include <stdint.h>

#include <stdexcept>
#include <assert.h>
#include <string.h>

/* our classes and stuff */
#include "pins.h"
#include "registers.h"
#include "cycle.h"
#include "instr_decoder.h"
#include "z80emu.h"

/* for MSVC */
#define WIN32_LEAN_AND_MEAN
