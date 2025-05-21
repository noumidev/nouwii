/*
 * nouwii is a Nintendo Wii emulator.
 * Copyright (C) 2025  noumidev
 */

#pragma once

#include "common/types.h"

#define MAKEDECL_DI_READIO(size)         \
u##size di_ReadIo##size(const u32 addr); \

#define MAKEDECL_DI_WRITEIO(size)                          \
void di_WriteIo##size(const u32 addr, const u##size data); \

void di_Initialize();
void di_Reset();
void di_Shutdown();

MAKEDECL_DI_READIO(8)
MAKEDECL_DI_READIO(16)
MAKEDECL_DI_READIO(32)
MAKEDECL_DI_READIO(64)

MAKEDECL_DI_WRITEIO(8)
MAKEDECL_DI_WRITEIO(16)
MAKEDECL_DI_WRITEIO(32)
MAKEDECL_DI_WRITEIO(64)
