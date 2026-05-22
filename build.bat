@echo off
echo building cpu vm...

gcc -std=c11 -O2 -Wall -Wno-unused-variable -Wno-unused-but-set-variable -o cpu.exe src/main.c src/state.c src/executor.c src/assembler.c src/disassembler.c src/compiler.c src/decompiler.c -lm 2>build.log

if errorlevel 1 (
    echo build failed - check build.log
    type build.log
    exit /b 1
)

echo build complete: cpu.exe
echo.
echo usage:
echo   cpu.exe test                      - run tests
echo   cpu.exe run file.vm               - execute bytecode
echo   cpu.exe asm file.asm file.vm      - assemble
echo   cpu.exe compile file.vm file.exe  - compile to native
echo.
