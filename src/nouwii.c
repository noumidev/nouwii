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

#define NUM_ARGS (1 + 2)

void nouwii_Initialize(const common_Config* config) {
    scheduler_Initialize();
    memory_Initialize(config->pathMem1, config->pathMem2);
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

        // hle_Tick(MAX_CYCLES_TO_RUN);
    }
}

int main(int argc, char** argv) {
    if (argc < NUM_ARGS) {
        puts("Usage: nouwii [path to MEM1 dump] [path to MEM2 dump]");
        return 1;
    }

    common_Config config;
    config.pathMem1 = argv[1];
    config.pathMem2 = argv[2];

    nouwii_Initialize(&config);
    nouwii_Reset();
    nouwii_Run();
    nouwii_Shutdown();

    return 0;
}
