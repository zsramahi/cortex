#include "core.h"
#include "opcodes.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static void usage(const char* prog) {
    printf("cpu vm - 256 registers, 512 opcodes, 1mb memory\n");
    printf("usage:\n");
    printf("  %s run <file.vm>              - execute bytecode\n", prog);
    printf("  %s asm <file.asm> <out.vm>    - assemble to bytecode\n", prog);
    printf("  %s disasm <file.vm> <out.asm> - disassemble bytecode\n", prog);
    printf("  %s compile <file.vm> <out>    - compile to self-extracting\n", prog);
    printf("  %s decompile <file> <out.asm> - extract and disassemble\n", prog);
    printf("  %s test                       - run built-in tests\n", prog);
}

static void runtests(void) {
    printf("running built-in tests...\n");
    vmstate vm;
    vminit(&vm);

    size_t pc = 0;

    vm.memory[pc++] = OP_MOVIMM & 0xFF; vm.memory[pc++] = (OP_MOVIMM >> 8) & 0xFF;
    vm.memory[pc++] = 0;
    vm.memory[pc++] = 10; vm.memory[pc++] = 0; vm.memory[pc++] = 0; vm.memory[pc++] = 0;

    vm.memory[pc++] = OP_MOVIMM & 0xFF; vm.memory[pc++] = (OP_MOVIMM >> 8) & 0xFF;
    vm.memory[pc++] = 1;
    vm.memory[pc++] = 20; vm.memory[pc++] = 0; vm.memory[pc++] = 0; vm.memory[pc++] = 0;

    vm.memory[pc++] = OP_ADD & 0xFF; vm.memory[pc++] = (OP_ADD >> 8) & 0xFF;
    vm.memory[pc++] = 0; vm.memory[pc++] = 1;

    vm.memory[pc++] = OP_OUT & 0xFF; vm.memory[pc++] = (OP_OUT >> 8) & 0xFF;
    vm.memory[pc++] = 0;

    vm.memory[pc++] = OP_HLT & 0xFF; vm.memory[pc++] = (OP_HLT >> 8) & 0xFF;

    vmpush(&vm, SENTINEL);
    vm.running = 1;

    while (vm.running && vm.instrcount < 1000)
        vmexecute(&vm);

    printf("test 1 (10+20): r0 = %u %s\n", vm.registers[0],
           vm.registers[0] == 30 ? "PASS" : "FAIL");


    vmreset(&vm);
    pc = 0;

    vm.memory[pc++] = OP_MOVIMM & 0xFF; vm.memory[pc++] = (OP_MOVIMM >> 8) & 0xFF;
    vm.memory[pc++] = 0;
    vm.memory[pc++] = 5; vm.memory[pc++] = 0; vm.memory[pc++] = 0; vm.memory[pc++] = 0;

    vm.memory[pc++] = OP_FACTORIAL & 0xFF; vm.memory[pc++] = (OP_FACTORIAL >> 8) & 0xFF;
    vm.memory[pc++] = 0;

    vm.memory[pc++] = OP_OUT & 0xFF; vm.memory[pc++] = (OP_OUT >> 8) & 0xFF;
    vm.memory[pc++] = 0;

    vm.memory[pc++] = OP_HLT & 0xFF; vm.memory[pc++] = (OP_HLT >> 8) & 0xFF;

    vmpush(&vm, SENTINEL);
    vm.running = 1;

    while (vm.running && vm.instrcount < 1000)
        vmexecute(&vm);

    printf("test 2 (5!): r0 = %u %s\n", vm.registers[0],
           vm.registers[0] == 120 ? "PASS" : "FAIL");

    vmreset(&vm);
    pc = 0;

    vm.memory[pc++] = OP_MOVIMM & 0xFF; vm.memory[pc++] = (OP_MOVIMM >> 8) & 0xFF;
    vm.memory[pc++] = 0;
    vm.memory[pc++] = 10; vm.memory[pc++] = 0; vm.memory[pc++] = 0; vm.memory[pc++] = 0;

    vm.memory[pc++] = OP_FIBONACCI & 0xFF; vm.memory[pc++] = (OP_FIBONACCI >> 8) & 0xFF;
    vm.memory[pc++] = 0;

    vm.memory[pc++] = OP_OUT & 0xFF; vm.memory[pc++] = (OP_OUT >> 8) & 0xFF;
    vm.memory[pc++] = 0;

    vm.memory[pc++] = OP_HLT & 0xFF; vm.memory[pc++] = (OP_HLT >> 8) & 0xFF;

    vmpush(&vm, SENTINEL);
    vm.running = 1;

    while (vm.running && vm.instrcount < 1000)
        vmexecute(&vm);

    printf("test 3 (fib 10): r0 = %u %s\n", vm.registers[0],
           vm.registers[0] == 55 ? "PASS" : "FAIL");

    vmreset(&vm);
    pc = 0;

    vm.memory[pc++] = OP_MOVIMM & 0xFF; vm.memory[pc++] = (OP_MOVIMM >> 8) & 0xFF;
    vm.memory[pc++] = 0;
    vm.memory[pc++] = 100; vm.memory[pc++] = 0; vm.memory[pc++] = 0; vm.memory[pc++] = 0;

    vm.memory[pc++] = OP_MOVIMM & 0xFF; vm.memory[pc++] = (OP_MOVIMM >> 8) & 0xFF;
    vm.memory[pc++] = 1;
    vm.memory[pc++] = 3; vm.memory[pc++] = 0; vm.memory[pc++] = 0; vm.memory[pc++] = 0;

    vm.memory[pc++] = OP_MOD & 0xFF; vm.memory[pc++] = (OP_MOD >> 8) & 0xFF;
    vm.memory[pc++] = 0; vm.memory[pc++] = 1;

    vm.memory[pc++] = OP_OUT & 0xFF; vm.memory[pc++] = (OP_OUT >> 8) & 0xFF;
    vm.memory[pc++] = 0;

    vm.memory[pc++] = OP_HLT & 0xFF; vm.memory[pc++] = (OP_HLT >> 8) & 0xFF;

    vmpush(&vm, SENTINEL);
    vm.running = 1;

    while (vm.running && vm.instrcount < 1000)
        vmexecute(&vm);

    printf("test 4 (100 mod 3): r0 = %u %s\n", vm.registers[0],
           vm.registers[0] == 1 ? "PASS" : "FAIL");

    vmreset(&vm);
    pc = 0;

    vm.memory[pc++] = OP_MOVIMM & 0xFF; vm.memory[pc++] = (OP_MOVIMM >> 8) & 0xFF;
    vm.memory[pc++] = 0;
    vm.memory[pc++] = 7; vm.memory[pc++] = 0; vm.memory[pc++] = 0; vm.memory[pc++] = 0;

    vm.memory[pc++] = OP_ISPRIME & 0xFF; vm.memory[pc++] = (OP_ISPRIME >> 8) & 0xFF;
    vm.memory[pc++] = 0;

    vm.memory[pc++] = OP_OUT & 0xFF; vm.memory[pc++] = (OP_OUT >> 8) & 0xFF;
    vm.memory[pc++] = 0;

    vm.memory[pc++] = OP_HLT & 0xFF; vm.memory[pc++] = (OP_HLT >> 8) & 0xFF;

    vmpush(&vm, SENTINEL);
    vm.running = 1;

    while (vm.running && vm.instrcount < 1000)
        vmexecute(&vm);

    printf("test 5 (isprime 7): r0 = %u %s\n", vm.registers[0],
           vm.registers[0] == 1 ? "PASS" : "FAIL");

    vmfree(&vm);
    printf("all tests complete\n");
}


