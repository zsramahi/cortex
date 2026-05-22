#include "core.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int vmdecompile(const char* infile, const char* outfile) {
    FILE* f = fopen(infile, "rb");
    if (!f) { fprintf(stderr, "cannot open %s\n", infile); return 0; }

    fseek(f, 0, SEEK_END);
    long size = ftell(f);
    fseek(f, 0, SEEK_SET);

    uint8_t* data = malloc(size);
    fread(data, 1, size, f);
    fclose(f);

    const uint8_t marker[] = {MARKER0, MARKER1, MARKER2, MARKER3, MARKER4, MARKER5, MARKER6, MARKER7};

    int found = 0;
    long markerpos = -1;

    for (long i = size - 8; i >= 0; i--) {
        if (memcmp(data + i, marker, 8) == 0) {
            markerpos = i;
            found = 1;
            break;
        }
    }

    if (!found) {
        fprintf(stderr, "no vm marker found in %s\n", infile);
        free(data);
        return 0;
    }

    long pos = markerpos + 8;
    if (pos + 4 > size) { fprintf(stderr, "truncated marker\n"); free(data); return 0; }

    uint32_t bcsize = data[pos] | (data[pos+1] << 8) | (data[pos+2] << 16) | (data[pos+3] << 24);
    pos += 4;

    if (pos + bcsize > (uint32_t)size) { fprintf(stderr, "bytecode extends past end\n"); free(data); return 0; }

    printf("found vm marker at offset 0x%lX, bytecode size: %u bytes\n", markerpos, bcsize);

    FILE* tmp = fopen("__tmp_bc.vm", "wb");
    if (!tmp) { free(data); return 0; }
    fwrite(data + pos, 1, bcsize, tmp);
    fclose(tmp);

    free(data);

    int result = vmdisassemble("__tmp_bc.vm", outfile);
    remove("__tmp_bc.vm");
    return result;
}
