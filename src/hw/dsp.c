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

#define CONTROL (ctx.control)

enum {
    DSP_CONTROL = 0xC00500A,
};

typedef struct Context {
    union {
        u16 raw;

        struct {
            u16       res : 1; // Reset
            u16     piint : 1; // PI interrupt
            u16      halt : 1; // Halt DSP?
            u16    aidint : 1; // AI interrupt status
            u16 aidintmsk : 1; // AI interrupt mask
            u16     arint : 1; // ARAM interrupt status
            u16  arintmsk : 1; // ARAM interrupt mask
            u16    dspint : 1; // DSP interrupt status
            u16 dspintmsk : 1; // DSP interrupt mask
            u16 ardmastat : 1; // ARAM DMA status
            u16   dmastat : 1; // DSP DMA status
            u16  bootmode : 1; // DSP boot mode
            u16           : 4;
        };
    } control;
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
        case DSP_CONTROL:
            printf("DSP_CONTROL read16\n");

            return CONTROL.raw;
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
        case DSP_CONTROL:
            printf("DSP_CONTROL write16 (data: %04X)\n", data);

            CONTROL.raw = data;

            if (CONTROL.res != 0) {
                printf("DSP reset\n");
            }
            break;
        default:
            printf("DSP Unimplemented write16 (address: %08X, data: %04X)\n", addr, data);

            exit(1); 
    }
}
