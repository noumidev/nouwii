/*
 * nouwii is a Nintendo Wii emulator.
 * Copyright (C) 2025  noumidev
 */

#include "hw/si.h"

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define MAKEFUNC_SI_READIO(size)                                     \
u##size si_ReadIo##size(const u32 addr) {                            \
    printf("SI Unimplemented read%d (address: %08X)\n", size, addr); \
    return 0;                                                        \
}                                                                    \

#define MAKEFUNC_SI_WRITEIO(size)                                                       \
void si_WriteIo##size(const u32 addr, const u##size data) {                             \
    printf("SI Unimplemented write%d (address: %08X, data: %02X)\n", size, addr, data); \
}                                                                                       \

void si_Initialize() {

}

void si_Reset() {

}

void si_Shutdown() {

}

MAKEFUNC_SI_READIO(8)
MAKEFUNC_SI_READIO(16)
MAKEFUNC_SI_READIO(32)
MAKEFUNC_SI_READIO(64)

MAKEFUNC_SI_WRITEIO(8)
MAKEFUNC_SI_WRITEIO(16)
MAKEFUNC_SI_WRITEIO(32)
MAKEFUNC_SI_WRITEIO(64)
