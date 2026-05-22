#pragma once
#include <stdint.h>
#include <stddef.h>

#define MEMSIZE 1048576
#define REGCOUNT 256
#define STACKBASE (MEMSIZE - 4)
#define SENTINEL 0xFFFFFFFF
#define MARKER0 0xDE
#define MARKER1 0xAD
#define MARKER2 0xBE
#define MARKER3 0xEF
#define MARKER4 0xCA
#define MARKER5 0xFE
#define MARKER6 0xBA
#define MARKER7 0xBE

typedef struct {
    uint8_t* memory;
    uint32_t registers[REGCOUNT];
    uint32_t pc;
    uint32_t sp;
    uint32_t flags;
    int running;
    uint32_t instrcount;
} vmstate;

void vminit(vmstate* vm);
void vmfree(vmstate* vm);
void vmreset(vmstate* vm);
uint8_t vmreadbyte(vmstate* vm, uint32_t addr);
void vmwritebyte(vmstate* vm, uint32_t addr, uint8_t val);
uint32_t vmreadword(vmstate* vm, uint32_t addr);
void vmwriteword(vmstate* vm, uint32_t addr, uint32_t val);
uint8_t vmfetchbyte(vmstate* vm);
uint32_t vmfetchword(vmstate* vm);
void vmpush(vmstate* vm, uint32_t val);
uint32_t vmpop(vmstate* vm);
void vmupdateflags(vmstate* vm, uint32_t result);

void vmexecute(vmstate* vm);

int vmassemble(const char* infile, const char* outfile);
int vmdisassemble(const char* infile, const char* outfile);
int vmcompile(const char* infile, const char* outfile);
int vmdecompile(const char* infile, const char* outfile);
