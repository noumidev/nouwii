/*
 * nouwii is a Nintendo Wii emulator.
 * Copyright (C) 2025  noumidev
 */

#include "hw/ai.h"

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define MAKEFUNC_AI_READIO(size)                                     \
u##size ai_ReadIo##size(const u32 addr) {                            \
    printf("AI Unimplemented read%d (address: %08X)\n", size, addr); \
    exit(1);                                                         \
}                                                                    \

#define MAKEFUNC_AI_WRITEIO(size)                                                       \
void ai_WriteIo##size(const u32 addr, const u##size data) {                             \
    printf("AI Unimplemented write%d (address: %08X, data: %02X)\n", size, addr, data); \
    exit(1);                                                                            \
}                                                                                       \

enum {
    AI_CONTROL = 0xD006C00,
};

#define CONTROL (ctx.control)

typedef struct Context {
    union {
        u32 raw;

        struct {
            u32    pstat :  1; // Play status
            u32      afr :  1; // Auxiliary frequency
            u32 aiintmsk :  1; // AI interrupt mask
            u32    aiint :  1; // AI interrupt
            u32 aiintvld :  1; // AI interrupt via matching
            u32  screset :  1; // Sample counter reset
            u32     rate :  1; // Sample rate
            u32          : 25;
        };
    } control;
} Context;

static Context ctx;

void ai_Initialize() {

}

void ai_Reset() {
    memset(&ctx, 0, sizeof(ctx));
}

void ai_Shutdown() {

}

MAKEFUNC_AI_READIO(8)
MAKEFUNC_AI_READIO(16)
MAKEFUNC_AI_READIO(64)

u32 ai_ReadIo32(const u32 addr) {
    switch (addr) {
        case AI_CONTROL:
            printf("AI_CONTROL read32\n");

            return CONTROL.raw;
        default:
            printf("AI Unimplemented read32 (address: %08X)\n", addr);

            exit(1);
    }
}

MAKEFUNC_AI_WRITEIO(8)
MAKEFUNC_AI_WRITEIO(16)
MAKEFUNC_AI_WRITEIO(64)

void ai_WriteIo32(const u32 addr, const u32 data) {
    switch (addr) {
        case AI_CONTROL:
            printf("AI_CONTROL write32 (data: %08X)\n", data);

            CONTROL.raw = data;
            break;
        default:
            printf("AI Unimplemented write32 (address: %08X, data: %08X)\n", addr, data);

            exit(1); 
    }
}
