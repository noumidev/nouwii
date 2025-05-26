/*
 * nouwii is a Nintendo Wii emulator.
 * Copyright (C) 2025  noumidev
 */

#pragma once

#include "common/types.h"

void es_Initialize();
void es_Reset();
void es_Shutdown();

// vDevice functions
u32 es_Ioctlv(const u32 ioctl, const u32 numIn, const u32 numOut, const u32 vec);
