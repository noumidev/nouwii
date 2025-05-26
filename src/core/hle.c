/*
 * nouwii is a Nintendo Wii emulator.
 * Copyright (C) 2025  noumidev
 */

#include "core/hle.h"

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "common/types.h"

#include "core/dev_di.h"
#include "core/es.h"
#include "core/fs.h"
#include "core/memory.h"

#include "hw/ipc.h"

#define NUM_ARGS (5)

#define MAX_FILES     (128)
#define MAX_FILE_NAME (129)

#define NUM_TASK_CYCLES (64)

enum {
    COMMAND_OPEN     = 1,
    COMMAND_CLOSE    = 2,
    COMMAND_READ     = 3,
    COMMAND_WRITE    = 4,
    COMMAND_SEEK     = 5,
    COMMAND_IOCTL    = 6,
    COMMAND_IOCTLV   = 7,
    COMMAND_RESPONSE = 8,
};

enum {
    TASK_NONE,
    TASK_ACKNOWLEDGE,
    TASK_COMMAND_COMPLETED,
};

static u32 DummyIoctl(u32, u32, u32, u32, u32) {
    printf("HLE Dummy ioctl\n");

    return IOS_OK;
}

static u32 DummyIoctlv(u32, u32, u32, u32) {
    printf("HLE Dummy ioctlv\n");

    return IOS_OK;
}

typedef struct File {
    int opened;

    char name[MAX_FILE_NAME];

    FILE* data;

    u32 (*ioctl)(u32, u32, u32, u32, u32);
    u32 (*ioctlv)(u32, u32, u32, u32);
} File;

typedef union Packet {
    u32 raw[8];

    struct {
        u32 cmd;
        u32 retval;
        u32 fd;
        u32 arg[NUM_ARGS];
    };
} __attribute__((packed)) Packet;

#define IOCTL   (packet.arg[0])
#define ADDR0   (packet.arg[1])
#define NUM_IN  (packet.arg[1])
#define SIZE0   (packet.arg[2])
#define NUM_OUT (packet.arg[2])
#define ADDR1   (packet.arg[3])
#define VEC     (packet.arg[3])
#define SIZE1   (packet.arg[4])

typedef struct Context {
    int currentTask;

    int taskTimer;
} Context;

static Context ctx;

static File files[MAX_FILES];

static i32 OpenFile(const char* path, const u32 mode) {
    (void)mode;

    static i32 fd = 0;

    assert(fd < MAX_FILES);

    File* file = &files[fd];

    assert(!file->opened);

    if (strcmp(path, "/dev/di") == 0) {
        file->ioctl = dev_di_Ioctl;
    } else if (strcmp(path, "/dev/es") == 0) {
        file->ioctlv = es_Ioctlv;
    } else if (strcmp(path, "/dev/fs") == 0) {
       file->ioctl = fs_Ioctl;
    } else if (strncmp(path, "/dev/net", strlen("/dev/net")) == 0) {
        // TODO
    } else if (strncmp(path, "/dev/stm", strlen("/dev/stm")) == 0) {
        // TODO
    } else {
        // Try and open as file
        char fpath[MAX_FILE_NAME];

        strncat(fpath, "filesystem", MAX_FILE_NAME);
        strncat(fpath, path, MAX_FILE_NAME);

        file->data = fopen(fpath, "r+b");

        if (file->data == NULL) {
            printf("HLE Failed to open file %s\n", path);
            exit(1);
        }
    }

    file->opened = NOUWII_TRUE;

    strncpy(file->name, path, MAX_FILE_NAME);

    return fd++;
}

static u32 CloseFile(const i32 fd) {
    assert(fd < MAX_FILES);

    File* file = &files[fd];

    assert(file->opened);

    printf("HLE IPC_Close (fd: %d, name: %s)\n", fd, file->name);

    file->opened = NOUWII_FALSE;

    return IOS_OK;
}

