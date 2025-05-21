/*
 * nouwii is a Nintendo Wii emulator.
 * Copyright (C) 2025  noumidev
 */

#include "hw/pi.h"

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define CONSOLE_TYPE (2 << 28)

#define MAKEFUNC_PI_READIO(size)                                     \
u##size pi_ReadIo##size(const u32 addr) {                            \
    printf("PI Unimplemented read%d (address: %08X)\n", size, addr); \
    exit(1);                                                         \
}                                                                    \

enum {
    PI_CONSOLE_TYPE = 0xC00302C,
};

void pi_Initialize() {

}

void pi_Reset() {

}

void pi_Shutdown() {

}

MAKEFUNC_PI_READIO(8)
MAKEFUNC_PI_READIO(16)
MAKEFUNC_PI_READIO(64)

u32 pi_ReadIo32(const u32 addr) {
    switch (addr) {
        case PI_CONSOLE_TYPE:
            printf("PI_CONSOLE_TYPE read32\n");

            return CONSOLE_TYPE;
        default:
            printf("PI Unimplemented read32 (address: %08X)\n", addr);

            exit(1);
    }
}
