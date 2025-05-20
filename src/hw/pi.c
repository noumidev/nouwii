/*
 * nouwii is a Nintendo Wii emulator.
 * Copyright (C) 2025  noumidev
 */

#include "hw/pi.h"

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define MAKEFUNC_PI_READIO(size)                                     \
u##size pi_ReadIo##size(const u32 addr) {                            \
    printf("PI Unimplemented read%d (address: %08X)\n", size, addr); \
    exit(1);                                                         \
}                                                                    \

void pi_Initialize() {

}

void pi_Reset() {

}

void pi_Shutdown() {

}

MAKEFUNC_PI_READIO(8)
MAKEFUNC_PI_READIO(16)
MAKEFUNC_PI_READIO(32)
MAKEFUNC_PI_READIO(64)
