
# cRISC-V

This is an excercise in C to implement a RISC-V RV32I (and possibly extensions) emulator in plain C. It is aimed at being correct, (very) fast, and easy to understand.

NOTE: Currently under heavy development, expect everything to change.

## Features

### Current
- Full 5-stage pipeline
- RV32I `Base Instruction Set` fully implemented
- Shared bus with mmio support
- RAM support
- ELF loading
- Basic trap handling

### Missing
- CSR support
  There is no support at all for CSR

### Future
- `M` Standard Extension for Integer Multiplication and Division
  This is kind of a basic feature, so I think this will be implemented
