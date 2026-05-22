#include "core.h"
#include <stdlib.h>
#include <string.h>

void vminit(vmstate* vm) {
    vm->memory = (uint8_t*)calloc(MEMSIZE, 1);
    memset(vm->registers, 0, sizeof(vm->registers));
    vm->pc = 0;
    vm->sp = STACKBASE;
    vm->flags = 0;
    vm->running = 0;
    vm->instrcount = 0;
}

void vmfree(vmstate* vm) {
    if (vm->memory) {
        free(vm->memory);
        vm->memory = NULL;
    }
}

void vmreset(vmstate* vm) {
    memset(vm->memory, 0, MEMSIZE);
    memset(vm->registers, 0, sizeof(vm->registers));
    vm->pc = 0;
    vm->sp = STACKBASE;
    vm->flags = 0;
    vm->running = 0;
    vm->instrcount = 0;
}

uint8_t vmreadbyte(vmstate* vm, uint32_t addr) {
    if (addr >= MEMSIZE) return 0;
    return vm->memory[addr];
}

void vmwritebyte(vmstate* vm, uint32_t addr, uint8_t val) {
    if (addr >= MEMSIZE) return;
    vm->memory[addr] = val;
}

uint32_t vmreadword(vmstate* vm, uint32_t addr) {
    if (addr + 3 >= MEMSIZE) return 0;
    return vm->memory[addr] | (vm->memory[addr+1] << 8) |
           (vm->memory[addr+2] << 16) | (vm->memory[addr+3] << 24);
}

void vmwriteword(vmstate* vm, uint32_t addr, uint32_t val) {
    if (addr + 3 >= MEMSIZE) return;
    vm->memory[addr] = val & 0xFF;
    vm->memory[addr+1] = (val >> 8) & 0xFF;
    vm->memory[addr+2] = (val >> 16) & 0xFF;
    vm->memory[addr+3] = (val >> 24) & 0xFF;
}

uint8_t vmfetchbyte(vmstate* vm) {
    uint8_t b = vmreadbyte(vm, vm->pc);
    vm->pc++;
    return b;
}

uint32_t vmfetchword(vmstate* vm) {
    uint32_t w = vmreadword(vm, vm->pc);
    vm->pc += 4;
    return w;
}

void vmpush(vmstate* vm, uint32_t val) {
    vm->sp -= 4;
    vmwriteword(vm, vm->sp, val);
}

uint32_t vmpop(vmstate* vm) {
    uint32_t val = vmreadword(vm, vm->sp);
    vm->sp += 4;
    return val;
}

void vmupdateflags(vmstate* vm, uint32_t result) {
    vm->flags = 0;
    if (result == 0) vm->flags |= 0x40;
    if (result & 0x80000000) vm->flags |= 0x80;
    uint32_t ones = 0;
    for (int i = 0; i < 32; i++) {
        if (result & (1u << i)) ones++;
    }
    if ((ones % 2) == 0) vm->flags |= 0x04;
}
