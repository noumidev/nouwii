/*
 * nouwii is a Nintendo Wii emulator.
 * Copyright (C) 2025  noumidev
 */

#include "core/dev_di.h"

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "core/hle.h"
#include "core/memory.h"

enum {
    IOCTL_DVD_LOW_GET_COVER_REGISTER = 0x7A,
};

static u32 DvdLowGetCoverRegister(const u32 addr0, const u32 size0, const u32 addr1, const u32 size1) {
    (void)addr0;
    (void)size0;

    assert(size1 >= sizeof(u32));

    printf("DI DvdLowGetCoverRegister (addr: %08X, size: %u)\n", addr1, size1);

    memset(memory_GetPointer(addr1), 0, sizeof(u32));

    return IOS_OK;
}

void dev_di_Initialize() {

}

void dev_di_Reset() {

}

void dev_di_Shutdown() {

}

u32 dev_di_Ioctl(const u32 ioctl, const u32 addr0, const u32 size0, const u32 addr1, const u32 size1) {
    switch (ioctl) {
        case IOCTL_DVD_LOW_GET_COVER_REGISTER:
            return DvdLowGetCoverRegister(addr0, size0, addr1, size1);
        default:
            printf("DI Unimplemented ioctlv %08X\n", ioctl);
            exit(1);
    }
}
