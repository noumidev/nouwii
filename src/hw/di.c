/*
 * nouwii is a Nintendo Wii emulator.
 * Copyright (C) 2025  noumidev
 */

#include "hw/di.h"

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define MAKEFUNC_DI_READIO(size)                                     \
u##size di_ReadIo##size(const u32 addr) {                            \
    printf("DI Unimplemented read%d (address: %08X)\n", size, addr); \
    exit(1);                                                         \
}                                                                    \

#define MAKEFUNC_DI_WRITEIO(size)                                                       \
void di_WriteIo##size(const u32 addr, const u##size data) {                             \
    printf("DI Unimplemented write%d (address: %08X, data: %02X)\n", size, addr, data); \
    exit(1);                                                                            \
}                                                                                       \

enum {
    DI_CFG = 0xD006024,
};

#define CONTROL (ctx.control)

typedef struct Context {
    
} Context;

static Context ctx;

void di_Initialize() {

}

void di_Reset() {
    memset(&ctx, 0, sizeof(ctx));
}

void di_Shutdown() {

}

MAKEFUNC_DI_READIO(8)
MAKEFUNC_DI_READIO(16)
MAKEFUNC_DI_READIO(64)

u32 di_ReadIo32(const u32 addr) {
    switch (addr) {
        case DI_CFG:
            printf("DI_CFG read32\n");

            return 0;
        default:
            printf("DI Unimplemented read32 (address: %08X)\n", addr);

            exit(1);
    }
}

MAKEFUNC_DI_WRITEIO(8)
MAKEFUNC_DI_WRITEIO(16)
MAKEFUNC_DI_WRITEIO(64)

void di_WriteIo32(const u32 addr, const u32 data) {
    switch (addr) {
        default:
            printf("DI Unimplemented write32 (address: %08X, data: %08X)\n", addr, data);

            exit(1); 
    }
}
