/*
 * nouwii is a Nintendo Wii emulator.
 * Copyright (C) 2025  noumidev
 */

#pragma once

#include "common/types.h"

u32 common_Clz(const u32 data);

u32 common_Rotl(const u32 data, const int amt);

f32 common_ToF32(const u32 data);
f64 common_ToF64(const u64 data);
