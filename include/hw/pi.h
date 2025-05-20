/*
 * nouwii is a Nintendo Wii emulator.
 * Copyright (C) 2025  noumidev
 */

#pragma once

#include "common/types.h"

#define MAKEDECL_PI_READIO(size)         \
u##size pi_ReadIo##size(const u32 addr); \

void pi_Initialize();
void pi_Reset();
void pi_Shutdown();

MAKEDECL_PI_READIO(8)
MAKEDECL_PI_READIO(16)
MAKEDECL_PI_READIO(32)
MAKEDECL_PI_READIO(64)
