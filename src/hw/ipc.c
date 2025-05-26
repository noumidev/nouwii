/*
 * nouwii is a Nintendo Wii emulator.
 * Copyright (C) 2025  noumidev
 */

#include "hw/ipc.h"

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "core/hle.h"

#include "hw/hollywood.h"

#define MASK_PPCCTRL (0x00000039)

#define PPCCTRL (ctx.ppcctrl)
#define ARMMSG  (ctx.armmsg)
#define PPCMSG  (ctx.ppcmsg)

enum {
    FLAG_EXECUTE     = 1 << 0,
    FLAG_ACKNOWLEDGE = 1 << 1,
    FLAG_COMPLETED   = 1 << 2,
    FLAG_RELAUNCH    = 1 << 3,
};

typedef struct Context {
    u32 armmsg, ppcmsg;

    union {
        u32 raw;

        struct {
            u32  x1 :  1;
            u32  y2 :  1;
            u32  y1 :  1;
            u32  x2 :  1;
            u32 iy1 :  1;
            u32 iy2 :  1;
            u32     : 26;
        };
    } ppcctrl;
} Context;

static Context ctx;

static void CheckHwInterrupt() {
    if (((PPCCTRL.y1 != 0) && (PPCCTRL.iy1 != 0)) ||
        ((PPCCTRL.y2 != 0) && (PPCCTRL.iy2 != 0))) {
            hollywood_AssertIrq(HOLLYWOOD_IRQ_BROADWAY_IPC);
        }
}

void ipc_Initialize() {

}

void ipc_Reset() {
    memset(&ctx, 0, sizeof(ctx));
}

void ipc_Shutdown() {

}

void ipc_CommandAcknowledged() {
    printf("IPC Acknowledged command\n");

    PPCCTRL.y2 = 1;

    CheckHwInterrupt();
}

void ipc_CommandCompleted() {
    printf("IPC Completed command\n");
    
    PPCCTRL.y1 = 1;

    ARMMSG = PPCMSG;

    CheckHwInterrupt();
}

u32 ipc_ReadArmMessage() {
    return ARMMSG;
}

u32 ipc_ReadPpcControl() {
    return PPCCTRL.raw;
}

void ipc_WritePpcControl(const u32 data) {
    if (((PPCCTRL.raw & FLAG_EXECUTE) == 0) && ((data & FLAG_EXECUTE) != 0)) {
        hle_IpcExecute(PPCMSG);
    }

    if (((PPCCTRL.raw & FLAG_RELAUNCH) == 0) && ((data & FLAG_RELAUNCH) != 0)) {
        hle_IpcRelaunch();
    }

    PPCCTRL.raw &= ~MASK_PPCCTRL;
    PPCCTRL.raw |= data & MASK_PPCCTRL;

    if ((data & FLAG_ACKNOWLEDGE) != 0) {
        PPCCTRL.y2 = 0;
    }

    if ((data & FLAG_COMPLETED) != 0) {
        PPCCTRL.y1 = 0;
    }

    CheckHwInterrupt();
}

void ipc_WritePpcMessage(const u32 data) {
    PPCMSG = data;
}
