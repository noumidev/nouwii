/*
 * nouwii is a Nintendo Wii emulator.
 * Copyright (C) 2025  noumidev
 */

#pragma once

#include "common/types.h"

#define MAKEDECL_MI_READIO(size)         \
u##size mi_ReadIo##size(const u32 addr); \

#define MAKEDECL_MI_WRITEIO(size)                          \
void mi_WriteIo##size(const u32 addr, const u##size data); \

void mi_Initialize();
void mi_Reset();
void mi_Shutdown();

MAKEDECL_MI_READIO(8)
MAKEDECL_MI_READIO(16)
MAKEDECL_MI_READIO(32)
MAKEDECL_MI_READIO(64)

MAKEDECL_MI_WRITEIO(8)
MAKEDECL_MI_WRITEIO(16)
MAKEDECL_MI_WRITEIO(32)
MAKEDECL_MI_WRITEIO(64)
