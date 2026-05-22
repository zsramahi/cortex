#include "core.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int vmcompile(const char* infile, const char* outfile) {
    FILE* tpl = fopen("cpu.exe", "rb");
    if (!tpl) {
        tpl = fopen("cpu", "rb");
        if (!tpl) {
            fprintf(stderr, "cannot open cpu executable as template\n");
            return 0;
        }
    }

    fseek(tpl, 0, SEEK_END);
    long tplsize = ftell(tpl);
    fseek(tpl, 0, SEEK_SET);

    uint8_t* tpldata = malloc(tplsize);
    fread(tpldata, 1, tplsize, tpl);
    fclose(tpl);

    FILE* bc = fopen(infile, "rb");
    if (!bc) { fprintf(stderr, "cannot open %s\n", infile); free(tpldata); return 0; }

    fseek(bc, 0, SEEK_END);
    long bcsize = ftell(bc);
    fseek(bc, 0, SEEK_SET);

    uint8_t* bcdata = malloc(bcsize);
    fread(bcdata, 1, bcsize, bc);
    fclose(bc);

    size_t outsize = tplsize + 8 + 4 + bcsize;
    uint8_t* outdata = malloc(outsize);
    memcpy(outdata, tpldata, tplsize);

    size_t pos = tplsize;
    outdata[pos++] = MARKER0;
    outdata[pos++] = MARKER1;
    outdata[pos++] = MARKER2;
    outdata[pos++] = MARKER3;
    outdata[pos++] = MARKER4;
    outdata[pos++] = MARKER5;
    outdata[pos++] = MARKER6;
    outdata[pos++] = MARKER7;

    outdata[pos++] = bcsize & 0xFF;
    outdata[pos++] = (bcsize >> 8) & 0xFF;
    outdata[pos++] = (bcsize >> 16) & 0xFF;
    outdata[pos++] = (bcsize >> 24) & 0xFF;

    memcpy(outdata + pos, bcdata, bcsize);

    FILE* out = fopen(outfile, "wb");
    if (!out) { fprintf(stderr, "cannot create %s\n", outfile); free(tpldata); free(bcdata); free(outdata); return 0; }
    fwrite(outdata, 1, outsize, out);
    fclose(out);

    printf("compiled %ld bytes bytecode into %zu byte executable: %s\n", bcsize, outsize, outfile);

    free(tpldata);
    free(bcdata);
    free(outdata);
    return 1;
}