static int runembedded(const char* self) {
    FILE* f = fopen(self, "rb");
    if (!f) return 0;

    fseek(f, 0, SEEK_END);
    long size = ftell(f);
    fseek(f, 0, SEEK_SET);

    uint8_t* data = malloc(size);
    fread(data, 1, size, f);
    fclose(f);

    const uint8_t marker[] = {MARKER0, MARKER1, MARKER2, MARKER3, MARKER4, MARKER5, MARKER6, MARKER7};

    for (long i = size - 12; i >= 0; i--) {
        if (memcmp(data + i, marker, 8) == 0) {
            uint32_t bcsize = data[i+8] | (data[i+9] << 8) | (data[i+10] << 16) | (data[i+11] << 24);
            if (i + 12 + (long)bcsize <= size) {
                printf("running embedded bytecode (%u bytes)...\n", bcsize);

                vmstate vm;
                vminit(&vm);
                memcpy(vm.memory, data + i + 12, bcsize);
                vmpush(&vm, SENTINEL);
                vm.running = 1;

                while (vm.running && vm.instrcount < 10000000)
                    vmexecute(&vm);

                printf("execution complete: %u instructions\n", vm.instrcount);
                int ret = (int)vm.registers[0];
                vmfree(&vm);
                free(data);
                return ret;
            }
        }
    }

    free(data);
    return -1;
}

