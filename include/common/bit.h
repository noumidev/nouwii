/*
 * nouwii is a Nintendo Wii emulator.
 * Copyright (C) 2025  noumidev
 */

#pragma once

#include "common/types.h"

#define MAKEDECL_FROMF(size)                    \
u##size common_FromF##size(const f##size data); \

#define MAKEDECL_TOF(size)                    \
f##size common_ToF##size(const u##size data); \

u32 common_Clz(const u32 data);

u32 common_Rotl(const u32 data, const int amt);

MAKEDECL_FROMF(32)
MAKEDECL_FROMF(64)

MAKEDECL_TOF(32)
MAKEDECL_TOF(64)
