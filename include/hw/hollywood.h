/*
 * nouwii is a Nintendo Wii emulator.
 * Copyright (C) 2025  noumidev
 */

#pragma once

#include "common/types.h"

enum {
    HOLLYWOOD_IRQ_BROADWAY_IPC = 30,
};

#define MAKEDECL_HOLLYWOOD_READIO(size)         \
u##size hollywood_ReadIo##size(const u32 addr); \

#define MAKEDECL_HOLLYWOOD_WRITEIO(size)                          \
void hollywood_WriteIo##size(const u32 addr, const u##size data); \

void hollywood_Initialize();
void hollywood_Reset();
void hollywood_Shutdown();

void hollywood_AssertIrq(const u32 irqn);
void hollywood_ClearIrq(const u32 irqn);

MAKEDECL_HOLLYWOOD_READIO(8)
MAKEDECL_HOLLYWOOD_READIO(16)
MAKEDECL_HOLLYWOOD_READIO(32)
MAKEDECL_HOLLYWOOD_READIO(64)

MAKEDECL_HOLLYWOOD_WRITEIO(8)
MAKEDECL_HOLLYWOOD_WRITEIO(16)
MAKEDECL_HOLLYWOOD_WRITEIO(32)
MAKEDECL_HOLLYWOOD_WRITEIO(64)