int main(int argc, char* argv[]) {
    if (argc < 2) {
        int ret = runembedded(argv[0]);
        if (ret >= 0) return ret;
        usage(argv[0]);
        return 1;
    }

    if (strcmp(argv[1], "test") == 0) {
        runtests();
        return 0;
    }

    if (strcmp(argv[1], "run") == 0) {
        if (argc < 3) { fprintf(stderr, "error: missing input file\n"); return 1; }

        FILE* f = fopen(argv[2], "rb");
        if (!f) { fprintf(stderr, "cannot open %s\n", argv[2]); return 1; }

        vmstate vm;
        vminit(&vm);
        fread(vm.memory, 1, MEMSIZE, f);
        fclose(f);

        vmpush(&vm, SENTINEL);
        vm.running = 1;

        while (vm.running && vm.instrcount < 10000000)
            vmexecute(&vm);

        printf("execution complete: %u instructions\n", vm.instrcount);
        int ret = (int)vm.registers[0];
        vmfree(&vm);
        return ret;
    }

    if (strcmp(argv[1], "asm") == 0) {
        if (argc < 4) { fprintf(stderr, "error: missing input or output\n"); return 1; }
        return vmassemble(argv[2], argv[3]) ? 0 : 1;
    }

    if (strcmp(argv[1], "disasm") == 0) {
        if (argc < 4) { fprintf(stderr, "error: missing input or output\n"); return 1; }
        return vmdisassemble(argv[2], argv[3]) ? 0 : 1;
    }

    if (strcmp(argv[1], "compile") == 0) {
        if (argc < 4) { fprintf(stderr, "error: missing input or output\n"); return 1; }

        const char* input = argv[2];
        size_t ilen = strlen(input);
        int isasm = (ilen > 4 && strcmp(input + ilen - 4, ".asm") == 0);

        if (isasm) {
            printf("assembling %s...\n", input);
            if (!vmassemble(input, "__tmp_compile.vm")) return 1;
            int ret = vmcompile("__tmp_compile.vm", argv[3]) ? 0 : 1;
            remove("__tmp_compile.vm");
            return ret;
        }

        return vmcompile(input, argv[3]) ? 0 : 1;
    }

    if (strcmp(argv[1], "decompile") == 0) {
        if (argc < 4) { fprintf(stderr, "error: missing input or output\n"); return 1; }
        return vmdecompile(argv[2], argv[3]) ? 0 : 1;
    }

    fprintf(stderr, "unknown command: %s\n", argv[1]);
    usage(argv[0]);
    return 1;
}
