/*
 * nouwii is a Nintendo Wii emulator.
 * Copyright (C) 2025  noumidev
 */

#include "common/bit.h"

#include <string.h>

#define MAKEFUNC_FROMF(size)                     \
u##size common_FromF##size(const f##size data) { \
    u##size n;                                   \
    memcpy(&n, &data, sizeof(n));                \
    return n;                                    \
}                                                \

#define MAKEFUNC_TOF(size)                     \
f##size common_ToF##size(const u##size data) { \
    f##size f;                                 \
    memcpy(&f, &data, sizeof(f));              \
    return f;                                  \
}                                              \

u32 common_Clz(const u32 data) {
    for (u32 i = 0; i < 32; i++) {
        if ((data & (1 << (31 - i))) != 0) {
            return i;
        }
    }

    return 32;
}

u32 common_Rotl(const u32 data, const int amt) {
    if (amt == 0) {
        return data;
    }

    return (data << (amt & 31)) | (data >> (32 - (amt & 31)));
}

MAKEFUNC_FROMF(32)
MAKEFUNC_FROMF(64)

MAKEFUNC_TOF(32)
MAKEFUNC_TOF(64)
