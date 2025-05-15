/*
 * nouwii is a Nintendo Wii emulator.
 * Copyright (C) 2025  noumidev
 */

#pragma once

#include "common/types.h"

#define MAKEDECL_READ(size) u##size memory_Read##size(const u32 addr);
#define MAKEDECL_WRITE(size) void memory_Write##size(const u32 addr, const u##size data);

void memory_Initialize(const char* pathMem1, const char* pathMem2);
void memory_Reset();
void memory_Shutdown();

MAKEDECL_READ(8)
MAKEDECL_READ(16)
MAKEDECL_READ(32)

MAKEDECL_WRITE(8)
MAKEDECL_WRITE(16)
MAKEDECL_WRITE(32)

void memory_Map(u8* mem, const u32 addr, const u32 size, const int read, const int write);

void* memory_GetPointer(const u32 addr);
