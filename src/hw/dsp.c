/*
 * nouwii is a Nintendo Wii emulator.
 * Copyright (C) 2025  noumidev
 */

#include "hw/dsp.h"

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define NUM_MAILBOXES (2)

#define MASK_CONTROL (0x0957)

#define MAKEFUNC_DSP_READIO(size)                                     \
u##size dsp_ReadIo##size(const u32 addr) {                            \
    printf("DSP Unimplemented read%d (address: %08X)\n", size, addr); \
    exit(1);                                                          \
}                                                                     \

#define MAKEFUNC_DSP_WRITEIO(size)                                                       \
void dsp_WriteIo##size(const u32 addr, const u##size data) {                             \
    printf("DSP Unimplemented write%d (address: %08X, data: %02X)\n", size, addr, data); \
    exit(1);                                                                             \
}                                                                                        \

#define MAILBOX_IN  (ctx.mailbox[MBOX_IN])
#define MAILBOX_OUT (ctx.mailbox[MBOX_OUT])
#define CONTROL     (ctx.control)
#define ARSIZE      (ctx.arsize)
#define MMADDR      (ctx.mmaddr)
#define ARADDR      (ctx.araddr)
#define DMASIZE     (ctx.dmasize)

enum {
    LO = 0,
    HI = 1,
};

enum {
    DSP_MAILBOX_IN  = 0xC005000,
    DSP_MAILBOX_OUT = 0xC005004,
    DSP_CONTROL     = 0xC00500A,
    DSP_ARSIZE      = 0xC005012,
    DSP_MMADDR      = 0xC005020,
    DSP_ARADDR      = 0xC005024,
    DSP_DMASIZE     = 0xC005028,
};

enum {
    MBOX_IN  = 0,
    MBOX_OUT = 1,
};

enum {
    CONTROL_AIDINT = 1 << 3,
    CONTROL_ARINT  = 1 << 5,
    CONTROL_DSPINT = 1 << 7,
};

typedef struct Context {
    union {
        u16 raw[2];

        struct {
            u32 data : 31;
            u32  set :  1;
        };
    } mailbox[NUM_MAILBOXES];

    union {
        u16 raw;

        struct {
            u16       res : 1; // Reset
            u16     piint : 1; // PI interrupt
            u16      halt : 1; // Halt DSP?
            u16    aidint : 1; // AI interrupt status
            u16 aidintmsk : 1; // AI interrupt mask
            u16     arint : 1; // ARAM interrupt status
            u16  arintmsk : 1; // ARAM interrupt mask
            u16    dspint : 1; // DSP interrupt status
            u16 dspintmsk : 1; // DSP interrupt mask
            u16 ardmastat : 1; // ARAM DMA status
            u16   dmastat : 1; // DSP DMA status
            u16  bootmode : 1; // DSP boot mode
            u16           : 4;
        };
    } control;

    union {
        u16 raw;

        struct {
            u16  cas :  3;
            u16 size :  3;
            u16      : 10;
        };
    } arsize;

    union {
        u16 raw[2];

        u32 addr;
    } mmaddr, araddr;

    union {
        u16 raw[2];
        u32 raw32;

        struct {
            u32 size : 31;
            u32  dir :  1;
        };
    } dmasize;
} Context;

static Context ctx;

void dsp_Initialize() {

}

void dsp_Reset() {
    memset(&ctx, 0, sizeof(ctx));
}

void dsp_Shutdown() {

}

MAKEFUNC_DSP_READIO(8)
MAKEFUNC_DSP_READIO(32)
MAKEFUNC_DSP_READIO(64)

u16 dsp_ReadIo16(const u32 addr) {
    switch (addr) {
        case DSP_MAILBOX_OUT:
            printf("DSP_MAILBOX_OUT_H read16\n");

            return MAILBOX_OUT.raw[HI];
        case DSP_MAILBOX_OUT + sizeof(u16):
            printf("DSP_MAILBOX_OUT_L read16\n");

            return MAILBOX_OUT.raw[LO];
        case DSP_CONTROL:
            printf("DSP_CONTROL read16\n");

            return CONTROL.raw;
        default:
            printf("DSP Unimplemented read16 (address: %08X)\n", addr);

            exit(1);
    }
}

MAKEFUNC_DSP_WRITEIO(8)
MAKEFUNC_DSP_WRITEIO(64)

void dsp_WriteIo16(const u32 addr, const u16 data) {
    switch (addr) {
        case DSP_MAILBOX_IN:
            printf("DSP_MAILBOX_IN_H write16 (data: %04X)\n", data);

            MAILBOX_IN.raw[HI] = data;
            break;
        case DSP_MAILBOX_IN + sizeof(u16):
            printf("DSP_MAILBOX_IN_L write16 (data: %04X)\n", data);

            MAILBOX_IN.raw[LO] = data;
            break;
        case DSP_CONTROL:
            printf("DSP_CONTROL write16 (data: %04X)\n", data);

            CONTROL.raw &= ~MASK_CONTROL;
            CONTROL.raw |= data & MASK_CONTROL;

            if (CONTROL.res != 0) {
                printf("DSP reset\n");

                CONTROL.res = 0;
            }

            if ((data & CONTROL_AIDINT) != 0) {
                CONTROL.aidint = 0;
            }

            if ((data & CONTROL_ARINT) != 0) {
                CONTROL.arint = 0;
            }

            if ((data & CONTROL_DSPINT) != 0) {
                CONTROL.dspint = 0;
            }

            break;
        case DSP_ARSIZE:
            printf("DSP_ARSIZE write16 (data: %04X)\n", data);

            ARSIZE.raw = data;
            break;
        default:
            printf("DSP Unimplemented write16 (address: %08X, data: %04X)\n", addr, data);

            exit(1); 
    }
}

void dsp_WriteIo32(const u32 addr, const u32 data) {
    switch (addr) {
        case DSP_MMADDR:
            printf("DSP_MMADDR write32 (data: %08X)\n", data);

            MMADDR.addr = data;
            break;
        case DSP_ARADDR:
            printf("DSP_ARADDR write32 (data: %08X)\n", data);

            ARADDR.addr = data;
            break;
        case DSP_DMASIZE:
            printf("DSP_DMASIZE write32 (data: %08X)\n", data);

            DMASIZE.raw32 = data;

            // HACK
            CONTROL.arint = 1;
            MAILBOX_OUT.set = 1;
            break;
        default:
            printf("DSP Unimplemented write32 (address: %08X, data: %08X)\n", addr, data);

            exit(1); 
    }
}
