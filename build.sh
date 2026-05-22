#!/bin/bash
set -e

echo "Building CPU VM..."

gcc -std=c11 -O2 -Wall -Wno-unused-variable -Wno-unused-but-set-variable \
    -o cpu \
    src/main.c src/state.c src/executor.c src/assembler.c \
    src/disassembler.c src/compiler.c src/decompiler.c \
    -lm 2>&1 | tee build.log

if [ ${PIPESTATUS[0]} -ne 0 ]; then
    echo "Build failed - check build.log"
    exit 1
fi

echo "Build complete: cpu"
echo ""
echo "Usage:"
echo "  ./cpu test                      - run tests"
echo "  ./cpu run file.vm               - execute bytecode"
echo "  ./cpu asm file.asm file.vm      - assemble"
echo "  ./cpu compile file.vm file.exe  - compile to native"
echo ""
