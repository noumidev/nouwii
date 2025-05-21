/*
 * nouwii is a Nintendo Wii emulator.
 * Copyright (C) 2025  noumidev
 */

#pragma once

#include "common/types.h"

#define MAKEDECL_AI_READIO(size)         \
u##size ai_ReadIo##size(const u32 addr); \

#define MAKEDECL_AI_WRITEIO(size)                          \
void ai_WriteIo##size(const u32 addr, const u##size data); \

void ai_Initialize();
void ai_Reset();
void ai_Shutdown();

MAKEDECL_AI_READIO(8)
MAKEDECL_AI_READIO(16)
MAKEDECL_AI_READIO(32)
MAKEDECL_AI_READIO(64)

MAKEDECL_AI_WRITEIO(8)
MAKEDECL_AI_WRITEIO(16)
MAKEDECL_AI_WRITEIO(32)
MAKEDECL_AI_WRITEIO(64)
