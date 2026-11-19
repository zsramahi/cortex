# Cortex

A virtual CPU with 512 opcodes, 256 general-purpose 32-bit registers, and 1MB flat memory. Ships with a full toolchain: assembler, disassembler, compiler, decompiler, and executor.

## Usage

```
cortex run <file.vm>              - execute bytecode
cortex asm <file.asm> <out.vm>    - assemble to bytecode
cortex disasm <file.vm> <out.asm> - disassemble bytecode
cortex compile <file.vm> <out>    - compile to self-extracting binary
cortex decompile <file> <out.asm> - extract and disassemble
cortex test                       - run built-in tests
```

**Build (Linux)**
```bash
./build.sh
```

**Build (Windows)**
```
build.bat
```

Requires `gcc` with C11 support.

## Purpose

Proof of concept. Exploring how far a single-binary VM can go with a fixed-width instruction set, from basic arithmetic up through signal processing, linear algebra, cryptography, and procedural generation, all packed into 512 opcodes with no external dependencies at runtime.
