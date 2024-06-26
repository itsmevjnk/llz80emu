# llz80emu

`llz80emu` is a clock-accurate Zilog Z80 emulator written in C++.

The emulator's behaviour is modeled using information from the following sources:
* [Z80 netlist simulation details](https://floooh.github.io/2021/12/06/z80-instruction-timing.html)
* [Z80 opcode table](https://clrhome.org/table/)
* [Z80 flag affection](http://www.z80.info/z80sflag.htm)
* [Documentation on the undocumented MEMPTR register](https://gist.github.com/drhelius/8497817)
* [The Undocumented Z80 Documented by Sean Young](https://gist.github.com/drhelius/8497817)
* [David Bank's documentation on undocumented flag behaviour](https://github.com/hoglet67/Z80Decoder/wiki/Undocumented-Flags)

To maintain emulation performance and simplicity, the inner operation (ie. during instruction execution) is not accurately modeled; however, the emulator should have the same number of cycles and behaviour for instructions as a real Z80 CPU.

The emulator passes [raddad772's Z80 unit tests](https://github.com/SingleStepTests/z80) (and the former [JSMoo](https://github.com/raddad772/jsmoo)), albeit with [some instruction timing discrepancies](https://github.com/SingleStepTests/z80/issues/3). The emulator also passes all `zexall` tests.

## Installation

The emulator can be compiled using Visual Studio with the provided solution file, as well as with CMake.

## Usage

`llz80emu` is provided as a library; ie. a frontend is required to do anything useful with it.

The emulator provides the following publicly accessible methods in the `z80emu` class:
* `z80emu::z80emu(bool clk)`: Instantiate a Z80 emulator object with the specified initial CLK pin state.
* `void z80emu::set_clkpin(bool state)`: Set the CLK pin state without triggering the CPU's operation.
* `z80_pins_t z80emu::clock(z80_pinbits_t state)`: Emulate the CPU on a state transition (rising/falling edge) of the CLK pin. Input pins' states are to be provided through the `state` argument, and the method returns the new pins' states and directions (see `pins.h`).
* `void z80emu::trigger_nmi()`: Trigger the NMI pin on the CPU. This method is to be called on the falling edge of the NMI pin.
* `z80_pins_t z80emu::get_pins()`: Retrieve the emulator's pins' states and directions, without clocking the CPU.
* `z80_registers_t z80emu::get_regs()`: Retrieve the emulator's register values.
* `void z80emu::set_regs(const z80_registers_t& regs)`: Set the emulator's register values.

## Contributing

Pull requests and discussions/bug reports through [Issues](https://github.com/itsmevjnk/llz80emu/issues) are welcome.

## License

[MIT](https://choosealicense.com/licenses/mit/)