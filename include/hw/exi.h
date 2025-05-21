/*
 * nouwii is a Nintendo Wii emulator.
 * Copyright (C) 2025  noumidev
 */

#pragma once

#include "common/types.h"

#define MAKEDECL_EXI_READIO(size)         \
u##size exi_ReadIo##size(const u32 addr); \

#define MAKEDECL_EXI_WRITEIO(size)                          \
void exi_WriteIo##size(const u32 addr, const u##size data); \

void exi_Initialize();
void exi_Reset();
void exi_Shutdown();

MAKEDECL_EXI_READIO(8)
MAKEDECL_EXI_READIO(16)
MAKEDECL_EXI_READIO(32)
MAKEDECL_EXI_READIO(64)

MAKEDECL_EXI_WRITEIO(8)
MAKEDECL_EXI_WRITEIO(16)
MAKEDECL_EXI_WRITEIO(32)
MAKEDECL_EXI_WRITEIO(64)
