#include "core.h"
#include "opcodes.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef struct { uint16_t code; const char* name; int operands; } disinfo;

static const disinfo distable[] = {
    {OP_NOP, "nop", 0}, {OP_HLT, "hlt", 0},
    {OP_MOV, "mov", 2}, {OP_ADD, "add", 2}, {OP_SUB, "sub", 2},
    {OP_MUL, "mul", 2}, {OP_DIV, "div", 2}, {OP_AND, "and", 2},
    {OP_OR, "or", 2}, {OP_XOR, "xor", 2}, {OP_NOT, "not", 1},
    {OP_SHL, "shl", 2}, {OP_SHR, "shr", 2}, {OP_CMP, "cmp", 2},
    {OP_JMP, "jmp", -1}, {OP_JE, "je", -1}, {OP_JNE, "jne", -1},
    {OP_JG, "jg", -1}, {OP_JL, "jl", -1}, {OP_JGE, "jge", -1},
    {OP_JLE, "jle", -1}, {OP_JZ, "jz", -1}, {OP_JNZ, "jnz", -1},
    {OP_JC, "jc", -1}, {OP_JNC, "jnc", -1},
    {OP_CALL, "call", -1}, {OP_RET, "ret", 0},
    {OP_PUSH, "push", 1}, {OP_POP, "pop", 1},
    {OP_LOAD, "load", 2}, {OP_STORE, "store", 2},
    {OP_IN, "in", 1}, {OP_OUT, "out", 1},
    {OP_MOVIMM, "movimm", -2}, {OP_MOD, "mod", 2},
    {OP_INC, "inc", 1}, {OP_DEC, "dec", 1},
    {OP_SAR, "sar", 2}, {OP_ROL, "rol", 2}, {OP_ROR, "ror", 2},
    {OP_TEST, "test", 2},
    {OP_LOADBYTE, "loadbyte", 2}, {OP_STOREBYTE, "storebyte", 2},
    {OP_LOADREG, "loadreg", 2}, {OP_STOREREG, "storereg", 2},
    {OP_CLR, "clr", 1}, {OP_SWAP, "swap", 2},
    {OP_NEG, "neg", 1}, {OP_ABS, "abs", 1}, {OP_BSWAP, "bswap", 1},
    {OP_BTS, "bts", 2}, {OP_BTR, "btr", 2}, {OP_BTC, "btc", 2},
    {OP_BSF, "bsf", 1}, {OP_BSR, "bsr", 1}, {OP_POPCNT, "popcnt", 1},
    {OP_ADDIMM, "addimm", -2}, {OP_SUBIMM, "subimm", -2},
    {OP_MULIMM, "mulimm", -2}, {OP_ANDIMM, "andimm", -2},
    {OP_ORIMM, "orimm", -2}, {OP_XORIMM, "xorimm", -2},
    {OP_CMPIMM, "cmpimm", -2}, {OP_PUSHIMM, "pushimm", -3},
    {OP_DUP, "dup", 0}, {OP_DROP, "drop", 0}, {OP_OVER, "over", 0},
    {OP_ROT, "rot", 0}, {OP_PICK, "pick", 1},
    {OP_LOOP, "loop", -1}, {OP_LOOPE, "loope", -1}, {OP_LOOPNE, "loopne", -1},
    {OP_STRCPY, "strcpy", 2}, {OP_STRCMP, "strcmp", 2}, {OP_STRLEN, "strlen", 1},
    {OP_MEMSET, "memset", 3}, {OP_MEMCPY, "memcpy", 3},
    {OP_MIN, "min", 2}, {OP_MAX, "max", 2},
    {OP_SQRT, "sqrt", 1}, {OP_POW, "pow", 2},
    {OP_SIN, "sin", 1}, {OP_COS, "cos", 1}, {OP_TAN, "tan", 1},
    {OP_RAND, "rand", 1},
    {OP_FACTORIAL, "factorial", 1}, {OP_FIBONACCI, "fibonacci", 1},
    {OP_ISPRIME, "isprime", 1}, {OP_GCD, "gcd", 2},
    {OP_INC, "inc", 1}, {OP_DEC, "dec", 1},
    {0xFFFF, NULL, 0}
};


int vmdisassemble(const char* infile, const char* outfile) {
    FILE* f = fopen(infile, "rb");
    if (!f) { fprintf(stderr, "cannot open %s\n", infile); return 0; }

    fseek(f, 0, SEEK_END);
    long size = ftell(f);
    fseek(f, 0, SEEK_SET);

    uint8_t* data = malloc(size);
    fread(data, 1, size, f);
    fclose(f);

    FILE* out = fopen(outfile, "w");
    if (!out) { fprintf(stderr, "cannot create %s\n", outfile); free(data); return 0; }

    size_t pc = 0;
    int instrcount = 0;

    while (pc + 1 < (size_t)size) {
        uint16_t op = data[pc] | (data[pc + 1] << 8);
        pc += 2;

        if (op == OP_HLT) {
            fprintf(out, "hlt\n");
            instrcount++;
            break;
        }

        const disinfo* info = NULL;
        for (int i = 0; distable[i].name; i++) {
            if (distable[i].code == op) { info = &distable[i]; break; }
        }

        if (!info) {
            fprintf(out, "; unknown 0x%04X\n", op);
            instrcount++;
            continue;
        }

        if (info->operands == 0) {
            fprintf(out, "%s\n", info->name);
        } else if (info->operands == 1) {
            if (pc < (size_t)size) {
                fprintf(out, "%s %d\n", info->name, data[pc]);
                pc++;
            }
        } else if (info->operands == 2) {
            if (pc + 1 < (size_t)size) {
                fprintf(out, "%s %d, %d\n", info->name, data[pc], data[pc+1]);
                pc += 2;
            }
        } else if (info->operands == 3) {
            if (pc + 2 < (size_t)size) {
                fprintf(out, "%s %d, %d, %d\n", info->name, data[pc], data[pc+1], data[pc+2]);
                pc += 3;
            }
        } else if (info->operands == -1) {
            if (pc + 3 < (size_t)size) {
                uint32_t addr = data[pc] | (data[pc+1] << 8) | (data[pc+2] << 16) | (data[pc+3] << 24);
                fprintf(out, "%s %u\n", info->name, addr);
                pc += 4;
            }
        } else if (info->operands == -2) {
            if (pc + 4 < (size_t)size) {
                uint8_t reg = data[pc]; pc++;
                uint32_t val = data[pc] | (data[pc+1] << 8) | (data[pc+2] << 16) | (data[pc+3] << 24);
                fprintf(out, "%s %d, %u\n", info->name, reg, val);
                pc += 4;
            }
        } else if (info->operands == -3) {
            if (pc + 3 < (size_t)size) {
                uint32_t val = data[pc] | (data[pc+1] << 8) | (data[pc+2] << 16) | (data[pc+3] << 24);
                fprintf(out, "%s %u\n", info->name, val);
                pc += 4;
            }
        }
        instrcount++;
    }

    fclose(out);
    printf("disassembled %d instructions to %s\n", instrcount, outfile);
    free(data);
    return 1;
}
