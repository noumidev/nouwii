/*
 * nouwii is a Nintendo Wii emulator.
 * Copyright (C) 2025  noumidev
 */

#pragma once

#include "common/types.h"

#define MAKEDECL_SI_READIO(size)         \
u##size si_ReadIo##size(const u32 addr); \

#define MAKEDECL_SI_WRITEIO(size)                          \
void si_WriteIo##size(const u32 addr, const u##size data); \

void si_Initialize();
void si_Reset();
void si_Shutdown();

MAKEDECL_SI_READIO(8)
MAKEDECL_SI_READIO(16)
MAKEDECL_SI_READIO(32)
MAKEDECL_SI_READIO(64)

MAKEDECL_SI_WRITEIO(8)
MAKEDECL_SI_WRITEIO(16)
MAKEDECL_SI_WRITEIO(32)
MAKEDECL_SI_WRITEIO(64)
