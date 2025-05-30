/*
 * nouwii is a Nintendo Wii emulator.
 * Copyright (C) 2025  noumidev
 */

#include "nouwii.h"

#include <stdio.h>

#include "common/config.h"

#include "core/dev_di.h"
#include "core/es.h"
#include "core/fs.h"
#include "core/hle.h"
#include "core/loader.h"
#include "core/memory.h"
#include "core/scheduler.h"

#include "hw/ai.h"
#include "hw/broadway.h"
#include "hw/di.h"
#include "hw/dsp.h"
#include "hw/exi.h"
#include "hw/hollywood.h"
#include "hw/ipc.h"
#include "hw/mi.h"
#include "hw/pi.h"
#include "hw/si.h"
#include "hw/vi.h"

#define NUM_ARGS (1 + 1)

static void InitializeGlobals() {
    // Values taken from a MEM1 dump after IOS boot
    memory_Write32(0x0028, 0x01800000); // Memory size
    memory_Write32(0x0034, 0x81800000); // Arena end
    memory_Write32(0x00F4, 0x01800000); // bi2.bin?
    memory_Write32(0x3100, 0x01800000); // Physical MEM1 size
    memory_Write32(0x3104, 0x01800000); // Simulated MEM1 size
    memory_Write32(0x3108, 0x81800000); // ?
    memory_Write32(0x3110, 0x81800000); // MEM1 arena end
    memory_Write32(0x3114, 0xDEADBEEF); // ?
    memory_Write32(0x3118, 0x04000000); // Physical MEM2 size
    memory_Write32(0x311C, 0x04000000); // Simulated MEM2 size
    memory_Write32(0x3120, 0x93600000); // PPC MEM2 end
    memory_Write32(0x3124, 0x90000800); // User MEM2 start
    memory_Write32(0x3128, 0x935E0000); // User MEM2 end
    memory_Write32(0x312C, 0xDEADBEEF); // ?
    memory_Write32(0x3130, 0x935E0000); // IPC buffer start
    memory_Write32(0x3134, 0x93600000); // IPC buffer end
    memory_Write32(0x3138, 0x00000011); // Hollywood version
    memory_Write32(0x313C, 0xDEADBEEF); // ?
    memory_Write32(0x3140, 0x00501C20); // IOS80 v7200
    memory_Write32(0x3144, 0x00092711); // IOS build date (09/27/11)
    memory_Write32(0x3148, 0x93600000); // IOS heap start
    memory_Write32(0x314C, 0x93620000); // IOS heap end
    memory_Write32(0x3150, 0xDEADBEEF); // ?
    memory_Write32(0x3154, 0xDEADBEEF); // ?
    memory_Write32(0x3158, 0x00000029); // GDDR vendor code
    memory_Write32(0x315C, 0xDEADBEEF); // ?
    memory_Write32(0x3160, 0x00000000); // Init semaphore
    memory_Write32(0x3164, 0x00000001); // GC mode (why is this 1?)
}

void nouwii_Initialize(const common_Config* config) {
    scheduler_Initialize();
    memory_Initialize();
    hle_Initialize();

    dev_di_Initialize();
    es_Initialize();
    fs_Initialize();

    ai_Initialize();
    broadway_Initialize();
    di_Initialize();
    dsp_Initialize();
    exi_Initialize();
    hollywood_Initialize();
    ipc_Initialize();
    mi_Initialize();
    pi_Initialize();
    si_Initialize();
    vi_Initialize();

    loader_SetDolPath(config->pathDol);
}

void nouwii_Reset() {
    scheduler_Reset();
    memory_Reset();
    hle_Reset();

    dev_di_Reset();
    es_Reset();
    fs_Reset();

    ai_Reset();
    broadway_Reset();
    di_Reset();
    dsp_Reset();
    exi_Reset();
    hollywood_Reset();
    ipc_Reset();
    mi_Reset();
    pi_Reset();
    si_Reset();
    vi_Reset();

    loader_LoadDol();

    InitializeGlobals();

    broadway_SetEntry(loader_GetEntry());
}

void nouwii_Shutdown() {
    scheduler_Shutdown();
    memory_Shutdown();
    hle_Shutdown();

    dev_di_Shutdown();
    es_Shutdown();
    fs_Shutdown();

    ai_Shutdown();
    broadway_Shutdown();
    di_Shutdown();
    dsp_Shutdown();
    exi_Shutdown();
    hollywood_Shutdown();
    ipc_Shutdown();
    mi_Shutdown();
    pi_Shutdown();
    si_Shutdown();
    vi_Shutdown();
}

void nouwii_Run() {
    while (NOUWII_TRUE) {
        scheduler_Run();
        broadway_Run();
    }
}

int main(int argc, char** argv) {
    if (argc < NUM_ARGS) {
        puts("Usage: nouwii [path to DOL]");
        return 1;
    }

    common_Config config;
    config.pathDol = argv[1];

    nouwii_Initialize(&config);
    nouwii_Reset();
    nouwii_Run();
    nouwii_Shutdown();

    return 0;
}