static u32 ReadFile(const i32 fd, const u32 addr, const u32 size) {
    assert(fd < MAX_FILES);

    File* file = &files[fd];

    assert(file->opened);
    assert(file->data != NULL);

    printf("HLE IPC_Read (fd: %d, name: %s, addr: %08X, size: %u)\n", fd, file->name, addr, size);

    assert(fread(memory_GetPointer(addr), sizeof(u8), size, file->data) == size);

    return size;
}

static u32 WriteFile(const i32 fd, const u32 addr, const u32 size) {
    assert(fd < MAX_FILES);

    File* file = &files[fd];

    assert(file->opened);
    assert(file->data != NULL);

    printf("HLE IPC_Write (fd: %d, name: %s, addr: %08X, size: %u)\n", fd, file->name, addr, size);

    assert(fwrite(memory_GetPointer(addr), sizeof(u8), size, file->data) == size);

    return size;
}

static u32 SeekFile(const i32 fd, const u32 offset, const u32 origin) {
    assert(fd < MAX_FILES);

    File* file = &files[fd];

    assert(file->opened);
    assert(file->data != NULL);

    printf("HLE IPC_Seek (fd: %d, name: %s, offset: %u, origin: %u)\n", fd, file->name, offset, origin);
    
    assert(origin == 0);

    fseek(file->data, offset, SEEK_SET);

    return IOS_OK;
}

void hle_Initialize() {

}

void hle_Reset() {
    memset(&ctx, 0, sizeof(ctx));
    memset(files, 0, sizeof(files));

    for (int i = 0; i < MAX_FILES; i++) {
        File* file = &files[i];

        file->ioctl = DummyIoctl;
        file->ioctlv = DummyIoctlv;
    }
}

void hle_Shutdown() {

}

void hle_IpcExecute(const u32 ppcmsg) {
    assert(ctx.currentTask == TASK_NONE);

    Packet packet;

    for (u32 i = 0; i < sizeof(packet); i += sizeof(u32)) {
        packet.raw[i / sizeof(u32)] = memory_Read32(ppcmsg + i);

        printf("%08X ", packet.raw[i / sizeof(u32)]);
    }

    printf("\n");

    switch (packet.cmd) {
        case COMMAND_OPEN:
            {
                const char* name = (const char*)memory_GetPointer(packet.arg[0]);
                const u32 mode = packet.arg[1];

                printf("HLE IPC_Open (name: %s, mode: %u)\n", name, mode);

                packet.retval = OpenFile(name, mode);
                break;
            }
        case COMMAND_CLOSE:
            packet.retval = CloseFile(packet.fd);
            break;
        case COMMAND_READ:
            packet.retval = ReadFile(packet.fd, packet.arg[0], packet.arg[1]);
            break;
        case COMMAND_WRITE:
            packet.retval = WriteFile(packet.fd, packet.arg[0], packet.arg[1]);
            break;
        case COMMAND_SEEK:
            packet.retval = SeekFile(packet.fd, packet.arg[0], packet.arg[1]);
            break;
        case COMMAND_IOCTL:
            printf("HLE IPC_Ioctl (fd: %u, ioctl: %08X)\n", packet.fd, packet.arg[0]);

            packet.retval = files[packet.fd].ioctl(IOCTL, ADDR0, SIZE0, ADDR1, SIZE1);
            break;
        case COMMAND_IOCTLV:
            printf("HLE IPC_Ioctlv (fd: %u, ioctl: %08X, #in: %u, #out: %u)\n", packet.fd, IOCTL, NUM_IN, NUM_OUT);

            packet.retval = files[packet.fd].ioctlv(IOCTL, NUM_IN, NUM_OUT, VEC);
            break;
        default:
            printf("HLE Unimplemented IPC command type %u\n", packet.cmd);

            exit(1);
    }

    packet.fd = packet.cmd;
    packet.cmd = COMMAND_RESPONSE;

    for (u32 i = 0; i < sizeof(packet); i += sizeof(u32)) {
        printf("%08X ", packet.raw[i / sizeof(u32)]);

        memory_Write32(ppcmsg + i, packet.raw[i / sizeof(u32)]);
    }

    printf("\n");

    ctx.taskTimer = NUM_TASK_CYCLES;
    ctx.currentTask = TASK_ACKNOWLEDGE;
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
