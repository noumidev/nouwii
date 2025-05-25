/*
 * nouwii is a Nintendo Wii emulator.
 * Copyright (C) 2025  noumidev
 */

#pragma once

#include "common/types.h"

enum {
    PI_IRQ_HOLLYWOOD = 14,
};

#define MAKEDECL_PI_READIO(size)         \
u##size pi_ReadIo##size(const u32 addr); \

#define MAKEDECL_PI_WRITEIO(size)                          \
void pi_WriteIo##size(const u32 addr, const u##size data); \

void pi_Initialize();
void pi_Reset();
void pi_Shutdown();

void pi_AssertIrq(const u32 irqn);
void pi_ClearIrq(const u32 irqn);

int pi_IsIrqAsserted();

MAKEDECL_PI_READIO(8)
MAKEDECL_PI_READIO(16)
MAKEDECL_PI_READIO(32)
MAKEDECL_PI_READIO(64)

MAKEDECL_PI_WRITEIO(8)
MAKEDECL_PI_WRITEIO(16)
MAKEDECL_PI_WRITEIO(32)
MAKEDECL_PI_WRITEIO(64)
