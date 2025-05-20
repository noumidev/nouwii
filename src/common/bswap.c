/*
 * nouwii is a Nintendo Wii emulator.
 * Copyright (C) 2025  noumidev
 */

#include "common/bswap.h"

u8 common_Bswap8(const u8 data) {
    return data;
}

u16 common_Bswap16(const u16 data) {
    return (data >> 8) | (data << 8);
}

u32 common_Bswap32(const u32 data) {
    return (data >> 24) | ((data >> 8) & 0xFF00) | ((data & 0xFF00) << 8) | (data << 24);
}

u64 common_Bswap64(const u64 data) {
    u64 n = data;

    n = ((n & 0x00000000FFFFFFFFULL) << 32) | ((n & 0xFFFFFFFF00000000ULL) >> 32);
    n = ((n & 0x0000FFFF0000FFFFULL) << 16) | ((n & 0xFFFF0000FFFF0000ULL) >> 16);
    n = ((n & 0x00FF00FF00FF00FFULL) <<  8) | ((n & 0xFF00FF00FF00FF00ULL) >>  8);

    return n;
}
