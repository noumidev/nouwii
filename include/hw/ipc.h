/*
 * nouwii is a Nintendo Wii emulator.
 * Copyright (C) 2025  noumidev
 */

#pragma once

#include "common/types.h"

void ipc_Initialize();
void ipc_Reset();
void ipc_Shutdown();

void ipc_CommandAcknowledged();
void ipc_CommandCompleted();

u32 ipc_ReadArmMessage();
u32 ipc_ReadPpcControl();

void ipc_WritePpcControl(const u32 data);
void ipc_WritePpcMessage(const u32 data);
