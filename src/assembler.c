#include "core.h"
#include "opcodes.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

typedef struct {
    const char* name;
    uint16_t code;
    int operands;
} opinfo;

static const opinfo optable[] = {
    {"nop", OP_NOP, 0}, {"hlt", OP_HLT, 0}, {"halt", OP_HLT, 0},
    {"mov", OP_MOV, 2}, {"add", OP_ADD, 2}, {"sub", OP_SUB, 2},
    {"mul", OP_MUL, 2}, {"div", OP_DIV, 2}, {"and", OP_AND, 2},
    {"or", OP_OR, 2}, {"xor", OP_XOR, 2}, {"not", OP_NOT, 1},
    {"shl", OP_SHL, 2}, {"shr", OP_SHR, 2}, {"cmp", OP_CMP, 2},
    {"jmp", OP_JMP, -1}, {"je", OP_JE, -1}, {"jne", OP_JNE, -1},
    {"jg", OP_JG, -1}, {"jl", OP_JL, -1}, {"jge", OP_JGE, -1},
    {"jle", OP_JLE, -1}, {"jz", OP_JZ, -1}, {"jnz", OP_JNZ, -1},
    {"jc", OP_JC, -1}, {"jnc", OP_JNC, -1},
    {"call", OP_CALL, -1}, {"ret", OP_RET, 0},
    {"push", OP_PUSH, 1}, {"pop", OP_POP, 1},
    {"load", OP_LOAD, 2}, {"store", OP_STORE, 2},
    {"in", OP_IN, 1}, {"out", OP_OUT, 1},
    {"movimm", OP_MOVIMM, -2}, {"mod", OP_MOD, 2},
    {"inc", OP_INC, 1}, {"dec", OP_DEC, 1},
    {"sar", OP_SAR, 2}, {"rol", OP_ROL, 2}, {"ror", OP_ROR, 2},
    {"test", OP_TEST, 2}, {"loadbyte", OP_LOADBYTE, 2},
    {"storebyte", OP_STOREBYTE, 2}, {"loadreg", OP_LOADREG, 2},
    {"storereg", OP_STOREREG, 2}, {"clr", OP_CLR, 1},
    {"swap", OP_SWAP, 2}, {"neg", OP_NEG, 1}, {"abs", OP_ABS, 1},
    {"bswap", OP_BSWAP, 1}, {"bts", OP_BTS, 2}, {"btr", OP_BTR, 2},
    {"btc", OP_BTC, 2}, {"bsf", OP_BSF, 1}, {"bsr", OP_BSR, 1},
    {"popcnt", OP_POPCNT, 1},
    {"addimm", OP_ADDIMM, -2}, {"subimm", OP_SUBIMM, -2},
    {"mulimm", OP_MULIMM, -2}, {"andimm", OP_ANDIMM, -2},
    {"orimm", OP_ORIMM, -2}, {"xorimm", OP_XORIMM, -2},
    {"cmpimm", OP_CMPIMM, -2}, {"pushimm", OP_PUSHIMM, -3},
    {"dup", OP_DUP, 0}, {"drop", OP_DROP, 0}, {"over", OP_OVER, 0},
    {"rot", OP_ROT, 0}, {"pick", OP_PICK, 1},
    {"loop", OP_LOOP, -1}, {"loope", OP_LOOPE, -1}, {"loopne", OP_LOOPNE, -1},
    {"strcpy", OP_STRCPY, 2}, {"strcmp", OP_STRCMP, 2}, {"strlen", OP_STRLEN, 1},
    {"memset", OP_MEMSET, 3}, {"memcpy", OP_MEMCPY, 3},
    {"min", OP_MIN, 2}, {"max", OP_MAX, 2},
    {"sqrt", OP_SQRT, 1}, {"pow", OP_POW, 2},
    {"sin", OP_SIN, 1}, {"cos", OP_COS, 1}, {"tan", OP_TAN, 1},
    {"rand", OP_RAND, 1},

    {"int", OP_INT, 1}, {"iret", OP_IRET, 0},
    {"cli", OP_CLI, 0}, {"sti", OP_STI, 0}, {"pause", OP_PAUSE, 0},
    {"nopmulti", OP_NOPMULTI, 1}, {"cacheflush", OP_CACHEFLUSH, 0},
    {"prefetch", OP_PREFETCH, 1}, {"fence", OP_FENCE, 0},
    {"getpc", OP_GETPC, 1}, {"getsp", OP_GETSP, 1}, {"setsp", OP_SETSP, 1},
    {"getflags", OP_GETFLAGS, 1}, {"setflags", OP_SETFLAGS, 1},
    {"clc", OP_CLC, 0}, {"stc", OP_STC, 0}, {"cmc", OP_CMC, 0},
    {"std", OP_STD, 0}, {"cld", OP_CLD, 0},
    {"sahf", OP_SAHF, 1}, {"lahf", OP_LAHF, 1},
    {"pushf", OP_PUSHF, 0}, {"popf", OP_POPF, 0},
    {"pusha", OP_PUSHA, 0}, {"popa", OP_POPA, 0},
    {"enter", OP_ENTER, -3}, {"leave", OP_LEAVE, 0},
    {"xchg", OP_XCHG, 2}, {"lea", OP_LEA, 2},
    {"cmpxchg", OP_CMPXCHG, 2}, {"xadd", OP_XADD, 2},
    {"lock", OP_LOCK, 0}, {"unlock", OP_UNLOCK, 0}, {"wait", OP_WAIT, 0},
    {"adc", OP_ADC, 2}, {"sbb", OP_SBB, 2},
    {"imul", OP_IMUL, 2}, {"idiv", OP_IDIV, 2},
    {"movsx", OP_MOVSX, 2}, {"movzx", OP_MOVZX, 2},
    {"setc", OP_SETC, 1}, {"setz", OP_SETZ, 1}, {"setn", OP_SETN, 1},
    {"seto", OP_SETO, 1}, {"sete", OP_SETE, 1}, {"setne", OP_SETNE, 1},
    {"setg", OP_SETG, 1}, {"setl", OP_SETL, 1},
    {"setge", OP_SETGE, 1}, {"setle", OP_SETLE, 1},
    {"cmove", OP_CMOVE, 2}, {"cmovne", OP_CMOVNE, 2},
    {"cmovg", OP_CMOVG, 2}, {"cmovl", OP_CMOVL, 2},
    {"cmovge", OP_CMOVGE, 2}, {"cmovle", OP_CMOVLE, 2},
    {"shld", OP_SHLD, 3}, {"shrd", OP_SHRD, 3},
    {"rcl", OP_RCL, 2}, {"rcr", OP_RCR, 2},
    {"asin", OP_ASIN, 1}, {"acos", OP_ACOS, 1}, {"atan", OP_ATAN, 1},
    {"log", OP_LOG, 1}, {"log10", OP_LOG10, 1}, {"exp", OP_EXP, 1},
    {"ceil", OP_CEIL, 1}, {"floor", OP_FLOOR, 1}, {"round", OP_ROUND, 1},
    {"trunc", OP_TRUNC, 1}, {"fabs", OP_FABS, 1},
    {"sinh", OP_SINH, 1}, {"cosh", OP_COSH, 1}, {"tanh", OP_TANH, 1},
    {"gcd", OP_GCD, 2}, {"lcm", OP_LCM, 2},
    {"factorial", OP_FACTORIAL, 1}, {"fibonacci", OP_FIBONACCI, 1},
    {"isprime", OP_ISPRIME, 1}, {"nextprime", OP_NEXTPRIME, 1},
    {"reversebits", OP_REVERSEBITS, 1}, {"grayencode", OP_GRAYENCODE, 1},
    {"graydecode", OP_GRAYDECODE, 1}, {"parity", OP_PARITY, 1},
    {"hammingweight", OP_HAMMINGWEIGHT, 1}, {"hammingdistance", OP_HAMMINGDISTANCE, 2},
    {"crc32", OP_CRC32, 2}, {"hash", OP_HASH, 2},
    {"atomicadd", OP_ATOMICADD, 2}, {"atomicsub", OP_ATOMICSUB, 2},
    {"atomicxchg", OP_ATOMICXCHG, 2}, {"atomicand", OP_ATOMICAND, 2},
    {"atomicor", OP_ATOMICOR, 2}, {"atomicxor", OP_ATOMICXOR, 2},
    {"atomicmax", OP_ATOMICMAX, 2}, {"atomicmin", OP_ATOMICMIN, 2},
    {"barrier", OP_BARRIER, 0}, {"membarrier", OP_MEMBARRIER, 0},
    {"loadfence", OP_LOADFENCE, 0}, {"storefence", OP_STOREFENCE, 0},
    {"fullfence", OP_FULLFENCE, 0}, {"spinlock", OP_SPINLOCK, 0},
    {"spinunlock", OP_SPINUNLOCK, 0}, {"yield", OP_YIELD, 0},
    {"sleep", OP_SLEEP, 1}, {"wakeup", OP_WAKEUP, 1},
    {"allocate", OP_ALLOCATE, 1}, {"deallocate", OP_DEALLOCATE, 2},
    {"memzero", OP_MEMZERO, 2}, {"memfill", OP_MEMFILL, 3},
    {"memcompare", OP_MEMCOMPARE, 3}, {"memmove", OP_MEMMOVE, 3},
    {"memswap", OP_MEMSWAP, 3},
    {"bitcount", OP_BITCOUNT, 1}, {"bitreverse", OP_BITREVERSE, 1},
    {"bitrotate", OP_BITROTATE, 2},
    {"saturate", OP_SATURATE, 2}, {"clamp", OP_CLAMP, 3}, {"lerp", OP_LERP, 3},
    {"dotproduct", OP_DOTPRODUCT, 3}, {"crossproduct", OP_CROSSPRODUCT, 3},
    {"magnitude", OP_MAGNITUDE, 2}, {"normalize", OP_NORMALIZE, 2},
    {"distance", OP_DISTANCE, 3},
    {"sort", OP_SORT, 2}, {"shuffle", OP_SHUFFLE, 2},
    {"reverse", OP_REVERSE, 2}, {"search", OP_SEARCH, 3},
    {"reduce", OP_REDUCE, 2}, {"scan", OP_SCAN, 2},
    {"broadcast", OP_BROADCAST, 3},
    {"rdrand", OP_RDRAND, 1}, {"rdseed", OP_RDSEED, 1},
    {"rdtscp", OP_RDTSCP, 1}, {"syscall", OP_SYSCALL, 1},
    {NULL, 0, 0}
};


