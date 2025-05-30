/*
 * nouwii is a Nintendo Wii emulator.
 * Copyright (C) 2025  noumidev
 */

#include "core/fs.h"

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "core/hle.h"
#include "core/memory.h"

enum {
    IOCTL_SET_ATTR = 5,
    IOCTL_GET_ATTR = 6,
};

static u32 SetAttr(const u32 addr0, const u32 size0, const u32 addr1, const u32 size1) {
    (void)addr1;
    (void)size1;

    assert(size0 == 0x4C);

    const char* name = memory_GetPointer(addr0) + 6;

    printf("FS SetAttr (name: %s)\n", name);

    return IOS_OK;
}

static u32 GetAttr(const u32 addr0, const u32 size0, const u32 addr1, const u32 size1) {
    assert(size0 == 0x40);
    assert(size1 == 0x4C);

    const char* name = memory_GetPointer(addr0);

    printf("FS GetAttr (name: %s, addr: %08X, size: %u)\n", name, addr1, size1);

    memset(memory_GetPointer(addr1), 0, size1);
    strncpy(memory_GetPointer(addr1) + 6, name, 0x40);

    return IOS_OK;
}

void fs_Initialize() {

}

void fs_Reset() {

}

void fs_Shutdown() {

}

u32 fs_Ioctl(const u32 ioctl, const u32 addr0, const u32 size0, const u32 addr1, const u32 size1) {
    switch (ioctl) {
        case IOCTL_SET_ATTR:
            return SetAttr(addr0, size0, addr1, size1);
        case IOCTL_GET_ATTR:
            return GetAttr(addr0, size0, addr1, size1);
        default:
            printf("FS Unimplemented ioctlv %08X\n", ioctl);
            exit(1);
    }
}
