/*
 * nouwii is a Nintendo Wii emulator.
 * Copyright (C) 2025  noumidev
 */

#pragma once

#include "common/types.h"

#define MAKEDECL_VI_READIO(size)         \
u##size vi_ReadIo##size(const u32 addr); \

#define MAKEDECL_VI_WRITEIO(size)                          \
void vi_WriteIo##size(const u32 addr, const u##size data); \

void vi_Initialize();
void vi_Reset();
void vi_Shutdown();

MAKEDECL_VI_READIO(8)
MAKEDECL_VI_READIO(16)
MAKEDECL_VI_READIO(32)
MAKEDECL_VI_READIO(64)

MAKEDECL_VI_WRITEIO(8)
MAKEDECL_VI_WRITEIO(16)
MAKEDECL_VI_WRITEIO(32)
MAKEDECL_VI_WRITEIO(64)
