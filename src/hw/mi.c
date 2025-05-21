/*
 * nouwii is a Nintendo Wii emulator.
 * Copyright (C) 2025  noumidev
 */

#include "hw/mi.h"

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define MAKEFUNC_MI_READIO(size)                                     \
u##size mi_ReadIo##size(const u32 addr) {                            \
    printf("MI Unimplemented read%d (address: %08X)\n", size, addr); \
    return 0;                                                        \
}                                                                    \

#define MAKEFUNC_MI_WRITEIO(size)                                                       \
void mi_WriteIo##size(const u32 addr, const u##size data) {                             \
    printf("MI Unimplemented write%d (address: %08X, data: %02X)\n", size, addr, data); \
}                                                                                       \

void mi_Initialize() {

}

void mi_Reset() {

}

void mi_Shutdown() {

}

MAKEFUNC_MI_READIO(8)
MAKEFUNC_MI_READIO(16)
MAKEFUNC_MI_READIO(32)
MAKEFUNC_MI_READIO(64)

MAKEFUNC_MI_WRITEIO(8)
MAKEFUNC_MI_WRITEIO(16)
MAKEFUNC_MI_WRITEIO(32)
MAKEFUNC_MI_WRITEIO(64)
