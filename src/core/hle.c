/*
 * nouwii is a Nintendo Wii emulator.
 * Copyright (C) 2025  noumidev
 */

#include "core/hle.h"

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "core/memory.h"

#include "hw/ipc.h"

#define NUM_ARGS (5)

#define NUM_TASK_CYCLES (4096)

enum {
    COMMAND_OPEN = 1,
};

enum {
    TASK_NONE,
    TASK_ACKNOWLEDGE,
    TASK_COMMAND_COMPLETED,
};

typedef union Packet {
    u32 raw[8];

    struct {
        u32 cmd;
        u32 retval;
        u32 fd;
        u32 arg[NUM_ARGS];
    };
} __attribute__((packed)) Packet;

typedef struct Context {
    int currentTask;

    int taskTimer;
} Context;

static Context ctx;

void hle_Initialize() {

}

void hle_Reset() {
    memset(&ctx, 0, sizeof(ctx));
}

void hle_Shutdown() {

}

void hle_IpcExecute(const u32 ppcmsg) {
    static i32 fd = 0;

    Packet packet;

    for (u32 i = 0; i < sizeof(packet); i += sizeof(u32)) {
        packet.raw[i / sizeof(u32)] = memory_Read32(ppcmsg + i);
    }

    switch (packet.cmd) {
        case COMMAND_OPEN:
            {
                const char* name = (const char*)memory_GetPointer(packet.arg[0]);
                const u32 mode = packet.arg[1];

                printf("HLE IPC_Open (name: %s, mode: %u)\n", name, mode);

                packet.retval = 0;
                packet.fd = fd++;

                ctx.currentTask = TASK_ACKNOWLEDGE;
                break;
            }
        default:
            printf("HLE Unimplemented IPC command type %u\n", packet.cmd);

            exit(1);
    }

    ctx.taskTimer = NUM_TASK_CYCLES;
}

void hle_IpcRelaunch() {
    printf("HLE Relaunch IPC\n");
}

void hle_Tick(const i64 cycles) {
    if (ctx.taskTimer <= 0) {
        return;
    }

    ctx.taskTimer -= cycles;

    if (ctx.taskTimer <= 0) {
        switch (ctx.currentTask) {
            case TASK_ACKNOWLEDGE:
                ipc_CommandAcknowledged();

                ctx.taskTimer = NUM_TASK_CYCLES;
                ctx.currentTask = TASK_COMMAND_COMPLETED;
                break;
            case TASK_COMMAND_COMPLETED:
                ipc_CommandCompleted();

                ctx.currentTask = TASK_NONE;
                break;
            default:
                break;
        }
    }
}