#define MAXLABELS 4096
#define MAXFIXUPS 4096

typedef struct { char name[64]; uint32_t addr; } label;
typedef struct { char name[64]; uint32_t addr; } fixup;

static void emitbyte(uint8_t** buf, size_t* len, size_t* cap, uint8_t b) {
    if (*len >= *cap) { *cap *= 2; *buf = realloc(*buf, *cap); }
    (*buf)[(*len)++] = b;
}

static void emitword(uint8_t** buf, size_t* len, size_t* cap, uint32_t w) {
    emitbyte(buf, len, cap, w & 0xFF);
    emitbyte(buf, len, cap, (w >> 8) & 0xFF);
    emitbyte(buf, len, cap, (w >> 16) & 0xFF);
    emitbyte(buf, len, cap, (w >> 24) & 0xFF);
}

static void emitop(uint8_t** buf, size_t* len, size_t* cap, uint16_t op) {
    emitbyte(buf, len, cap, op & 0xFF);
    emitbyte(buf, len, cap, (op >> 8) & 0xFF);
}

static void strtolower(char* s) {
    for (; *s; s++) *s = tolower(*s);
}

static char* trimline(char* s) {
    while (*s && isspace(*s)) s++;
    char* end = s + strlen(s) - 1;
    while (end > s && isspace(*end)) { *end = 0; end--; }
    return s;
}

