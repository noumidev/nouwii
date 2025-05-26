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
    IOCTL_GET_ATTR = 6,
};

static u32 GetAttr(const u32 addr0, const u32 size0, const u32 addr1, const u32 size1) {
    (void)addr1;
    (void)size1;

    assert(size0 == 64);

    const char* name = memory_GetPointer(addr0);

    printf("FS GetAttr (name: %s)\n", name);

    exit(1);
}

void fs_Initialize() {

}

void fs_Reset() {

}

void fs_Shutdown() {

}

u32 fs_Ioctl(const u32 ioctl, const u32 addr0, const u32 size0, const u32 addr1, const u32 size1) {
    switch (ioctl) {
        case IOCTL_GET_ATTR:
            return GetAttr(addr0, size0, addr1, size1);
        default:
            printf("FS Unimplemented ioctlv %08X\n", ioctl);
            exit(1);
    }
}
