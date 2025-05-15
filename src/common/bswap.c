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
