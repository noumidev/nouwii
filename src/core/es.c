/*
 * nouwii is a Nintendo Wii emulator.
 * Copyright (C) 2025  noumidev
 */

#include "core/es.h"

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "core/hle.h"
#include "core/memory.h"

#define TITLE_ID (0x0000000100000002)

#define CHECK_ARGS(IN, OUT) \
assert(numIn == IN);        \
assert(numOut == OUT);      \

enum {
    IOCTLV_GET_DATA_DIR = 0x1D,
    IOCTLV_GET_TITLE_ID = 0x20,
};

static void GetIoctlvArgs(const int n, u32* addr, u32* size, const u32 vec) {
    *addr = memory_Read32(vec + sizeof(u64) * n);
    *size = memory_Read32(vec + sizeof(u64) * n + sizeof(u32));
}

static u32 GetDataDir(const u32 numIn, const u32 numOut, const u32 vec) {
    CHECK_ARGS(1, 1)

    u32 addrIn, sizeIn;
    u32 addrOut, sizeOut;

    GetIoctlvArgs(0, &addrIn, &sizeIn, vec);
    GetIoctlvArgs(1, &addrOut, &sizeOut, vec);

    assert(sizeIn == sizeof(u64));

    const u64 titleId = memory_Read64(addrIn);

    printf("ES GetDataDir (title ID: %016llX, addr: %08X, size: %u)\n", titleId, addrOut, sizeOut);

    assert(titleId == TITLE_ID);

    strncpy(memory_GetPointer(addrOut), "/title/00000001/00000002/data", sizeOut);

    return IOS_OK;
}

static u32 GetTitleId(const u32 numIn, const u32 numOut, const u32 vec) {
    CHECK_ARGS(0, 1)

    u32 addr, size;

    GetIoctlvArgs(0, &addr, &size, vec);

    assert(size == sizeof(u64));

    printf("ES GetTitleId (addr: %08X, size: %u)\n", addr, size);

    memory_Write64(addr, TITLE_ID);

    return IOS_OK;
}

void es_Initialize() {

}

void es_Reset() {

}

void es_Shutdown() {

}

u32 es_Ioctlv(const u32 ioctl, const u32 numIn, const u32 numOut, const u32 vec) {
    switch (ioctl) {
        case IOCTLV_GET_DATA_DIR:
            return GetDataDir(numIn, numOut, vec);
        case IOCTLV_GET_TITLE_ID:
            return GetTitleId(numIn, numOut, vec);
        default:
            printf("ES Unimplemented ioctlv %08X\n", ioctl);
            exit(1);
    }
}
