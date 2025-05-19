/*
 * nouwii is a Nintendo Wii emulator.
 * Copyright (C) 2025  noumidev
 */

#include "common/bit.h"

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
