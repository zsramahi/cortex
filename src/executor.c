#include "core.h"
#include "opcodes.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <time.h>

static int seeded = 0;

static uint32_t rotl32(uint32_t v, uint32_t n) {
    n &= 31;
    return (v << n) | (v >> (32 - n));
}

static uint32_t rotr32(uint32_t v, uint32_t n) {
    n &= 31;
    return (v >> n) | (v << (32 - n));
}

static uint32_t bswap32(uint32_t v) {
    return ((v >> 24) & 0xFF) | ((v >> 8) & 0xFF00) |
           ((v << 8) & 0xFF0000) | ((v << 24) & 0xFF000000);
}

static uint32_t crc32table[256];
static int crc32ready = 0;

static void initcrc32(void) {
    for (uint32_t i = 0; i < 256; i++) {
        uint32_t c = i;
        for (int j = 0; j < 8; j++)
            c = (c & 1) ? (0xEDB88320 ^ (c >> 1)) : (c >> 1);
        crc32table[i] = c;
    }
    crc32ready = 1;
}

static uint16_t fetchopcode(vmstate* vm) {
    uint8_t lo = vmfetchbyte(vm);
    uint8_t hi = vmfetchbyte(vm);
    return (uint16_t)(lo | (hi << 8));
}


void vmexecute(vmstate* vm) {
    if (vm->pc >= MEMSIZE) { vm->running = 0; return; }
    if (!seeded) { srand((unsigned)time(NULL)); seeded = 1; }

    uint16_t op = fetchopcode(vm);
    vm->instrcount++;

    uint8_t dst, src, reg;
    uint32_t addr, val, a, b, tmp;
    int32_t sa, sb;
    double fa, fb;

    switch (op) {

    case OP_NOP: break;
    case OP_HLT: vm->running = 0; break;

    case OP_MOV:
        dst = vmfetchbyte(vm); src = vmfetchbyte(vm);
        vm->registers[dst] = vm->registers[src];
        break;

    case OP_ADD:
        dst = vmfetchbyte(vm); src = vmfetchbyte(vm);
        vm->registers[dst] += vm->registers[src];
        vmupdateflags(vm, vm->registers[dst]);
        break;

    case OP_SUB:
        dst = vmfetchbyte(vm); src = vmfetchbyte(vm);
        vm->registers[dst] -= vm->registers[src];
        vmupdateflags(vm, vm->registers[dst]);
        break;

    case OP_MUL:
        dst = vmfetchbyte(vm); src = vmfetchbyte(vm);
        vm->registers[dst] *= vm->registers[src];
        vmupdateflags(vm, vm->registers[dst]);
        break;

    case OP_DIV:
        dst = vmfetchbyte(vm); src = vmfetchbyte(vm);
        if (vm->registers[src] != 0)
            vm->registers[dst] /= vm->registers[src];
        break;


    case OP_AND:
        dst = vmfetchbyte(vm); src = vmfetchbyte(vm);
        vm->registers[dst] &= vm->registers[src];
        vmupdateflags(vm, vm->registers[dst]);
        break;

    case OP_OR:
        dst = vmfetchbyte(vm); src = vmfetchbyte(vm);
        vm->registers[dst] |= vm->registers[src];
        vmupdateflags(vm, vm->registers[dst]);
        break;

    case OP_XOR:
        dst = vmfetchbyte(vm); src = vmfetchbyte(vm);
        vm->registers[dst] ^= vm->registers[src];
        vmupdateflags(vm, vm->registers[dst]);
        break;

    case OP_NOT:
        reg = vmfetchbyte(vm);
        vm->registers[reg] = ~vm->registers[reg];
        vmupdateflags(vm, vm->registers[reg]);
        break;

    case OP_SHL:
        dst = vmfetchbyte(vm); src = vmfetchbyte(vm);
        vm->registers[dst] <<= (vm->registers[src] & 31);
        vmupdateflags(vm, vm->registers[dst]);
        break;

    case OP_SHR:
        dst = vmfetchbyte(vm); src = vmfetchbyte(vm);
        vm->registers[dst] >>= (vm->registers[src] & 31);
        vmupdateflags(vm, vm->registers[dst]);
        break;

    case OP_CMP:
        dst = vmfetchbyte(vm); src = vmfetchbyte(vm);
        vmupdateflags(vm, vm->registers[dst] - vm->registers[src]);
        break;

    case OP_JMP:
        vm->pc = vmfetchword(vm);
        break;

    case OP_JE:
        addr = vmfetchword(vm);
        if (vm->flags & 0x40) vm->pc = addr;
        break;

    case OP_JNE:
        addr = vmfetchword(vm);
        if (!(vm->flags & 0x40)) vm->pc = addr;
        break;


    case OP_JG:
        addr = vmfetchword(vm);
        if (!(vm->flags & 0x40) && !(vm->flags & 0x80)) vm->pc = addr;
        break;

    case OP_JL:
        addr = vmfetchword(vm);
        if (vm->flags & 0x80) vm->pc = addr;
        break;

    case OP_JGE:
        addr = vmfetchword(vm);
        if (!(vm->flags & 0x80)) vm->pc = addr;
        break;

    case OP_JLE:
        addr = vmfetchword(vm);
        if ((vm->flags & 0x40) || (vm->flags & 0x80)) vm->pc = addr;
        break;

    case OP_CALL:
        addr = vmfetchword(vm);
        vmpush(vm, vm->pc);
        vm->pc = addr;
        break;

    case OP_RET:
        val = vmpop(vm);
        if (val == SENTINEL) vm->running = 0;
        else vm->pc = val;
        break;

    case OP_PUSH:
        reg = vmfetchbyte(vm);
        vmpush(vm, vm->registers[reg]);
        break;

    case OP_POP:
        reg = vmfetchbyte(vm);
        vm->registers[reg] = vmpop(vm);
        break;

    case OP_LOAD:
        dst = vmfetchbyte(vm); src = vmfetchbyte(vm);
        vm->registers[dst] = vmreadword(vm, vm->registers[src]);
        break;

    case OP_STORE:
        dst = vmfetchbyte(vm); src = vmfetchbyte(vm);
        vmwriteword(vm, vm->registers[dst], vm->registers[src]);
        break;

    case OP_IN:
        reg = vmfetchbyte(vm);
        printf("in> ");
        scanf("%u", &vm->registers[reg]);
        break;

    case OP_OUT:
        reg = vmfetchbyte(vm);
        printf("r%d = %u\n", reg, vm->registers[reg]);
        break;


    case OP_MOVIMM:
        reg = vmfetchbyte(vm);
        vm->registers[reg] = vmfetchword(vm);
        break;

    case OP_MOD:
        dst = vmfetchbyte(vm); src = vmfetchbyte(vm);
        if (vm->registers[src] != 0)
            vm->registers[dst] %= vm->registers[src];
        break;

    case OP_INC:
        reg = vmfetchbyte(vm);
        vm->registers[reg]++;
        vmupdateflags(vm, vm->registers[reg]);
        break;

    case OP_DEC:
        reg = vmfetchbyte(vm);
        vm->registers[reg]--;
        vmupdateflags(vm, vm->registers[reg]);
        break;

    case OP_SAR:
        dst = vmfetchbyte(vm); src = vmfetchbyte(vm);
        sa = (int32_t)vm->registers[dst];
        vm->registers[dst] = (uint32_t)(sa >> (vm->registers[src] & 31));
        break;

    case OP_ROL:
        dst = vmfetchbyte(vm); src = vmfetchbyte(vm);
        vm->registers[dst] = rotl32(vm->registers[dst], vm->registers[src]);
        break;

    case OP_ROR:
        dst = vmfetchbyte(vm); src = vmfetchbyte(vm);
        vm->registers[dst] = rotr32(vm->registers[dst], vm->registers[src]);
        break;

    case OP_TEST:
        dst = vmfetchbyte(vm); src = vmfetchbyte(vm);
        vmupdateflags(vm, vm->registers[dst] & vm->registers[src]);
        break;

    case OP_JZ:
        addr = vmfetchword(vm);
        if (vm->flags & 0x40) vm->pc = addr;
        break;

    case OP_JNZ:
        addr = vmfetchword(vm);
        if (!(vm->flags & 0x40)) vm->pc = addr;
        break;

    case OP_JC:
        addr = vmfetchword(vm);
        if (vm->flags & 0x01) vm->pc = addr;
        break;

    case OP_JNC:
        addr = vmfetchword(vm);
        if (!(vm->flags & 0x01)) vm->pc = addr;
        break;


    case OP_LOADBYTE:
        dst = vmfetchbyte(vm); src = vmfetchbyte(vm);
        vm->registers[dst] = vmreadbyte(vm, vm->registers[src]);
        break;

    case OP_STOREBYTE:
        dst = vmfetchbyte(vm); src = vmfetchbyte(vm);
        vmwritebyte(vm, vm->registers[dst], (uint8_t)vm->registers[src]);
        break;

    case OP_LOADREG:
        dst = vmfetchbyte(vm); src = vmfetchbyte(vm);
        vm->registers[dst] = vmreadword(vm, vm->registers[src]);
        break;

    case OP_STOREREG:
        dst = vmfetchbyte(vm); src = vmfetchbyte(vm);
        vmwriteword(vm, vm->registers[dst], vm->registers[src]);
        break;

    case OP_CLR:
        reg = vmfetchbyte(vm);
        vm->registers[reg] = 0;
        break;

    case OP_SWAP:
        dst = vmfetchbyte(vm); src = vmfetchbyte(vm);
        tmp = vm->registers[dst];
        vm->registers[dst] = vm->registers[src];
        vm->registers[src] = tmp;
        break;

    case OP_NEG:
        reg = vmfetchbyte(vm);
        vm->registers[reg] = (uint32_t)(-(int32_t)vm->registers[reg]);
        vmupdateflags(vm, vm->registers[reg]);
        break;

    case OP_ABS:
        reg = vmfetchbyte(vm);
        sa = (int32_t)vm->registers[reg];
        vm->registers[reg] = (uint32_t)(sa < 0 ? -sa : sa);
        break;

    case OP_BSWAP:
        reg = vmfetchbyte(vm);
        vm->registers[reg] = bswap32(vm->registers[reg]);
        break;

    case OP_BTS:
        dst = vmfetchbyte(vm); src = vmfetchbyte(vm);
        vm->registers[dst] |= (1u << (vm->registers[src] & 31));
        break;

    case OP_BTR:
        dst = vmfetchbyte(vm); src = vmfetchbyte(vm);
        vm->registers[dst] &= ~(1u << (vm->registers[src] & 31));
        break;

    case OP_BTC:
        dst = vmfetchbyte(vm); src = vmfetchbyte(vm);
        vm->registers[dst] ^= (1u << (vm->registers[src] & 31));
        break;


    case OP_BSF:
        reg = vmfetchbyte(vm);
        val = vm->registers[reg];
        if (val == 0) { vm->registers[reg] = 32; }
        else { uint32_t pos = 0; while (!(val & (1u << pos))) pos++; vm->registers[reg] = pos; }
        break;

    case OP_BSR:
        reg = vmfetchbyte(vm);
        val = vm->registers[reg];
        if (val == 0) { vm->registers[reg] = 32; }
        else { uint32_t pos = 31; while (!(val & (1u << pos))) pos--; vm->registers[reg] = pos; }
        break;

    case OP_POPCNT:
        reg = vmfetchbyte(vm);
        val = vm->registers[reg];
        val = val - ((val >> 1) & 0x55555555);
        val = (val & 0x33333333) + ((val >> 2) & 0x33333333);
        vm->registers[reg] = (((val + (val >> 4)) & 0x0F0F0F0F) * 0x01010101) >> 24;
        break;

    case OP_ADDIMM:
        reg = vmfetchbyte(vm); val = vmfetchword(vm);
        vm->registers[reg] += val;
        vmupdateflags(vm, vm->registers[reg]);
        break;

    case OP_SUBIMM:
        reg = vmfetchbyte(vm); val = vmfetchword(vm);
        vm->registers[reg] -= val;
        vmupdateflags(vm, vm->registers[reg]);
        break;

    case OP_MULIMM:
        reg = vmfetchbyte(vm); val = vmfetchword(vm);
        vm->registers[reg] *= val;
        break;

    case OP_ANDIMM:
        reg = vmfetchbyte(vm); val = vmfetchword(vm);
        vm->registers[reg] &= val;
        vmupdateflags(vm, vm->registers[reg]);
        break;

    case OP_ORIMM:
        reg = vmfetchbyte(vm); val = vmfetchword(vm);
        vm->registers[reg] |= val;
        vmupdateflags(vm, vm->registers[reg]);
        break;

    case OP_XORIMM:
        reg = vmfetchbyte(vm); val = vmfetchword(vm);
        vm->registers[reg] ^= val;
        vmupdateflags(vm, vm->registers[reg]);
        break;

    case OP_CMPIMM:
        reg = vmfetchbyte(vm); val = vmfetchword(vm);
        vmupdateflags(vm, vm->registers[reg] - val);
        break;

    case OP_PUSHIMM:
        val = vmfetchword(vm);
        vmpush(vm, val);
        break;


    case OP_DUP:
        val = vmreadword(vm, vm->sp);
        vmpush(vm, val);
        break;

    case OP_DROP:
        vmpop(vm);
        break;

    case OP_OVER:
        val = vmreadword(vm, vm->sp + 4);
        vmpush(vm, val);
        break;

    case OP_ROT: {
        uint32_t c = vmpop(vm); uint32_t b2 = vmpop(vm); uint32_t a2 = vmpop(vm);
        vmpush(vm, b2); vmpush(vm, c); vmpush(vm, a2);
        break;
    }

    case OP_PICK:
        reg = vmfetchbyte(vm);
        val = vmreadword(vm, vm->sp + vm->registers[reg] * 4);
        vmpush(vm, val);
        break;

    case OP_LOOP:
        addr = vmfetchword(vm);
        vm->registers[0]--;
        if (vm->registers[0] != 0) vm->pc = addr;
        break;

    case OP_LOOPE:
        addr = vmfetchword(vm);
        vm->registers[0]--;
        if (vm->registers[0] != 0 && (vm->flags & 0x40)) vm->pc = addr;
        break;

    case OP_LOOPNE:
        addr = vmfetchword(vm);
        vm->registers[0]--;
        if (vm->registers[0] != 0 && !(vm->flags & 0x40)) vm->pc = addr;
        break;

    case OP_STRCPY:
        dst = vmfetchbyte(vm); src = vmfetchbyte(vm);
        a = vm->registers[dst]; b = vm->registers[src];
        while (a < MEMSIZE && b < MEMSIZE) {
            uint8_t ch = vmreadbyte(vm, b);
            vmwritebyte(vm, a, ch);
            if (ch == 0) break;
            a++; b++;
        }
        break;

    case OP_STRCMP:
        dst = vmfetchbyte(vm); src = vmfetchbyte(vm);
        a = vm->registers[dst]; b = vm->registers[src];
        val = 0;
        while (a < MEMSIZE && b < MEMSIZE) {
            uint8_t ca = vmreadbyte(vm, a);
            uint8_t cb = vmreadbyte(vm, b);
            if (ca != cb) { val = (ca < cb) ? (uint32_t)-1 : 1; break; }
            if (ca == 0) break;
            a++; b++;
        }
        vm->registers[dst] = val;
        vmupdateflags(vm, val);
        break;


    case OP_STRLEN:
        reg = vmfetchbyte(vm);
        a = vm->registers[reg]; val = 0;
        while (a < MEMSIZE && vmreadbyte(vm, a) != 0) { val++; a++; }
        vm->registers[reg] = val;
        break;

    case OP_MEMSET:
        dst = vmfetchbyte(vm); src = vmfetchbyte(vm); reg = vmfetchbyte(vm);
        a = vm->registers[dst]; b = vm->registers[src]; val = vm->registers[reg];
        for (uint32_t i = 0; i < val && a + i < MEMSIZE; i++)
            vmwritebyte(vm, a + i, (uint8_t)b);
        break;

    case OP_MEMCPY:
        dst = vmfetchbyte(vm); src = vmfetchbyte(vm); reg = vmfetchbyte(vm);
        a = vm->registers[dst]; b = vm->registers[src]; val = vm->registers[reg];
        for (uint32_t i = 0; i < val && a + i < MEMSIZE && b + i < MEMSIZE; i++)
            vmwritebyte(vm, a + i, vmreadbyte(vm, b + i));
        break;

    case OP_MIN:
        dst = vmfetchbyte(vm); src = vmfetchbyte(vm);
        if (vm->registers[src] < vm->registers[dst])
            vm->registers[dst] = vm->registers[src];
        break;

    case OP_MAX:
        dst = vmfetchbyte(vm); src = vmfetchbyte(vm);
        if (vm->registers[src] > vm->registers[dst])
            vm->registers[dst] = vm->registers[src];
        break;

    case OP_SQRT:
        reg = vmfetchbyte(vm);
        vm->registers[reg] = (uint32_t)sqrt((double)vm->registers[reg]);
        break;

    case OP_POW:
        dst = vmfetchbyte(vm); src = vmfetchbyte(vm);
        vm->registers[dst] = (uint32_t)pow((double)vm->registers[dst], (double)vm->registers[src]);
        break;

    case OP_SIN:
        reg = vmfetchbyte(vm);
        fa = (double)vm->registers[reg] / 1000.0;
        vm->registers[reg] = (uint32_t)(int32_t)(sin(fa) * 1000.0);
        break;

    case OP_COS:
        reg = vmfetchbyte(vm);
        fa = (double)vm->registers[reg] / 1000.0;
        vm->registers[reg] = (uint32_t)(int32_t)(cos(fa) * 1000.0);
        break;

    case OP_TAN:
        reg = vmfetchbyte(vm);
        fa = (double)vm->registers[reg] / 1000.0;
        vm->registers[reg] = (uint32_t)(int32_t)(tan(fa) * 1000.0);
        break;

    case OP_RAND:
        reg = vmfetchbyte(vm);
        vm->registers[reg] = (uint32_t)rand();
        break;


    case OP_INT:
        reg = vmfetchbyte(vm);
        break;

    case OP_IRET:
        val = vmpop(vm);
        if (val == SENTINEL) vm->running = 0;
        else vm->pc = val;
        break;

    case OP_CLI:
        vm->flags &= ~0x200;
        break;

    case OP_STI:
        vm->flags |= 0x200;
        break;

    case OP_PAUSE:
        break;

    case OP_NOPMULTI:
        reg = vmfetchbyte(vm);
        break;

    case OP_CACHEFLUSH:
        break;

    case OP_PREFETCH:
        reg = vmfetchbyte(vm);
        break;

    case OP_FENCE:
        break;

    case OP_GETPC:
        reg = vmfetchbyte(vm);
        vm->registers[reg] = vm->pc;
        break;

    case OP_GETSP:
        reg = vmfetchbyte(vm);
        vm->registers[reg] = vm->sp;
        break;

    case OP_SETSP:
        reg = vmfetchbyte(vm);
        vm->sp = vm->registers[reg];
        break;

    case OP_GETFLAGS:
        reg = vmfetchbyte(vm);
        vm->registers[reg] = vm->flags;
        break;

    case OP_SETFLAGS:
        reg = vmfetchbyte(vm);
        vm->flags = vm->registers[reg];
        break;

    case OP_CLC: vm->flags &= ~0x01; break;
    case OP_STC: vm->flags |= 0x01; break;
    case OP_CMC: vm->flags ^= 0x01; break;
    case OP_STD: vm->flags |= 0x400; break;
    case OP_CLD: vm->flags &= ~0x400; break;

    case OP_SAHF:
        reg = vmfetchbyte(vm);
        vm->flags = (vm->flags & 0xFFFFFF00) | (vm->registers[reg] & 0xFF);
        break;

    case OP_LAHF:
        reg = vmfetchbyte(vm);
        vm->registers[reg] = vm->flags & 0xFF;
        break;

    case OP_PUSHF:
        vmpush(vm, vm->flags);
        break;

    case OP_POPF:
        vm->flags = vmpop(vm);
        break;


    case OP_PUSHA:
        for (int i = 0; i < REGCOUNT; i++) vmpush(vm, vm->registers[i]);
        break;

    case OP_POPA:
        for (int i = REGCOUNT - 1; i >= 0; i--) vm->registers[i] = vmpop(vm);
        break;

    case OP_ENTER:
        vmpush(vm, vm->registers[255]);
        vm->registers[255] = vm->sp;
        val = vmfetchword(vm);
        vm->sp -= val;
        break;

    case OP_LEAVE:
        vm->sp = vm->registers[255];
        vm->registers[255] = vmpop(vm);
        break;

    case OP_XCHG:
        dst = vmfetchbyte(vm); src = vmfetchbyte(vm);
        tmp = vm->registers[dst];
        vm->registers[dst] = vm->registers[src];
        vm->registers[src] = tmp;
        break;

    case OP_LEA:
        dst = vmfetchbyte(vm); src = vmfetchbyte(vm);
        vm->registers[dst] = vm->registers[src];
        break;

    case OP_CMPXCHG:
        dst = vmfetchbyte(vm); src = vmfetchbyte(vm);
        if (vm->registers[0] == vm->registers[dst]) {
            vm->flags |= 0x40;
            vm->registers[dst] = vm->registers[src];
        } else {
            vm->flags &= ~0x40;
            vm->registers[0] = vm->registers[dst];
        }
        break;

    case OP_XADD:
        dst = vmfetchbyte(vm); src = vmfetchbyte(vm);
        tmp = vm->registers[dst];
        vm->registers[dst] += vm->registers[src];
        vm->registers[src] = tmp;
        vmupdateflags(vm, vm->registers[dst]);
        break;

    case OP_LOCK: break;
    case OP_UNLOCK: break;
    case OP_WAIT: break;

    case OP_ADC:
        dst = vmfetchbyte(vm); src = vmfetchbyte(vm);
        vm->registers[dst] += vm->registers[src] + (vm->flags & 0x01);
        vmupdateflags(vm, vm->registers[dst]);
        break;

    case OP_SBB:
        dst = vmfetchbyte(vm); src = vmfetchbyte(vm);
        vm->registers[dst] -= vm->registers[src] + (vm->flags & 0x01);
        vmupdateflags(vm, vm->registers[dst]);
        break;

    case OP_IMUL:
        dst = vmfetchbyte(vm); src = vmfetchbyte(vm);
        sa = (int32_t)vm->registers[dst];
        sb = (int32_t)vm->registers[src];
        vm->registers[dst] = (uint32_t)(sa * sb);
        break;

    case OP_IDIV:
        dst = vmfetchbyte(vm); src = vmfetchbyte(vm);
        sb = (int32_t)vm->registers[src];
        if (sb != 0) vm->registers[dst] = (uint32_t)((int32_t)vm->registers[dst] / sb);
        break;


    case OP_MOVSX:
        dst = vmfetchbyte(vm); src = vmfetchbyte(vm);
        vm->registers[dst] = (uint32_t)(int32_t)(int8_t)(vm->registers[src] & 0xFF);
        break;

    case OP_MOVZX:
        dst = vmfetchbyte(vm); src = vmfetchbyte(vm);
        vm->registers[dst] = vm->registers[src] & 0xFF;
        break;

    case OP_SETC:
        reg = vmfetchbyte(vm);
        vm->registers[reg] = (vm->flags & 0x01) ? 1 : 0;
        break;

    case OP_SETZ:
        reg = vmfetchbyte(vm);
        vm->registers[reg] = (vm->flags & 0x40) ? 1 : 0;
        break;

    case OP_SETN:
        reg = vmfetchbyte(vm);
        vm->registers[reg] = (vm->flags & 0x80) ? 1 : 0;
        break;

    case OP_SETO:
        reg = vmfetchbyte(vm);
        vm->registers[reg] = (vm->flags & 0x800) ? 1 : 0;
        break;

    case OP_SETE:
        reg = vmfetchbyte(vm);
        vm->registers[reg] = (vm->flags & 0x40) ? 1 : 0;
        break;

    case OP_SETNE:
        reg = vmfetchbyte(vm);
        vm->registers[reg] = (vm->flags & 0x40) ? 0 : 1;
        break;

    case OP_SETG:
        reg = vmfetchbyte(vm);
        vm->registers[reg] = (!(vm->flags & 0x40) && !(vm->flags & 0x80)) ? 1 : 0;
        break;

    case OP_SETL:
        reg = vmfetchbyte(vm);
        vm->registers[reg] = (vm->flags & 0x80) ? 1 : 0;
        break;

    case OP_SETGE:
        reg = vmfetchbyte(vm);
        vm->registers[reg] = !(vm->flags & 0x80) ? 1 : 0;
        break;

    case OP_SETLE:
        reg = vmfetchbyte(vm);
        vm->registers[reg] = ((vm->flags & 0x40) || (vm->flags & 0x80)) ? 1 : 0;
        break;

    case OP_CMOVE:
        dst = vmfetchbyte(vm); src = vmfetchbyte(vm);
        if (vm->flags & 0x40) vm->registers[dst] = vm->registers[src];
        break;

    case OP_CMOVNE:
        dst = vmfetchbyte(vm); src = vmfetchbyte(vm);
        if (!(vm->flags & 0x40)) vm->registers[dst] = vm->registers[src];
        break;

    case OP_CMOVG:
        dst = vmfetchbyte(vm); src = vmfetchbyte(vm);
        if (!(vm->flags & 0x40) && !(vm->flags & 0x80)) vm->registers[dst] = vm->registers[src];
        break;

    case OP_CMOVL:
        dst = vmfetchbyte(vm); src = vmfetchbyte(vm);
        if (vm->flags & 0x80) vm->registers[dst] = vm->registers[src];
        break;

    case OP_CMOVGE:
        dst = vmfetchbyte(vm); src = vmfetchbyte(vm);
        if (!(vm->flags & 0x80)) vm->registers[dst] = vm->registers[src];
        break;

    case OP_CMOVLE:
        dst = vmfetchbyte(vm); src = vmfetchbyte(vm);
        if ((vm->flags & 0x40) || (vm->flags & 0x80)) vm->registers[dst] = vm->registers[src];
        break;


    case OP_SHLD:
        dst = vmfetchbyte(vm); src = vmfetchbyte(vm); reg = vmfetchbyte(vm);
        val = vm->registers[reg] & 31;
        vm->registers[dst] = (vm->registers[dst] << val) | (vm->registers[src] >> (32 - val));
        break;

    case OP_SHRD:
        dst = vmfetchbyte(vm); src = vmfetchbyte(vm); reg = vmfetchbyte(vm);
        val = vm->registers[reg] & 31;
        vm->registers[dst] = (vm->registers[dst] >> val) | (vm->registers[src] << (32 - val));
        break;

    case OP_RCL:
        dst = vmfetchbyte(vm); src = vmfetchbyte(vm);
        val = vm->registers[src] & 31;
        for (uint32_t i = 0; i < val; i++) {
            uint32_t msb = (vm->registers[dst] >> 31) & 1;
            vm->registers[dst] = (vm->registers[dst] << 1) | (vm->flags & 1);
            vm->flags = (vm->flags & ~1u) | msb;
        }
        break;

    case OP_RCR:
        dst = vmfetchbyte(vm); src = vmfetchbyte(vm);
        val = vm->registers[src] & 31;
        for (uint32_t i = 0; i < val; i++) {
            uint32_t lsb = vm->registers[dst] & 1;
            vm->registers[dst] = (vm->registers[dst] >> 1) | ((vm->flags & 1) << 31);
            vm->flags = (vm->flags & ~1u) | lsb;
        }
        break;

    case OP_ASIN:
        reg = vmfetchbyte(vm);
        fa = (double)(int32_t)vm->registers[reg] / 1000.0;
        vm->registers[reg] = (uint32_t)(int32_t)(asin(fa) * 1000.0);
        break;

    case OP_ACOS:
        reg = vmfetchbyte(vm);
        fa = (double)(int32_t)vm->registers[reg] / 1000.0;
        vm->registers[reg] = (uint32_t)(int32_t)(acos(fa) * 1000.0);
        break;

    case OP_ATAN:
        reg = vmfetchbyte(vm);
        fa = (double)(int32_t)vm->registers[reg] / 1000.0;
        vm->registers[reg] = (uint32_t)(int32_t)(atan(fa) * 1000.0);
        break;

    case OP_LOG:
        reg = vmfetchbyte(vm);
        fa = (double)vm->registers[reg] / 1000.0;
        vm->registers[reg] = (fa > 0) ? (uint32_t)(int32_t)(log(fa) * 1000.0) : 0;
        break;

    case OP_LOG10:
        reg = vmfetchbyte(vm);
        fa = (double)vm->registers[reg] / 1000.0;
        vm->registers[reg] = (fa > 0) ? (uint32_t)(int32_t)(log10(fa) * 1000.0) : 0;
        break;

    case OP_EXP:
        reg = vmfetchbyte(vm);
        fa = (double)(int32_t)vm->registers[reg] / 1000.0;
        vm->registers[reg] = (uint32_t)(int32_t)(exp(fa) * 1000.0);
        break;

    case OP_CEIL:
        reg = vmfetchbyte(vm);
        fa = (double)(int32_t)vm->registers[reg] / 1000.0;
        vm->registers[reg] = (uint32_t)(int32_t)(ceil(fa) * 1000.0);
        break;

    case OP_FLOOR:
        reg = vmfetchbyte(vm);
        fa = (double)(int32_t)vm->registers[reg] / 1000.0;
        vm->registers[reg] = (uint32_t)(int32_t)(floor(fa) * 1000.0);
        break;


    case OP_ROUND:
        reg = vmfetchbyte(vm);
        fa = (double)(int32_t)vm->registers[reg] / 1000.0;
        vm->registers[reg] = (uint32_t)(int32_t)(round(fa) * 1000.0);
        break;

    case OP_TRUNC:
        reg = vmfetchbyte(vm);
        fa = (double)(int32_t)vm->registers[reg] / 1000.0;
        vm->registers[reg] = (uint32_t)(int32_t)(trunc(fa) * 1000.0);
        break;

    case OP_FABS:
        reg = vmfetchbyte(vm);
        sa = (int32_t)vm->registers[reg];
        vm->registers[reg] = (uint32_t)(sa < 0 ? -sa : sa);
        break;

    case OP_SINH:
        reg = vmfetchbyte(vm);
        fa = (double)(int32_t)vm->registers[reg] / 1000.0;
        vm->registers[reg] = (uint32_t)(int32_t)(sinh(fa) * 1000.0);
        break;

    case OP_COSH:
        reg = vmfetchbyte(vm);
        fa = (double)(int32_t)vm->registers[reg] / 1000.0;
        vm->registers[reg] = (uint32_t)(int32_t)(cosh(fa) * 1000.0);
        break;

    case OP_TANH:
        reg = vmfetchbyte(vm);
        fa = (double)(int32_t)vm->registers[reg] / 1000.0;
        vm->registers[reg] = (uint32_t)(int32_t)(tanh(fa) * 1000.0);
        break;

    case OP_GCD:
        dst = vmfetchbyte(vm); src = vmfetchbyte(vm);
        a = vm->registers[dst]; b = vm->registers[src];
        while (b != 0) { tmp = b; b = a % b; a = tmp; }
        vm->registers[dst] = a;
        break;

    case OP_LCM:
        dst = vmfetchbyte(vm); src = vmfetchbyte(vm);
        a = vm->registers[dst]; b = vm->registers[src];
        if (a == 0 || b == 0) { vm->registers[dst] = 0; break; }
        tmp = a; val = b;
        while (val != 0) { uint32_t t2 = val; val = tmp % val; tmp = t2; }
        vm->registers[dst] = (a / tmp) * b;
        break;

    case OP_FACTORIAL:
        reg = vmfetchbyte(vm);
        val = vm->registers[reg]; tmp = 1;
        for (uint32_t i = 2; i <= val; i++) tmp *= i;
        vm->registers[reg] = tmp;
        break;

    case OP_FIBONACCI:
        reg = vmfetchbyte(vm);
        val = vm->registers[reg];
        if (val <= 1) { vm->registers[reg] = val; }
        else {
            a = 0; b = 1;
            for (uint32_t i = 2; i <= val; i++) { tmp = a + b; a = b; b = tmp; }
            vm->registers[reg] = b;
        }
        break;

    case OP_ISPRIME:
        reg = vmfetchbyte(vm);
        val = vm->registers[reg];
        tmp = (val > 1) ? 1 : 0;
        if (val > 2 && val % 2 == 0) tmp = 0;
        for (uint32_t i = 3; i * i <= val && tmp; i += 2)
            if (val % i == 0) tmp = 0;
        vm->registers[reg] = tmp;
        break;

    case OP_NEXTPRIME:
        reg = vmfetchbyte(vm);
        val = vm->registers[reg];
        if (val < 2) val = 2;
        else { val++; if (val % 2 == 0) val++;
            while (1) { int ip = 1;
                for (uint32_t i = 3; i * i <= val; i += 2) if (val % i == 0) { ip = 0; break; }
                if (ip) break; val += 2; }
        }
        vm->registers[reg] = val;
        break;


    case OP_REVERSEBITS:
        reg = vmfetchbyte(vm);
        val = vm->registers[reg]; tmp = 0;
        for (int i = 0; i < 32; i++) { tmp = (tmp << 1) | (val & 1); val >>= 1; }
        vm->registers[reg] = tmp;
        break;

    case OP_GRAYENCODE:
        reg = vmfetchbyte(vm);
        vm->registers[reg] = vm->registers[reg] ^ (vm->registers[reg] >> 1);
        break;

    case OP_GRAYDECODE:
        reg = vmfetchbyte(vm);
        val = vm->registers[reg]; tmp = val;
        while (val >>= 1) tmp ^= val;
        vm->registers[reg] = tmp;
        break;

    case OP_PARITY:
        reg = vmfetchbyte(vm);
        val = vm->registers[reg];
        val ^= val >> 16; val ^= val >> 8; val ^= val >> 4;
        val ^= val >> 2; val ^= val >> 1;
        vm->registers[reg] = val & 1;
        break;

    case OP_HAMMINGWEIGHT:
        reg = vmfetchbyte(vm);
        val = vm->registers[reg];
        val = val - ((val >> 1) & 0x55555555);
        val = (val & 0x33333333) + ((val >> 2) & 0x33333333);
        vm->registers[reg] = (((val + (val >> 4)) & 0x0F0F0F0F) * 0x01010101) >> 24;
        break;

    case OP_HAMMINGDISTANCE:
        dst = vmfetchbyte(vm); src = vmfetchbyte(vm);
        val = vm->registers[dst] ^ vm->registers[src];
        val = val - ((val >> 1) & 0x55555555);
        val = (val & 0x33333333) + ((val >> 2) & 0x33333333);
        vm->registers[dst] = (((val + (val >> 4)) & 0x0F0F0F0F) * 0x01010101) >> 24;
        break;

    case OP_CRC32:
        dst = vmfetchbyte(vm); src = vmfetchbyte(vm);
        if (!crc32ready) initcrc32();
        a = vm->registers[dst]; b = vm->registers[src];
        val = 0xFFFFFFFF;
        for (uint32_t i = 0; i < b && a + i < MEMSIZE; i++)
            val = crc32table[(val ^ vmreadbyte(vm, a + i)) & 0xFF] ^ (val >> 8);
        vm->registers[dst] = val ^ 0xFFFFFFFF;
        break;

    case OP_HASH:
        dst = vmfetchbyte(vm); src = vmfetchbyte(vm);
        a = vm->registers[dst]; b = vm->registers[src];
        val = 2166136261u;
        for (uint32_t i = 0; i < b && a + i < MEMSIZE; i++) {
            val ^= vmreadbyte(vm, a + i);
            val *= 16777619u;
        }
        vm->registers[dst] = val;
        break;

    case OP_ATOMICADD:
        dst = vmfetchbyte(vm); src = vmfetchbyte(vm);
        a = vm->registers[dst];
        val = vmreadword(vm, a);
        vmwriteword(vm, a, val + vm->registers[src]);
        vm->registers[src] = val;
        break;

    case OP_ATOMICSUB:
        dst = vmfetchbyte(vm); src = vmfetchbyte(vm);
        a = vm->registers[dst];
        val = vmreadword(vm, a);
        vmwriteword(vm, a, val - vm->registers[src]);
        vm->registers[src] = val;
        break;

    case OP_ATOMICXCHG:
        dst = vmfetchbyte(vm); src = vmfetchbyte(vm);
        a = vm->registers[dst];
        val = vmreadword(vm, a);
        vmwriteword(vm, a, vm->registers[src]);
        vm->registers[src] = val;
        break;


    case OP_ATOMICAND:
        dst = vmfetchbyte(vm); src = vmfetchbyte(vm);
        a = vm->registers[dst]; val = vmreadword(vm, a);
        vmwriteword(vm, a, val & vm->registers[src]);
        vm->registers[src] = val;
        break;

    case OP_ATOMICOR:
        dst = vmfetchbyte(vm); src = vmfetchbyte(vm);
        a = vm->registers[dst]; val = vmreadword(vm, a);
        vmwriteword(vm, a, val | vm->registers[src]);
        vm->registers[src] = val;
        break;

    case OP_ATOMICXOR:
        dst = vmfetchbyte(vm); src = vmfetchbyte(vm);
        a = vm->registers[dst]; val = vmreadword(vm, a);
        vmwriteword(vm, a, val ^ vm->registers[src]);
        vm->registers[src] = val;
        break;

    case OP_ATOMICMAX:
        dst = vmfetchbyte(vm); src = vmfetchbyte(vm);
        a = vm->registers[dst]; val = vmreadword(vm, a);
        vmwriteword(vm, a, val > vm->registers[src] ? val : vm->registers[src]);
        vm->registers[src] = val;
        break;

    case OP_ATOMICMIN:
        dst = vmfetchbyte(vm); src = vmfetchbyte(vm);
        a = vm->registers[dst]; val = vmreadword(vm, a);
        vmwriteword(vm, a, val < vm->registers[src] ? val : vm->registers[src]);
        vm->registers[src] = val;
        break;

    case OP_BARRIER: break;
    case OP_MEMBARRIER: break;
    case OP_LOADFENCE: break;
    case OP_STOREFENCE: break;
    case OP_FULLFENCE: break;
    case OP_SPINLOCK: break;
    case OP_SPINUNLOCK: break;
    case OP_YIELD: break;

    case OP_SLEEP:
        reg = vmfetchbyte(vm);
        break;

    case OP_WAKEUP:
        reg = vmfetchbyte(vm);
        break;

    case OP_THREADCREATE:
        dst = vmfetchbyte(vm); src = vmfetchbyte(vm);
        vm->registers[dst] = 0;
        break;

    case OP_THREADEXIT: break;
    case OP_THREADJOIN: reg = vmfetchbyte(vm); break;
    case OP_MUTEXLOCK: reg = vmfetchbyte(vm); break;
    case OP_MUTEXUNLOCK: reg = vmfetchbyte(vm); break;
    case OP_SEMWAIT: reg = vmfetchbyte(vm); break;
    case OP_SEMPOST: reg = vmfetchbyte(vm); break;
    case OP_CONDWAIT: reg = vmfetchbyte(vm); break;
    case OP_CONDSIGNAL: reg = vmfetchbyte(vm); break;
    case OP_CONDBROADCAST: reg = vmfetchbyte(vm); break;


    case OP_ALLOCATE:
        reg = vmfetchbyte(vm);
        val = vm->registers[reg];
        a = 0x10000;
        while (a + val < MEMSIZE - 0x10000) {
            int free = 1;
            for (uint32_t i = 0; i < val && free; i++)
                if (vmreadbyte(vm, a + i) != 0) free = 0;
            if (free) { vm->registers[reg] = a; break; }
            a += 4096;
        }
        break;

    case OP_DEALLOCATE:
        dst = vmfetchbyte(vm); src = vmfetchbyte(vm);
        a = vm->registers[dst]; b = vm->registers[src];
        for (uint32_t i = 0; i < b && a + i < MEMSIZE; i++)
            vmwritebyte(vm, a + i, 0);
        break;

    case OP_REALLOCATE:
        dst = vmfetchbyte(vm); src = vmfetchbyte(vm);
        break;

    case OP_MEMZERO:
        dst = vmfetchbyte(vm); src = vmfetchbyte(vm);
        a = vm->registers[dst]; b = vm->registers[src];
        for (uint32_t i = 0; i < b && a + i < MEMSIZE; i++)
            vmwritebyte(vm, a + i, 0);
        break;

    case OP_MEMFILL:
        dst = vmfetchbyte(vm); src = vmfetchbyte(vm); reg = vmfetchbyte(vm);
        a = vm->registers[dst]; b = vm->registers[src]; val = vm->registers[reg];
        for (uint32_t i = 0; i < val && a + i < MEMSIZE; i++)
            vmwritebyte(vm, a + i, (uint8_t)b);
        break;

    case OP_MEMCOMPARE:
        dst = vmfetchbyte(vm); src = vmfetchbyte(vm); reg = vmfetchbyte(vm);
        a = vm->registers[dst]; b = vm->registers[src]; val = vm->registers[reg];
        tmp = 0;
        for (uint32_t i = 0; i < val; i++) {
            uint8_t ca = vmreadbyte(vm, a + i);
            uint8_t cb = vmreadbyte(vm, b + i);
            if (ca != cb) { tmp = (ca < cb) ? (uint32_t)-1 : 1; break; }
        }
        vm->registers[dst] = tmp;
        vmupdateflags(vm, tmp);
        break;

    case OP_MEMMOVE:
        dst = vmfetchbyte(vm); src = vmfetchbyte(vm); reg = vmfetchbyte(vm);
        a = vm->registers[dst]; b = vm->registers[src]; val = vm->registers[reg];
        if (a < b) {
            for (uint32_t i = 0; i < val; i++)
                vmwritebyte(vm, a + i, vmreadbyte(vm, b + i));
        } else {
            for (uint32_t i = val; i > 0; i--)
                vmwritebyte(vm, a + i - 1, vmreadbyte(vm, b + i - 1));
        }
        break;

    case OP_MEMSWAP:
        dst = vmfetchbyte(vm); src = vmfetchbyte(vm); reg = vmfetchbyte(vm);
        a = vm->registers[dst]; b = vm->registers[src]; val = vm->registers[reg];
        for (uint32_t i = 0; i < val; i++) {
            uint8_t t = vmreadbyte(vm, a + i);
            vmwritebyte(vm, a + i, vmreadbyte(vm, b + i));
            vmwritebyte(vm, b + i, t);
        }
        break;


    case OP_BITCOUNT:
        reg = vmfetchbyte(vm);
        val = vm->registers[reg];
        val = val - ((val >> 1) & 0x55555555);
        val = (val & 0x33333333) + ((val >> 2) & 0x33333333);
        vm->registers[reg] = (((val + (val >> 4)) & 0x0F0F0F0F) * 0x01010101) >> 24;
        break;

    case OP_BITREVERSE:
        reg = vmfetchbyte(vm);
        val = vm->registers[reg]; tmp = 0;
        for (int i = 0; i < 32; i++) { tmp = (tmp << 1) | (val & 1); val >>= 1; }
        vm->registers[reg] = tmp;
        break;

    case OP_BITROTATE:
        dst = vmfetchbyte(vm); src = vmfetchbyte(vm);
        vm->registers[dst] = rotl32(vm->registers[dst], vm->registers[src]);
        break;

    case OP_BITEXTRACT:
        dst = vmfetchbyte(vm); src = vmfetchbyte(vm); reg = vmfetchbyte(vm);
        a = vm->registers[src] & 31;
        b = vm->registers[reg] & 31;
        if (b == 0) vm->registers[dst] = 0;
        else vm->registers[dst] = (vm->registers[dst] >> a) & ((1u << b) - 1);
        break;

    case OP_BITINSERT:
        dst = vmfetchbyte(vm); src = vmfetchbyte(vm); reg = vmfetchbyte(vm);
        a = vm->registers[reg] & 31;
        b = vmfetchbyte(vm);
        val = vm->registers[b] & 31;
        if (val > 0) {
            uint32_t mask = ((1u << val) - 1) << a;
            vm->registers[dst] = (vm->registers[dst] & ~mask) | ((vm->registers[src] << a) & mask);
        }
        break;

    case OP_BITMERGE:
        dst = vmfetchbyte(vm); src = vmfetchbyte(vm); reg = vmfetchbyte(vm);
        vm->registers[dst] = (vm->registers[dst] & vm->registers[reg]) |
                             (vm->registers[src] & ~vm->registers[reg]);
        break;

    case OP_BITPACK:
        dst = vmfetchbyte(vm); src = vmfetchbyte(vm);
        a = vm->registers[dst]; b = vm->registers[src];
        vm->registers[dst] = (a & 0xFFFF) | ((b & 0xFFFF) << 16);
        break;

    case OP_BITUNPACK:
        dst = vmfetchbyte(vm); src = vmfetchbyte(vm);
        vm->registers[dst] = vm->registers[src] & 0xFFFF;
        vm->registers[(dst + 1) & 0xFF] = (vm->registers[src] >> 16) & 0xFFFF;
        break;

    case OP_BITSPLIT:
        dst = vmfetchbyte(vm); src = vmfetchbyte(vm);
        vm->registers[dst] = vm->registers[src] & 0x55555555;
        vm->registers[(dst + 1) & 0xFF] = (vm->registers[src] >> 1) & 0x55555555;
        break;

    case OP_SATURATE:
        dst = vmfetchbyte(vm); src = vmfetchbyte(vm);
        if (vm->registers[dst] > vm->registers[src])
            vm->registers[dst] = vm->registers[src];
        break;

    case OP_CLAMP:
        dst = vmfetchbyte(vm); src = vmfetchbyte(vm); reg = vmfetchbyte(vm);
        if (vm->registers[dst] < vm->registers[src]) vm->registers[dst] = vm->registers[src];
        if (vm->registers[dst] > vm->registers[reg]) vm->registers[dst] = vm->registers[reg];
        break;

    case OP_LERP:
        dst = vmfetchbyte(vm); src = vmfetchbyte(vm); reg = vmfetchbyte(vm);
        a = vm->registers[dst]; b = vm->registers[src]; val = vm->registers[reg];
        vm->registers[dst] = a + ((b - a) * val) / 256;
        break;


    case OP_SMOOTHSTEP:
        dst = vmfetchbyte(vm); src = vmfetchbyte(vm); reg = vmfetchbyte(vm);
        a = vm->registers[src]; b = vm->registers[reg]; val = vm->registers[dst];
        if (val <= a) vm->registers[dst] = 0;
        else if (val >= b) vm->registers[dst] = 256;
        else { uint32_t t = ((val - a) * 256) / (b - a);
            vm->registers[dst] = (t * t * (768 - 2 * t)) / (256 * 256); }
        break;

    case OP_NOISE:
        reg = vmfetchbyte(vm);
        vm->registers[reg] = (uint32_t)rand();
        break;

    case OP_PERLIN:
        reg = vmfetchbyte(vm);
        val = vm->registers[reg];
        vm->registers[reg] = ((val * 1103515245 + 12345) >> 16) & 0xFF;
        break;

    case OP_SIMPLEX:
        reg = vmfetchbyte(vm);
        val = vm->registers[reg];
        vm->registers[reg] = ((val * 6364136223846793005ULL + 1442695040888963407ULL) >> 33) & 0xFF;
        break;

    case OP_VORONOI:
        dst = vmfetchbyte(vm); src = vmfetchbyte(vm);
        a = vm->registers[dst]; b = vm->registers[src];
        vm->registers[dst] = (a * a + b * b) % 256;
        break;

    case OP_FRACTAL:
        reg = vmfetchbyte(vm);
        val = vm->registers[reg]; tmp = 0;
        for (int i = 0; i < 4; i++) {
            tmp += ((val * (1103515245 + i) + 12345) >> 16) & 0xFF;
        }
        vm->registers[reg] = tmp / 4;
        break;

    case OP_MANDELBROT:
        dst = vmfetchbyte(vm); src = vmfetchbyte(vm); {
            int32_t cx = (int32_t)vm->registers[dst] - 128;
            int32_t cy = (int32_t)vm->registers[src] - 128;
            int32_t zx = 0, zy = 0; uint32_t iter = 0;
            while (zx * zx + zy * zy < 16384 && iter < 255) {
                int32_t tx = (zx * zx - zy * zy) / 128 + cx;
                zy = (2 * zx * zy) / 128 + cy;
                zx = tx; iter++;
            }
            vm->registers[dst] = iter;
        } break;

    case OP_JULIA:
        dst = vmfetchbyte(vm); src = vmfetchbyte(vm); {
            int32_t zx = (int32_t)vm->registers[dst] - 128;
            int32_t zy = (int32_t)vm->registers[src] - 128;
            int32_t cx = -100, cy = 0; uint32_t iter = 0;
            while (zx * zx + zy * zy < 16384 && iter < 255) {
                int32_t tx = (zx * zx - zy * zy) / 128 + cx;
                zy = (2 * zx * zy) / 128 + cy;
                zx = tx; iter++;
            }
            vm->registers[dst] = iter;
        } break;

    case OP_FFT:
        reg = vmfetchbyte(vm);
        break;

    case OP_IFFT:
        reg = vmfetchbyte(vm);
        break;

    case OP_DCT:
        reg = vmfetchbyte(vm);
        break;

    case OP_IDCT:
        reg = vmfetchbyte(vm);
        break;


    case OP_CONVOLVE:
        dst = vmfetchbyte(vm); src = vmfetchbyte(vm); reg = vmfetchbyte(vm); {
            uint32_t out = vm->registers[dst];
            uint32_t data = vm->registers[src];
            uint32_t kern = vm->registers[reg];
            uint32_t ksize = vmfetchbyte(vm);
            uint32_t len = vmfetchbyte(vm);
            for (uint32_t i = 0; i < len && out + i < MEMSIZE; i++) {
                int32_t sum = 0;
                for (uint32_t k = 0; k < ksize; k++) {
                    int32_t idx = (int32_t)i - (int32_t)(ksize / 2) + (int32_t)k;
                    if (idx >= 0 && (uint32_t)idx < len)
                        sum += (int32_t)vmreadbyte(vm, data + idx) * (int8_t)vmreadbyte(vm, kern + k);
                }
                vmwritebyte(vm, out + i, (uint8_t)(sum > 255 ? 255 : (sum < 0 ? 0 : sum)));
            }
        } break;

    case OP_CORRELATE:
        dst = vmfetchbyte(vm); src = vmfetchbyte(vm);
        break;

    case OP_FILTER:
        dst = vmfetchbyte(vm); src = vmfetchbyte(vm); reg = vmfetchbyte(vm);
        break;

    case OP_BLUR:
        dst = vmfetchbyte(vm); src = vmfetchbyte(vm); reg = vmfetchbyte(vm); {
            uint32_t out = vm->registers[dst];
            uint32_t data = vm->registers[src];
            uint32_t len = vm->registers[reg];
            for (uint32_t i = 0; i < len && out + i < MEMSIZE; i++) {
                uint32_t sum = 0; uint32_t cnt = 0;
                for (int k = -1; k <= 1; k++) {
                    int32_t idx = (int32_t)i + k;
                    if (idx >= 0 && (uint32_t)idx < len) {
                        sum += vmreadbyte(vm, data + idx); cnt++;
                    }
                }
                vmwritebyte(vm, out + i, (uint8_t)(sum / cnt));
            }
        } break;

    case OP_SHARPEN:
        dst = vmfetchbyte(vm); src = vmfetchbyte(vm); reg = vmfetchbyte(vm); {
            uint32_t out = vm->registers[dst];
            uint32_t data = vm->registers[src];
            uint32_t len = vm->registers[reg];
            for (uint32_t i = 0; i < len && out + i < MEMSIZE; i++) {
                int32_t center = (int32_t)vmreadbyte(vm, data + i) * 3;
                if (i > 0) center -= (int32_t)vmreadbyte(vm, data + i - 1);
                if (i + 1 < len) center -= (int32_t)vmreadbyte(vm, data + i + 1);
                vmwritebyte(vm, out + i, (uint8_t)(center > 255 ? 255 : (center < 0 ? 0 : center)));
            }
        } break;

    case OP_EDGE:
        dst = vmfetchbyte(vm); src = vmfetchbyte(vm); reg = vmfetchbyte(vm); {
            uint32_t out = vm->registers[dst];
            uint32_t data = vm->registers[src];
            uint32_t len = vm->registers[reg];
            for (uint32_t i = 0; i < len && out + i < MEMSIZE; i++) {
                int32_t diff = 0;
                if (i + 1 < len) diff = (int32_t)vmreadbyte(vm, data + i + 1) - (int32_t)vmreadbyte(vm, data + i);
                if (diff < 0) diff = -diff;
                vmwritebyte(vm, out + i, (uint8_t)(diff > 255 ? 255 : diff));
            }
        } break;

    case OP_DILATE:
        dst = vmfetchbyte(vm); src = vmfetchbyte(vm); reg = vmfetchbyte(vm); {
            uint32_t out = vm->registers[dst];
            uint32_t data = vm->registers[src];
            uint32_t len = vm->registers[reg];
            for (uint32_t i = 0; i < len && out + i < MEMSIZE; i++) {
                uint8_t mx = vmreadbyte(vm, data + i);
                if (i > 0 && vmreadbyte(vm, data + i - 1) > mx) mx = vmreadbyte(vm, data + i - 1);
                if (i + 1 < len && vmreadbyte(vm, data + i + 1) > mx) mx = vmreadbyte(vm, data + i + 1);
                vmwritebyte(vm, out + i, mx);
            }
        } break;

    case OP_ERODE:
        dst = vmfetchbyte(vm); src = vmfetchbyte(vm); reg = vmfetchbyte(vm); {
            uint32_t out = vm->registers[dst];
            uint32_t data = vm->registers[src];
            uint32_t len = vm->registers[reg];
            for (uint32_t i = 0; i < len && out + i < MEMSIZE; i++) {
                uint8_t mn = vmreadbyte(vm, data + i);
                if (i > 0 && vmreadbyte(vm, data + i - 1) < mn) mn = vmreadbyte(vm, data + i - 1);
                if (i + 1 < len && vmreadbyte(vm, data + i + 1) < mn) mn = vmreadbyte(vm, data + i + 1);
                vmwritebyte(vm, out + i, mn);
            }
        } break;


    case OP_MORPHOPEN:
        dst = vmfetchbyte(vm); src = vmfetchbyte(vm); reg = vmfetchbyte(vm);
        break;

    case OP_MORPHCLOSE:
        dst = vmfetchbyte(vm); src = vmfetchbyte(vm); reg = vmfetchbyte(vm);
        break;

    case OP_THRESHOLD:
        dst = vmfetchbyte(vm); src = vmfetchbyte(vm); reg = vmfetchbyte(vm); {
            uint32_t out = vm->registers[dst];
            uint32_t data = vm->registers[src];
            uint32_t len = vm->registers[reg];
            uint8_t thresh = vmfetchbyte(vm);
            for (uint32_t i = 0; i < len && out + i < MEMSIZE; i++)
                vmwritebyte(vm, out + i, vmreadbyte(vm, data + i) >= thresh ? 255 : 0);
        } break;

    case OP_QUANTIZE:
        dst = vmfetchbyte(vm); src = vmfetchbyte(vm); reg = vmfetchbyte(vm); {
            uint32_t out = vm->registers[dst];
            uint32_t data = vm->registers[src];
            uint32_t len = vm->registers[reg];
            uint8_t levels = vmfetchbyte(vm);
            if (levels == 0) levels = 1;
            for (uint32_t i = 0; i < len && out + i < MEMSIZE; i++) {
                uint8_t v = vmreadbyte(vm, data + i);
                vmwritebyte(vm, out + i, (v / (256 / levels)) * (256 / levels));
            }
        } break;

    case OP_DITHER:
        dst = vmfetchbyte(vm); src = vmfetchbyte(vm); reg = vmfetchbyte(vm);
        break;

    case OP_GAMMA:
        dst = vmfetchbyte(vm); src = vmfetchbyte(vm); reg = vmfetchbyte(vm); {
            uint32_t out = vm->registers[dst];
            uint32_t data = vm->registers[src];
            uint32_t len = vm->registers[reg];
            double g = (double)vmfetchbyte(vm) / 100.0;
            if (g <= 0) g = 1.0;
            for (uint32_t i = 0; i < len && out + i < MEMSIZE; i++) {
                double v = vmreadbyte(vm, data + i) / 255.0;
                vmwritebyte(vm, out + i, (uint8_t)(pow(v, 1.0/g) * 255.0));
            }
        } break;

    case OP_CONTRAST:
        dst = vmfetchbyte(vm); src = vmfetchbyte(vm); reg = vmfetchbyte(vm); {
            uint32_t out = vm->registers[dst];
            uint32_t data = vm->registers[src];
            uint32_t len = vm->registers[reg];
            int32_t factor = (int32_t)(int8_t)vmfetchbyte(vm);
            for (uint32_t i = 0; i < len && out + i < MEMSIZE; i++) {
                int32_t v = vmreadbyte(vm, data + i) - 128;
                v = (v * (128 + factor)) / 128 + 128;
                vmwritebyte(vm, out + i, (uint8_t)(v > 255 ? 255 : (v < 0 ? 0 : v)));
            }
        } break;

    case OP_BRIGHTNESS:
        dst = vmfetchbyte(vm); src = vmfetchbyte(vm); reg = vmfetchbyte(vm); {
            uint32_t out = vm->registers[dst];
            uint32_t data = vm->registers[src];
            uint32_t len = vm->registers[reg];
            int32_t adj = (int32_t)(int8_t)vmfetchbyte(vm);
            for (uint32_t i = 0; i < len && out + i < MEMSIZE; i++) {
                int32_t v = vmreadbyte(vm, data + i) + adj;
                vmwritebyte(vm, out + i, (uint8_t)(v > 255 ? 255 : (v < 0 ? 0 : v)));
            }
        } break;

    case OP_INVERT:
        dst = vmfetchbyte(vm); src = vmfetchbyte(vm); reg = vmfetchbyte(vm); {
            uint32_t out = vm->registers[dst];
            uint32_t data = vm->registers[src];
            uint32_t len = vm->registers[reg];
            for (uint32_t i = 0; i < len && out + i < MEMSIZE; i++)
                vmwritebyte(vm, out + i, 255 - vmreadbyte(vm, data + i));
        } break;


    case OP_GRAYSCALE:
        dst = vmfetchbyte(vm); src = vmfetchbyte(vm); reg = vmfetchbyte(vm); {
            uint32_t out = vm->registers[dst];
            uint32_t data = vm->registers[src];
            uint32_t pixels = vm->registers[reg];
            for (uint32_t i = 0; i < pixels && data + i * 3 + 2 < MEMSIZE; i++) {
                uint8_t r = vmreadbyte(vm, data + i * 3);
                uint8_t g = vmreadbyte(vm, data + i * 3 + 1);
                uint8_t bv = vmreadbyte(vm, data + i * 3 + 2);
                vmwritebyte(vm, out + i, (uint8_t)((r * 77 + g * 150 + bv * 29) >> 8));
            }
        } break;

    case OP_SEPIA:
        dst = vmfetchbyte(vm); src = vmfetchbyte(vm); reg = vmfetchbyte(vm); {
            uint32_t out = vm->registers[dst];
            uint32_t data = vm->registers[src];
            uint32_t pixels = vm->registers[reg];
            for (uint32_t i = 0; i < pixels && data + i * 3 + 2 < MEMSIZE; i++) {
                uint8_t r = vmreadbyte(vm, data + i * 3);
                uint8_t g = vmreadbyte(vm, data + i * 3 + 1);
                uint8_t bv = vmreadbyte(vm, data + i * 3 + 2);
                int32_t nr = (r * 393 + g * 769 + bv * 189) >> 10;
                int32_t ng = (r * 349 + g * 686 + bv * 168) >> 10;
                int32_t nb = (r * 272 + g * 534 + bv * 131) >> 10;
                vmwritebyte(vm, out + i * 3, (uint8_t)(nr > 255 ? 255 : nr));
                vmwritebyte(vm, out + i * 3 + 1, (uint8_t)(ng > 255 ? 255 : ng));
                vmwritebyte(vm, out + i * 3 + 2, (uint8_t)(nb > 255 ? 255 : nb));
            }
        } break;

    case OP_HUE:
        reg = vmfetchbyte(vm);
        break;

    case OP_SATURATION:
        reg = vmfetchbyte(vm);
        break;

    case OP_LUMINANCE:
        dst = vmfetchbyte(vm); src = vmfetchbyte(vm); reg = vmfetchbyte(vm); {
            uint8_t r = vmreadbyte(vm, vm->registers[src]);
            uint8_t g = vmreadbyte(vm, vm->registers[src] + 1);
            uint8_t bv = vmreadbyte(vm, vm->registers[src] + 2);
            vm->registers[dst] = (r * 77 + g * 150 + bv * 29) >> 8;
        } break;

    case OP_BLEND:
        dst = vmfetchbyte(vm); src = vmfetchbyte(vm); reg = vmfetchbyte(vm); {
            uint32_t alpha = vm->registers[reg];
            vm->registers[dst] = (vm->registers[dst] * (256 - alpha) + vm->registers[src] * alpha) / 256;
        } break;

    case OP_ALPHA: case OP_COMPOSITE: case OP_PREMULTIPLY: case OP_UNPREMULTIPLY:
    case OP_RGBTOHSL: case OP_HSLTORGB: case OP_RGBTOHSV: case OP_HSVTORGB:
    case OP_RGBTOYUV: case OP_YUVTORGB: case OP_RGBTOCMYK: case OP_CMYKTORGB:
        dst = vmfetchbyte(vm); src = vmfetchbyte(vm);
        break;


    case OP_DOTPRODUCT:
        dst = vmfetchbyte(vm); src = vmfetchbyte(vm); reg = vmfetchbyte(vm); {
            uint32_t va = vm->registers[dst];
            uint32_t vb = vm->registers[src];
            uint32_t len = vm->registers[reg];
            int32_t sum = 0;
            for (uint32_t i = 0; i < len; i++)
                sum += (int32_t)vmreadword(vm, va + i * 4) * (int32_t)vmreadword(vm, vb + i * 4);
            vm->registers[dst] = (uint32_t)sum;
        } break;

    case OP_CROSSPRODUCT:
        dst = vmfetchbyte(vm); src = vmfetchbyte(vm); reg = vmfetchbyte(vm); {
            uint32_t va = vm->registers[dst];
            uint32_t vb = vm->registers[src];
            uint32_t out = vm->registers[reg];
            int32_t ax = (int32_t)vmreadword(vm, va);
            int32_t ay = (int32_t)vmreadword(vm, va + 4);
            int32_t az = (int32_t)vmreadword(vm, va + 8);
            int32_t bx = (int32_t)vmreadword(vm, vb);
            int32_t by = (int32_t)vmreadword(vm, vb + 4);
            int32_t bz = (int32_t)vmreadword(vm, vb + 8);
            vmwriteword(vm, out, (uint32_t)(ay * bz - az * by));
            vmwriteword(vm, out + 4, (uint32_t)(az * bx - ax * bz));
            vmwriteword(vm, out + 8, (uint32_t)(ax * by - ay * bx));
        } break;

    case OP_MAGNITUDE:
        dst = vmfetchbyte(vm); src = vmfetchbyte(vm); {
            uint32_t va = vm->registers[dst];
            uint32_t len = vm->registers[src];
            double sum = 0;
            for (uint32_t i = 0; i < len; i++) {
                double v = (double)(int32_t)vmreadword(vm, va + i * 4);
                sum += v * v;
            }
            vm->registers[dst] = (uint32_t)(int32_t)sqrt(sum);
        } break;

    case OP_NORMALIZE:
        dst = vmfetchbyte(vm); src = vmfetchbyte(vm); {
            uint32_t va = vm->registers[dst];
            uint32_t len = vm->registers[src];
            double sum = 0;
            for (uint32_t i = 0; i < len; i++) {
                double v = (double)(int32_t)vmreadword(vm, va + i * 4);
                sum += v * v;
            }
            double mag = sqrt(sum);
            if (mag > 0) {
                for (uint32_t i = 0; i < len; i++) {
                    double v = (double)(int32_t)vmreadword(vm, va + i * 4);
                    vmwriteword(vm, va + i * 4, (uint32_t)(int32_t)((v / mag) * 1000.0));
                }
            }
        } break;

    case OP_DISTANCE:
        dst = vmfetchbyte(vm); src = vmfetchbyte(vm); reg = vmfetchbyte(vm); {
            uint32_t va = vm->registers[dst];
            uint32_t vb = vm->registers[src];
            uint32_t len = vm->registers[reg];
            double sum = 0;
            for (uint32_t i = 0; i < len; i++) {
                double d = (double)(int32_t)vmreadword(vm, va + i*4) - (double)(int32_t)vmreadword(vm, vb + i*4);
                sum += d * d;
            }
            vm->registers[dst] = (uint32_t)(int32_t)sqrt(sum);
        } break;

    case OP_REFLECT: case OP_REFRACT: case OP_PROJECT: case OP_REJECT:
        dst = vmfetchbyte(vm); src = vmfetchbyte(vm);
        break;


    case OP_MATMUL:
        dst = vmfetchbyte(vm); src = vmfetchbyte(vm); reg = vmfetchbyte(vm); {
            uint32_t out = vm->registers[dst];
            uint32_t ma = vm->registers[src];
            uint32_t mb = vm->registers[reg];
            uint8_t n = vmfetchbyte(vm);
            for (uint32_t i = 0; i < n; i++)
                for (uint32_t j = 0; j < n; j++) {
                    int32_t sum = 0;
                    for (uint32_t k = 0; k < n; k++)
                        sum += (int32_t)vmreadword(vm, ma + (i*n+k)*4) * (int32_t)vmreadword(vm, mb + (k*n+j)*4);
                    vmwriteword(vm, out + (i*n+j)*4, (uint32_t)sum);
                }
        } break;

    case OP_MATINV:
        dst = vmfetchbyte(vm); src = vmfetchbyte(vm);
        break;

    case OP_MATTRANS:
        dst = vmfetchbyte(vm); src = vmfetchbyte(vm); reg = vmfetchbyte(vm); {
            uint32_t out = vm->registers[dst];
            uint32_t ma = vm->registers[src];
            uint8_t n = vm->registers[reg] & 0xFF;
            for (uint32_t i = 0; i < n; i++)
                for (uint32_t j = 0; j < n; j++)
                    vmwriteword(vm, out + (j*n+i)*4, vmreadword(vm, ma + (i*n+j)*4));
        } break;

    case OP_MATDET:
        dst = vmfetchbyte(vm); src = vmfetchbyte(vm); {
            uint32_t ma = vm->registers[src];
            uint8_t n = vmfetchbyte(vm);
            if (n == 2) {
                int32_t a2 = (int32_t)vmreadword(vm, ma);
                int32_t b2 = (int32_t)vmreadword(vm, ma+4);
                int32_t c2 = (int32_t)vmreadword(vm, ma+8);
                int32_t d2 = (int32_t)vmreadword(vm, ma+12);
                vm->registers[dst] = (uint32_t)(a2*d2 - b2*c2);
            } else vm->registers[dst] = 0;
        } break;

    case OP_MATTRACE:
        dst = vmfetchbyte(vm); src = vmfetchbyte(vm); {
            uint32_t ma = vm->registers[src];
            uint8_t n = vmfetchbyte(vm);
            int32_t trace = 0;
            for (uint32_t i = 0; i < n; i++)
                trace += (int32_t)vmreadword(vm, ma + (i*n+i)*4);
            vm->registers[dst] = (uint32_t)trace;
        } break;

    case OP_EIGENVAL: case OP_EIGENVEC: case OP_LU: case OP_QR:
    case OP_SVD: case OP_CHOLESKY: case OP_LEASTSQUARES:
        dst = vmfetchbyte(vm); src = vmfetchbyte(vm);
        break;

    case OP_SOLVE:
        dst = vmfetchbyte(vm); src = vmfetchbyte(vm); reg = vmfetchbyte(vm);
        break;

    case OP_INTERPOLATE:
        dst = vmfetchbyte(vm); src = vmfetchbyte(vm); reg = vmfetchbyte(vm);
        a = vm->registers[dst]; b = vm->registers[src]; val = vm->registers[reg];
        vm->registers[dst] = a + ((b - a) * val) / 256;
        break;

    case OP_EXTRAPOLATE:
        dst = vmfetchbyte(vm); src = vmfetchbyte(vm); reg = vmfetchbyte(vm);
        a = vm->registers[dst]; b = vm->registers[src]; val = vm->registers[reg];
        vm->registers[dst] = b + ((b - a) * val) / 256;
        break;

    case OP_DIFFERENTIATE:
        dst = vmfetchbyte(vm); src = vmfetchbyte(vm); {
            uint32_t data = vm->registers[dst];
            uint32_t len = vm->registers[src];
            for (uint32_t i = 0; i + 1 < len; i++)
                vmwriteword(vm, data + i*4,
                    vmreadword(vm, data + (i+1)*4) - vmreadword(vm, data + i*4));
        } break;

    case OP_INTEGRATE:
        dst = vmfetchbyte(vm); src = vmfetchbyte(vm); {
            uint32_t data = vm->registers[dst];
            uint32_t len = vm->registers[src];
            uint32_t acc = 0;
            for (uint32_t i = 0; i < len; i++) {
                acc += vmreadword(vm, data + i*4);
                vmwriteword(vm, data + i*4, acc);
            }
        } break;

    case OP_GRADIENT: case OP_DIVERGENCE: case OP_CURL:
    case OP_LAPLACIAN: case OP_HESSIAN: case OP_JACOBIAN:
        dst = vmfetchbyte(vm); src = vmfetchbyte(vm);
        break;


    case OP_OPTIMIZE: case OP_MINIMIZE: case OP_MAXIMIZE:
        dst = vmfetchbyte(vm); src = vmfetchbyte(vm);
        break;

    case OP_SEARCH:
        dst = vmfetchbyte(vm); src = vmfetchbyte(vm); reg = vmfetchbyte(vm); {
            uint32_t data = vm->registers[dst];
            uint32_t len = vm->registers[src];
            uint32_t needle = vm->registers[reg];
            vm->registers[dst] = (uint32_t)-1;
            for (uint32_t i = 0; i < len; i++) {
                if (vmreadword(vm, data + i*4) == needle) {
                    vm->registers[dst] = i; break;
                }
            }
        } break;

    case OP_SORT:
        dst = vmfetchbyte(vm); src = vmfetchbyte(vm); {
            uint32_t data = vm->registers[dst];
            uint32_t len = vm->registers[src];
            for (uint32_t i = 0; i < len; i++)
                for (uint32_t j = i + 1; j < len; j++) {
                    uint32_t ai = vmreadword(vm, data + i*4);
                    uint32_t aj = vmreadword(vm, data + j*4);
                    if (aj < ai) {
                        vmwriteword(vm, data + i*4, aj);
                        vmwriteword(vm, data + j*4, ai);
                    }
                }
        } break;

    case OP_SHUFFLE:
        dst = vmfetchbyte(vm); src = vmfetchbyte(vm); {
            uint32_t data = vm->registers[dst];
            uint32_t len = vm->registers[src];
            for (uint32_t i = len; i > 1; i--) {
                uint32_t j = rand() % i;
                uint32_t ai = vmreadword(vm, data + (i-1)*4);
                uint32_t aj = vmreadword(vm, data + j*4);
                vmwriteword(vm, data + (i-1)*4, aj);
                vmwriteword(vm, data + j*4, ai);
            }
        } break;

    case OP_REVERSE:
        dst = vmfetchbyte(vm); src = vmfetchbyte(vm); {
            uint32_t data = vm->registers[dst];
            uint32_t len = vm->registers[src];
            for (uint32_t i = 0; i < len / 2; i++) {
                uint32_t ai = vmreadword(vm, data + i*4);
                uint32_t aj = vmreadword(vm, data + (len-1-i)*4);
                vmwriteword(vm, data + i*4, aj);
                vmwriteword(vm, data + (len-1-i)*4, ai);
            }
        } break;

    case OP_ROTATE:
        dst = vmfetchbyte(vm); src = vmfetchbyte(vm); reg = vmfetchbyte(vm); {
            uint32_t data = vm->registers[dst];
            uint32_t len = vm->registers[src];
            uint32_t rot = vm->registers[reg] % len;
            for (uint32_t i = 0; i < rot; i++) {
                uint32_t first = vmreadword(vm, data);
                for (uint32_t j = 0; j + 1 < len; j++)
                    vmwriteword(vm, data + j*4, vmreadword(vm, data + (j+1)*4));
                vmwriteword(vm, data + (len-1)*4, first);
            }
        } break;

    case OP_SHIFT:
        dst = vmfetchbyte(vm); src = vmfetchbyte(vm); reg = vmfetchbyte(vm); {
            uint32_t data = vm->registers[dst];
            uint32_t len = vm->registers[src];
            uint32_t n = vm->registers[reg];
            for (uint32_t i = 0; i + n < len; i++)
                vmwriteword(vm, data + i*4, vmreadword(vm, data + (i+n)*4));
            for (uint32_t i = len - n; i < len; i++)
                vmwriteword(vm, data + i*4, 0);
        } break;

    case OP_PARTITION:
        dst = vmfetchbyte(vm); src = vmfetchbyte(vm); reg = vmfetchbyte(vm); {
            uint32_t data = vm->registers[dst];
            uint32_t len = vm->registers[src];
            uint32_t pivot = vm->registers[reg];
            uint32_t lo = 0;
            for (uint32_t i = 0; i < len; i++) {
                if (vmreadword(vm, data + i*4) < pivot) {
                    uint32_t ai = vmreadword(vm, data + i*4);
                    uint32_t al = vmreadword(vm, data + lo*4);
                    vmwriteword(vm, data + i*4, al);
                    vmwriteword(vm, data + lo*4, ai);
                    lo++;
                }
            }
            vm->registers[dst] = lo;
        } break;


    case OP_MERGE:
        dst = vmfetchbyte(vm); src = vmfetchbyte(vm); reg = vmfetchbyte(vm);
        break;

    case OP_SPLIT:
        dst = vmfetchbyte(vm); src = vmfetchbyte(vm); reg = vmfetchbyte(vm);
        break;

    case OP_ZIP:
        dst = vmfetchbyte(vm); src = vmfetchbyte(vm); reg = vmfetchbyte(vm);
        break;

    case OP_UNZIP:
        dst = vmfetchbyte(vm); src = vmfetchbyte(vm); reg = vmfetchbyte(vm);
        break;

    case OP_FLATTEN:
        dst = vmfetchbyte(vm); src = vmfetchbyte(vm);
        break;

    case OP_RESHAPE:
        dst = vmfetchbyte(vm); src = vmfetchbyte(vm);
        break;

    case OP_TRANSPOSE:
        dst = vmfetchbyte(vm); src = vmfetchbyte(vm); reg = vmfetchbyte(vm); {
            uint32_t data = vm->registers[dst];
            uint32_t out = vm->registers[src];
            uint32_t n = vm->registers[reg];
            for (uint32_t i = 0; i < n; i++)
                for (uint32_t j = 0; j < n; j++)
                    vmwriteword(vm, out + (j*n+i)*4, vmreadword(vm, data + (i*n+j)*4));
        } break;

    case OP_BROADCAST:
        dst = vmfetchbyte(vm); src = vmfetchbyte(vm); reg = vmfetchbyte(vm); {
            uint32_t data = vm->registers[dst];
            uint32_t len = vm->registers[src];
            uint32_t v = vm->registers[reg];
            for (uint32_t i = 0; i < len; i++)
                vmwriteword(vm, data + i*4, v);
        } break;

    case OP_REDUCE:
        dst = vmfetchbyte(vm); src = vmfetchbyte(vm); {
            uint32_t data = vm->registers[dst];
            uint32_t len = vm->registers[src];
            uint32_t sum = 0;
            for (uint32_t i = 0; i < len; i++)
                sum += vmreadword(vm, data + i*4);
            vm->registers[dst] = sum;
        } break;

    case OP_SCAN:
        dst = vmfetchbyte(vm); src = vmfetchbyte(vm); {
            uint32_t data = vm->registers[dst];
            uint32_t len = vm->registers[src];
            uint32_t acc = 0;
            for (uint32_t i = 0; i < len; i++) {
                acc += vmreadword(vm, data + i*4);
                vmwriteword(vm, data + i*4, acc);
            }
        } break;

    case OP_GATHER:
        dst = vmfetchbyte(vm); src = vmfetchbyte(vm); reg = vmfetchbyte(vm); {
            uint32_t out = vm->registers[dst];
            uint32_t data = vm->registers[src];
            uint32_t idx = vm->registers[reg];
            uint32_t len = vmfetchbyte(vm);
            for (uint32_t i = 0; i < len; i++) {
                uint32_t ix = vmreadword(vm, idx + i*4);
                vmwriteword(vm, out + i*4, vmreadword(vm, data + ix*4));
            }
        } break;

    case OP_SCATTER:
        dst = vmfetchbyte(vm); src = vmfetchbyte(vm); reg = vmfetchbyte(vm); {
            uint32_t data = vm->registers[dst];
            uint32_t vals = vm->registers[src];
            uint32_t idx = vm->registers[reg];
            uint32_t len = vmfetchbyte(vm);
            for (uint32_t i = 0; i < len; i++) {
                uint32_t ix = vmreadword(vm, idx + i*4);
                vmwriteword(vm, data + ix*4, vmreadword(vm, vals + i*4));
            }
        } break;

    case OP_COMPRESS: case OP_EXPAND: case OP_ENCODE: case OP_DECODE:
    case OP_ENCRYPT: case OP_DECRYPT: case OP_SIGN: case OP_VERIFY:
        dst = vmfetchbyte(vm); src = vmfetchbyte(vm);
        break;


    case OP_CHECKSUM:
        dst = vmfetchbyte(vm); src = vmfetchbyte(vm); {
            uint32_t data = vm->registers[dst];
            uint32_t len = vm->registers[src];
            uint32_t sum = 0;
            for (uint32_t i = 0; i < len && data + i < MEMSIZE; i++)
                sum += vmreadbyte(vm, data + i);
            vm->registers[dst] = sum;
        } break;

    case OP_ADLER:
        dst = vmfetchbyte(vm); src = vmfetchbyte(vm); {
            uint32_t data = vm->registers[dst];
            uint32_t len = vm->registers[src];
            uint32_t a2 = 1, b2 = 0;
            for (uint32_t i = 0; i < len && data + i < MEMSIZE; i++) {
                a2 = (a2 + vmreadbyte(vm, data + i)) % 65521;
                b2 = (b2 + a2) % 65521;
            }
            vm->registers[dst] = (b2 << 16) | a2;
        } break;

    case OP_FLETCHER:
        dst = vmfetchbyte(vm); src = vmfetchbyte(vm); {
            uint32_t data = vm->registers[dst];
            uint32_t len = vm->registers[src];
            uint16_t s1 = 0, s2 = 0;
            for (uint32_t i = 0; i < len && data + i < MEMSIZE; i++) {
                s1 = (s1 + vmreadbyte(vm, data + i)) % 255;
                s2 = (s2 + s1) % 255;
            }
            vm->registers[dst] = (s2 << 8) | s1;
        } break;

    case OP_MD5: case OP_SHA1: case OP_SHA256: case OP_SHA512:
    case OP_BLAKE2: case OP_BLAKE3: case OP_XXHASH: case OP_MURMUR:
    case OP_CITYHASH: case OP_FARMHASH: case OP_METROHASH:
    case OP_HIGHWAYHASH: case OP_SPOOKYHASH: case OP_SIPHASH: case OP_WYHASH:
        dst = vmfetchbyte(vm); src = vmfetchbyte(vm); {
            uint32_t data = vm->registers[dst];
            uint32_t len = vm->registers[src];
            uint32_t h = 2166136261u;
            for (uint32_t i = 0; i < len && data + i < MEMSIZE; i++) {
                h ^= vmreadbyte(vm, data + i);
                h *= 16777619u;
            }
            vm->registers[dst] = h;
        } break;

    case OP_AES: case OP_CHACHA: case OP_SALSA: case OP_RC4:
    case OP_BLOWFISH: case OP_TWOFISH: case OP_SERPENT: case OP_CAMELLIA:
    case OP_ARIA: case OP_CLEFIA: case OP_SIMON: case OP_SPECK:
    case OP_PRESENT: case OP_TEA: case OP_XTEA: case OP_XXTEA:
    case OP_IDEA: case OP_SKIPJACK: case OP_MARS: case OP_RC5: case OP_RC6:
    case OP_CAST: case OP_SEED:
        dst = vmfetchbyte(vm); src = vmfetchbyte(vm); reg = vmfetchbyte(vm); {
            uint32_t data = vm->registers[dst];
            uint32_t key = vm->registers[src];
            uint32_t len = vm->registers[reg];
            for (uint32_t i = 0; i < len && data + i < MEMSIZE; i++) {
                uint8_t k = vmreadbyte(vm, key + (i % 16));
                vmwritebyte(vm, data + i, vmreadbyte(vm, data + i) ^ k);
            }
        } break;

    case OP_KECCAK: case OP_SHAKE: case OP_RIPEMD: case OP_WHIRLPOOL:
    case OP_TIGER: case OP_GOST: case OP_GOST94: case OP_SM3:
    case OP_STREEBOG: case OP_HAVAL: case OP_SNEFRU: case OP_GRINDAHL:
    case OP_JH: case OP_FUGUE: case OP_LUFFA: case OP_HAMSI:
    case OP_SHAVITE: case OP_ECHO: case OP_CUBEHASH: case OP_BMW:
    case OP_GROESTL: case OP_SKEIN: case OP_SHABAL: case OP_PANAMA:
    case OP_RADIOGATUN: case OP_THREEFISH:
        dst = vmfetchbyte(vm); src = vmfetchbyte(vm); {
            uint32_t data = vm->registers[dst];
            uint32_t len = vm->registers[src];
            uint32_t h = 2166136261u;
            for (uint32_t i = 0; i < len && data + i < MEMSIZE; i++) {
                h ^= vmreadbyte(vm, data + i);
                h *= 16777619u;
            }
            vm->registers[dst] = h;
        } break;


    case OP_BEZIER:
        dst = vmfetchbyte(vm); src = vmfetchbyte(vm); reg = vmfetchbyte(vm); {
            uint32_t p0 = vm->registers[dst];
            uint32_t p1 = vm->registers[src];
            uint32_t p2 = vm->registers[reg];
            uint32_t t = vmfetchbyte(vm);
            uint32_t it = 256 - t;
            vm->registers[dst] = (it * it * p0 + 2 * it * t * p1 + t * t * p2) / (256 * 256);
        } break;

    case OP_HERMITE:
        dst = vmfetchbyte(vm); src = vmfetchbyte(vm);
        break;

    case OP_SPLINE:
        dst = vmfetchbyte(vm); src = vmfetchbyte(vm);
        break;

    case OP_SIMD: case OP_BLENDPS: case OP_BLENDPD: case OP_BLENDVPS:
    case OP_BLENDVPD: case OP_DPPS: case OP_DPPD:
    case OP_INSERTPS: case OP_EXTRACTPS:
    case OP_PINSRB: case OP_PINSRD: case OP_PINSRQ: case OP_PINSRW:
    case OP_PEXTRB: case OP_PEXTRD: case OP_PEXTRQ: case OP_PEXTRW:
    case OP_PSHUFB: case OP_PSHUFD: case OP_PSHUFHW: case OP_PSHUFLW:
    case OP_PALIGNR: case OP_PBLENDW: case OP_PBLENDVB: case OP_PBLENDD:
    case OP_PMULLD:
    case OP_PMAXSB: case OP_PMAXSD: case OP_PMAXUD: case OP_PMAXUW:
    case OP_PMINSB: case OP_PMINSD: case OP_PMINUD: case OP_PMINUW:
    case OP_PMOVSXBW: case OP_PMOVSXBD: case OP_PMOVSXBQ:
    case OP_PMOVSXWD: case OP_PMOVSXWQ: case OP_PMOVSXDQ:
    case OP_PMOVZXBW: case OP_PMOVZXBD: case OP_PMOVZXBQ:
    case OP_PMOVZXWD: case OP_PMOVZXWQ: case OP_PMOVZXDQ:
    case OP_PACKUSDW: case OP_PCMPEQQ: case OP_PCMPGTQ:
    case OP_PHADDW: case OP_PHADDD: case OP_PHADDSW:
    case OP_PHSUBW: case OP_PHSUBD: case OP_PHSUBSW:
    case OP_PHMINPOSUW: case OP_PMADDUBSW:
    case OP_PSIGNB: case OP_PSIGNW: case OP_PSIGND:
    case OP_PABSB: case OP_PABSW: case OP_PABSD:
    case OP_PTEST: case OP_PCLMULQDQ: case OP_MPSADBW:
    case OP_PUNPCKLQDQ: case OP_PUNPCKHQDQ:
    case OP_ROUNDPS: case OP_ROUNDPD: case OP_ROUNDSS: case OP_ROUNDSD:
        dst = vmfetchbyte(vm); src = vmfetchbyte(vm);
        break;

    case OP_MOVNTDQ: case OP_MOVNTDQA: case OP_MOVNTI:
    case OP_MOVNTPD: case OP_MOVNTPS: case OP_MOVNTQ:
        dst = vmfetchbyte(vm); src = vmfetchbyte(vm);
        vmwriteword(vm, vm->registers[dst], vm->registers[src]);
        break;

    case OP_MOVDDUP: case OP_MOVSHDUP: case OP_MOVSLDUP:
        dst = vmfetchbyte(vm); src = vmfetchbyte(vm);
        vm->registers[dst] = vm->registers[src];
        break;

    case OP_LDDQU: case OP_MASKMOVDQU: case OP_MASKMOVQ:
        dst = vmfetchbyte(vm); src = vmfetchbyte(vm);
        vm->registers[dst] = vmreadword(vm, vm->registers[src]);
        break;

    case OP_LDMXCSR: case OP_STMXCSR:
    case OP_FXSAVE: case OP_FXRSTOR:
    case OP_XSAVE: case OP_XRSTOR: case OP_XSAVEOPT:
    case OP_XSAVEC: case OP_XSAVES: case OP_XRSTORS:
    case OP_XGETBV: case OP_XSETBV:
    case OP_VZEROALL: case OP_VZEROUPPER:
        break;

    case OP_CLFLUSH: case OP_CLFLUSHOPT: case OP_CLWB:
        reg = vmfetchbyte(vm);
        break;

    case OP_LFENCE: case OP_MFENCE: case OP_SFENCE:
        break;

    case OP_PREFETCHT0: case OP_PREFETCHT1: case OP_PREFETCHT2: case OP_PREFETCHW:
        reg = vmfetchbyte(vm);
        break;

    case OP_RDRAND: case OP_RDSEED:
        reg = vmfetchbyte(vm);
        vm->registers[reg] = (uint32_t)rand();
        break;

    case OP_RDTSCP:
        reg = vmfetchbyte(vm);
        vm->registers[reg] = vm->instrcount;
        break;

    case OP_SYSCALL:
        reg = vmfetchbyte(vm);
        break;

    default:
        break;
    }
}
