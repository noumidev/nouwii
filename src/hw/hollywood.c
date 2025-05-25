/*
 * nouwii is a Nintendo Wii emulator.
 * Copyright (C) 2025  noumidev
 */

#include "hw/hollywood.h"

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "hw/ipc.h"
#include "hw/pi.h"

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
    HW_IPCPPCMSG  = 0xD000000,
    HW_IPCPPCCTRL = 0xD000004,
    HW_PPCIRQFLAG = 0xD000030,
    HW_PPCIRQMASK = 0xD000034,
};

#define PPCIRQFLAG (ctx.ppcirqflag)
#define PPCIRQMASK (ctx.ppcirqmask)

typedef struct Context {
    u32 ppcirqflag;
    u32 ppcirqmask;
} Context;

static Context ctx;

static void CheckPiInterrupt() {
    if ((PPCIRQFLAG & PPCIRQMASK) != 0) {
        pi_AssertIrq(PI_IRQ_HOLLYWOOD);
    } else {
        pi_ClearIrq(PI_IRQ_HOLLYWOOD);
    }
}

void hollywood_Initialize() {

}

void hollywood_Reset() {
    memset(&ctx, 0, sizeof(ctx));
}

void hollywood_Shutdown() {

}

void hollywood_AssertIrq(const u32 irqn) {
    if ((PPCIRQFLAG & (1 << irqn)) == 0) {
        printf("Hollywood Interrupt %u asserted\n", irqn);
    }

    PPCIRQFLAG |= 1 << irqn;

    CheckPiInterrupt();
}

void hollywood_ClearIrq(const u32 irqn) {
    if ((PPCIRQFLAG & (1 << irqn)) != 0) {
        printf("Hollywood Interrupt %u cleared\n", irqn);
    }

    PPCIRQFLAG &= ~(1 << irqn);

    CheckPiInterrupt();
}

MAKEFUNC_HOLLYWOOD_READIO(8)
MAKEFUNC_HOLLYWOOD_READIO(16)
MAKEFUNC_HOLLYWOOD_READIO(64)

u32 hollywood_ReadIo32(const u32 addr) {
    switch (addr) {
        case HW_IPCPPCCTRL:
            printf("HW_IPCPPCMSG read32\n");

            return ipc_ReadPpcControl();
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
        case HW_IPCPPCMSG:
            printf("HW_IPCPPCMSG write32 (data: %08X)\n", data);

            ipc_WritePpcMessage(data);
            break;
        case HW_IPCPPCCTRL:
            printf("HW_IPCPPCCTRL write32 (data: %08X)\n", data);

            ipc_WritePpcControl(data);
            break;
        case HW_PPCIRQMASK:
            printf("HW_PPCIRQMASK write32 (data: %08X)\n", data);

            PPCIRQMASK = data;
            break;
        default:
            printf("Hollywood Unimplemented write32 (address: %08X, data: %08X)\n", addr, data);

            exit(1); 
    }
}
