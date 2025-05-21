/*
 * nouwii is a Nintendo Wii emulator.
 * Copyright (C) 2025  noumidev
 */

#include "hw/vi.h"

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define MAKEFUNC_VI_READIO(size)                                     \
u##size vi_ReadIo##size(const u32 addr) {                            \
    printf("VI Unimplemented read%d (address: %08X)\n", size, addr); \
    return 0;                                                        \
}                                                                    \

#define MAKEFUNC_VI_WRITEIO(size)                                                       \
void vi_WriteIo##size(const u32 addr, const u##size data) {                             \
    printf("VI Unimplemented write%d (address: %08X, data: %02X)\n", size, addr, data); \
}                                                                                       \

void vi_Initialize() {

}

void vi_Reset() {

}

void vi_Shutdown() {

}

MAKEFUNC_VI_READIO(8)
MAKEFUNC_VI_READIO(16)
MAKEFUNC_VI_READIO(32)
MAKEFUNC_VI_READIO(64)

MAKEFUNC_VI_WRITEIO(8)
MAKEFUNC_VI_WRITEIO(16)
MAKEFUNC_VI_WRITEIO(32)
MAKEFUNC_VI_WRITEIO(64)
