/*
 * nouwii is a Nintendo Wii emulator.
 * Copyright (C) 2025  noumidev
 */

#pragma once

#include "common/types.h"

#define MAKEDECL_GET(size) u##size GET##size(const u8* buf, const u64 sizeBuf, const u64 offset);

MAKEDECL_GET(8)
MAKEDECL_GET(16)
MAKEDECL_GET(32)

int common_IsAligned(const u64 addr, const u64 align);
u64 common_Align(const u64 addr, const u64 align);
