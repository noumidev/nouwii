/*
 * nouwii is a Nintendo Wii emulator.
 * Copyright (C) 2025  noumidev
 */

#pragma once

#include "common/types.h"

void hle_Initialize();
void hle_Reset();
void hle_Shutdown();

void hle_IpcExecute(const u32 ppcmsg);
void hle_IpcRelaunch();

void hle_Tick(const i64 cycles);
