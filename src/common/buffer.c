/*
 * nouwii is a Nintendo Wii emulator.
 * Copyright (C) 2025  noumidev
 */

#include "common/buffer.h"

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "common/bswap.h"

#define MAKEFUNC_GET(size)                                              \
u##size GET##size(const u8* buf, const u64 sizeBuf, const u64 offset) { \
    assert((offset + sizeof(u##size)) < sizeBuf);                       \
    u##size data;                                                       \
    memcpy(&data, &buf[offset], sizeof(u##size));                       \
    return common_Bswap##size(data);                                    \
}                                                                       \

MAKEFUNC_GET(8)
MAKEFUNC_GET(16)
MAKEFUNC_GET(32)

int common_IsAligned(const u64 addr, const u64 align) {
    return ((addr & (align - 1)) == 0);
}

u64 common_Align(const u64 addr, const u64 align) {
    if (common_IsAligned(addr, align)) {
        return addr;
    }

    return (addr & ~(align - 1)) + align;
}
