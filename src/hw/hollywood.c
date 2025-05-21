/*
 * nouwii is a Nintendo Wii emulator.
 * Copyright (C) 2025  noumidev
 */

#include "hw/hollywood.h"

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define MAKEFUNC_HOLLYWOOD_READIO(size)                                     \
u##size hollywood_ReadIo##size(const u32 addr) {                            \
    printf("Hollywood Unimplemented read%d (address: %08X)\n", size, addr); \
    exit(1);                                                                \
}                                                                           \

#define MAKEFUNC_HOLLYWOOD_WRITEIO(size)                                                       \
void hollywood_WriteIo##size(const u32 addr, const u##size data) {                             \
    printf("Hollywood Unimplemented write%d (address: %08X, data: %02X)\n", size, addr, data); \
    exit(1);                                                                                   \
}                                                                                              \

enum {
    HW_PPCIRQMASK = 0xD000034,
};

typedef struct Context {
    u32 ppcirqmask;
} Context;

static Context ctx;

void hollywood_Initialize() {

}

void hollywood_Reset() {
    memset(&ctx, 0, sizeof(ctx));
}

void hollywood_Shutdown() {

}

MAKEFUNC_HOLLYWOOD_READIO(8)
MAKEFUNC_HOLLYWOOD_READIO(16)
MAKEFUNC_HOLLYWOOD_READIO(64)

u32 hollywood_ReadIo32(const u32 addr) {
    switch (addr) {
        default:
            printf("Hollywood Unimplemented read32 (address: %08X)\n", addr);

            exit(1);
    }
}

MAKEFUNC_HOLLYWOOD_WRITEIO(8)
MAKEFUNC_HOLLYWOOD_WRITEIO(16)
MAKEFUNC_HOLLYWOOD_WRITEIO(64)

void hollywood_WriteIo32(const u32 addr, const u32 data) {
    switch (addr) {
        case HW_PPCIRQMASK:
            printf("HW_PPCIRQMASK write32 (data: %08X)\n", data);

            ctx.ppcirqmask = data;
            break;
        default:
            printf("Hollywood Unimplemented write32 (address: %08X, data: %08X)\n", addr, data);

            exit(1); 
    }
}
