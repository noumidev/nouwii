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

#define MAKEFUNC_PI_WRITEIO(size)                                                       \
void pi_WriteIo##size(const u32 addr, const u##size data) {                             \
    printf("PI Unimplemented write%d (address: %08X, data: %02X)\n", size, addr, data); \
    exit(1);                                                                            \
}                                                                                       \

enum {
    PI_INTMASK      = 0xC003004,
    PI_CONSOLE_TYPE = 0xC00302C,
};

typedef struct Context {
    u32 intmask;
} Context;

static Context ctx;

void pi_Initialize() {

}

void pi_Reset() {
    memset(&ctx, 0, sizeof(ctx));
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

MAKEFUNC_PI_WRITEIO(8)
MAKEFUNC_PI_WRITEIO(16)
MAKEFUNC_PI_WRITEIO(64)

void pi_WriteIo32(const u32 addr, const u32 data) {
    switch (addr) {
        case PI_INTMASK:
            printf("PI_INTMASK write32 (data: %08X)\n", data);

            ctx.intmask = data;
            break;
        default:
            printf("PI Unimplemented write32 (address: %08X, data: %08X)\n", addr, data);

            exit(1); 
    }
}
