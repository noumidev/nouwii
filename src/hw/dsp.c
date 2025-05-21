/*
 * nouwii is a Nintendo Wii emulator.
 * Copyright (C) 2025  noumidev
 */

#include "hw/dsp.h"

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define MAKEFUNC_DSP_READIO(size)                                     \
u##size dsp_ReadIo##size(const u32 addr) {                            \
    printf("DSP Unimplemented read%d (address: %08X)\n", size, addr); \
    exit(1);                                                          \
}                                                                     \

#define MAKEFUNC_DSP_WRITEIO(size)                                                       \
void dsp_WriteIo##size(const u32 addr, const u##size data) {                             \
    printf("DSP Unimplemented write%d (address: %08X, data: %02X)\n", size, addr, data); \
    exit(1);                                                                             \
}                                                                                        \

enum {
    A,
};

typedef struct Context {
} Context;

static Context ctx;

void dsp_Initialize() {

}

void dsp_Reset() {
    memset(&ctx, 0, sizeof(ctx));
}

void dsp_Shutdown() {

}

MAKEFUNC_DSP_READIO(8)
MAKEFUNC_DSP_READIO(32)
MAKEFUNC_DSP_READIO(64)

u16 dsp_ReadIo16(const u32 addr) {
    switch (addr) {
        default:
            printf("DSP Unimplemented read16 (address: %08X)\n", addr);

            exit(1);
    }
}

MAKEFUNC_DSP_WRITEIO(8)
MAKEFUNC_DSP_WRITEIO(32)
MAKEFUNC_DSP_WRITEIO(64)

void dsp_WriteIo16(const u32 addr, const u16 data) {
    switch (addr) {
        default:
            printf("DSP Unimplemented write16 (address: %08X, data: %04X)\n", addr, data);

            exit(1); 
    }
}
