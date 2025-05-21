/*
 * nouwii is a Nintendo Wii emulator.
 * Copyright (C) 2025  noumidev
 */

#pragma once

#include "common/types.h"

#define MAKEDECL_DSP_READIO(size)         \
u##size dsp_ReadIo##size(const u32 addr); \

#define MAKEDECL_DSP_WRITEIO(size)                          \
void dsp_WriteIo##size(const u32 addr, const u##size data); \

void dsp_Initialize();
void dsp_Reset();
void dsp_Shutdown();

MAKEDECL_DSP_READIO(8)
MAKEDECL_DSP_READIO(16)
MAKEDECL_DSP_READIO(32)
MAKEDECL_DSP_READIO(64)

MAKEDECL_DSP_WRITEIO(8)
MAKEDECL_DSP_WRITEIO(16)
MAKEDECL_DSP_WRITEIO(32)
MAKEDECL_DSP_WRITEIO(64)