int vmassemble(const char* infile, const char* outfile) {
    FILE* f = fopen(infile, "r");
    if (!f) { fprintf(stderr, "cannot open %s\n", infile); return 0; }

    uint8_t* code = malloc(65536);
    size_t codelen = 0, codecap = 65536;

    label labels[MAXLABELS];
    int labelcount = 0;
    fixup fixups[MAXFIXUPS];
    int fixupcount = 0;

    char line[1024];
    int linenum = 0;

    while (fgets(line, sizeof(line), f)) {
        linenum++;
        char* s = trimline(line);
        if (*s == 0 || *s == ';') continue;

        char* comment = strchr(s, ';');
        if (comment) *comment = 0;
        s = trimline(s);
        if (*s == 0) continue;

        size_t slen = strlen(s);
        if (s[slen - 1] == ':') {
            s[slen - 1] = 0;
            strtolower(s);
            if (labelcount < MAXLABELS) {
                strncpy(labels[labelcount].name, s, 63);
                labels[labelcount].addr = (uint32_t)codelen;
                labelcount++;
            }
            continue;
        }

        char mnemonic[64] = {0};
        char args[256] = {0};
        char* space = strchr(s, ' ');
        if (space) {
            size_t mlen = space - s;
            if (mlen > 63) mlen = 63;
            strncpy(mnemonic, s, mlen);
            strcpy(args, space + 1);
        } else {
            strncpy(mnemonic, s, 63);
        }
        strtolower(mnemonic);

        const opinfo* op = NULL;
        for (int i = 0; optable[i].name; i++) {
            if (strcmp(optable[i].name, mnemonic) == 0) {
                op = &optable[i]; break;
            }
        }

        if (!op) {
            fprintf(stderr, "line %d: unknown opcode '%s'\n", linenum, mnemonic);
            continue;
        }

        emitop(&code, &codelen, &codecap, op->code);

        if (op->operands == 0) continue;

        char* tok = strtok(args, ",");
        int argidx = 0;

        while (tok) {
            while (*tok && isspace(*tok)) tok++;
            char* tend = tok + strlen(tok) - 1;
            while (tend > tok && isspace(*tend)) { *tend = 0; tend--; }

            if (op->operands == -1 || (op->operands == -2 && argidx == 1) ||
                op->operands == -3) {
                if (argidx == 0 && op->operands == -2) {
                    emitbyte(&code, &codelen, &codecap, (uint8_t)atoi(tok));
                } else {
                    if (isdigit(*tok) || *tok == '-') {
                        uint32_t val = (uint32_t)strtoul(tok, NULL, 0);
                        emitword(&code, &codelen, &codecap, val);
                    } else {
                        strtolower(tok);
                        if (fixupcount < MAXFIXUPS) {
                            strncpy(fixups[fixupcount].name, tok, 63);
                            fixups[fixupcount].addr = (uint32_t)codelen;
                            fixupcount++;
                        }
                        emitword(&code, &codelen, &codecap, 0);
                    }
                }
            } else {
                emitbyte(&code, &codelen, &codecap, (uint8_t)atoi(tok));
            }
            argidx++;
            tok = strtok(NULL, ",");
        }
    }
    fclose(f);

    for (int i = 0; i < fixupcount; i++) {
        int found = 0;
        for (int j = 0; j < labelcount; j++) {
            if (strcmp(fixups[i].name, labels[j].name) == 0) {
                uint32_t addr = labels[j].addr;
                uint32_t pos = fixups[i].addr;
                code[pos] = addr & 0xFF;
                code[pos+1] = (addr >> 8) & 0xFF;
                code[pos+2] = (addr >> 16) & 0xFF;
                code[pos+3] = (addr >> 24) & 0xFF;
                found = 1; break;
            }
        }
        if (!found)
            fprintf(stderr, "unresolved label: %s\n", fixups[i].name);
    }

    FILE* out = fopen(outfile, "wb");
    if (!out) { fprintf(stderr, "cannot create %s\n", outfile); free(code); return 0; }
    fwrite(code, 1, codelen, out);
    fclose(out);

    printf("assembled %d lines, %zu bytes to %s\n", linenum, codelen, outfile);
    free(code);
    return 1;
}
