/*
 * nouwii is a Nintendo Wii emulator.
 * Copyright (C) 2025  noumidev
 */

#include "hw/broadway.h"

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "core/memory.h"

#define BROADWAY_DEBUG

#define NUM_GPRS (32)

#define INITIAL_PC (0x3400)

enum {
    PRIMARY_ADDI = 14,
    PRIMARY_ADDIS = 15,
};

#define MAKEFUNC_BROADWAY_READ(size)        \
static u##size Read##size(const u32 addr) { \
    return memory_Read##size(addr);         \
}                                           \

#define MAKEFUNC_BROADWAY_WRITE(size)                         \
static void Write##size(const u32 addr, const u##size data) { \
    memory_Write##size(addr, data);                           \
}                                                             \

#define TO_IBM_POS(n) (31 - n)

static u32 GetBits(const u32 n, const u32 start, const u32 end) {
    const u32 mask = (0xFFFFFFFF << TO_IBM_POS(end)) & (0xFFFFFFFF >> start);

    return (n & mask) >> TO_IBM_POS(end);
}

// Instruction fields
#define OPCD (GetBits(instr,  0,  5))
#define   RA (GetBits(instr, 11, 15))
#define   RB (GetBits(instr, 16, 20))
#define   RD (GetBits(instr,  6, 10))
#define UIMM (GetBits(instr, 16, 31))

typedef struct Context {
    i64 cyclesToRun;

    u32 ia, cia;

    u32 r[NUM_GPRS];
} Context;

static Context ctx;

MAKEFUNC_BROADWAY_READ(8)
MAKEFUNC_BROADWAY_READ(16)
MAKEFUNC_BROADWAY_READ(32)

static u32 FetchInstr() {
    ctx.cia = ctx.ia;

    const u32 instr = Read32(ctx.ia);

    ctx.ia += sizeof(instr);

    return instr;
}

MAKEFUNC_BROADWAY_WRITE(8)
MAKEFUNC_BROADWAY_WRITE(16)
MAKEFUNC_BROADWAY_WRITE(32)

static void ADDI(const u32 instr) {
    u32 n = (i16)UIMM;

    if (RA != 0) {
        n += ctx.r[RA];
    }

    ctx.r[RD] = n;

#ifdef BROADWAY_DEBUG
    printf("PPC [%08X] addi r%u, r%u, %X; r%u: %08X\n", ctx.cia, RD, RA, UIMM, RA, ctx.r[RA]);
#endif
}

static void ADDIS(const u32 instr) {
    u32 n = UIMM << 16;

    if (RA != 0) {
        n += ctx.r[RA];
    }

    ctx.r[RD] = n;

#ifdef BROADWAY_DEBUG
    printf("PPC [%08X] addis r%u, r%u, %X; r%u: %08X\n", ctx.cia, RD, RA, UIMM, RA, ctx.r[RA]);
#endif
}

static void ExecInstr(const u32 instr) {
    switch (OPCD) {
        case PRIMARY_ADDI:
            ADDI(instr);
            break;
        case PRIMARY_ADDIS:
            ADDIS(instr);
            break;
        default:
            printf("Unimplemented Broadway primary opcode %u (IA: %08X, instruction: %08X)\n", OPCD, ctx.cia, instr);

            exit(1);
    }
}

void broadway_Initialize() {

}

void broadway_Reset() {
    memset(&ctx, 0, sizeof(ctx));

    // HLE boot stub
    ctx.ia = INITIAL_PC;
}

void broadway_Shutdown() {

}

void broadway_Run() {
    for (; ctx.cyclesToRun > 0; ctx.cyclesToRun--) {
        const u32 instr = FetchInstr();

        ExecInstr(instr);
    }
}

i64* broadway_GetCyclesToRun() {
    return &ctx.cyclesToRun;
}
