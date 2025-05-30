/*
 * nouwii is a Nintendo Wii emulator.
 * Copyright (C) 2025  noumidev
 */

#include "core/loader.h"

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "common/buffer.h"
#include "common/file.h"

#include "core/memory.h"

#define MAX_TEXT (7)
#define MAX_DATA (11)

static const char* pathDol;

static u8* dol = NULL;

static u32 entry;

void loader_SetDolPath(const char *path) {
    pathDol = path;
}

void loader_LoadDol() {
    printf("Loading DOL %s\n", pathDol);

    const long size = common_LoadFile(pathDol, (void**)&dol);

    assert(size > 0);

    for (int i = 0; i < (MAX_TEXT + MAX_DATA); i++) {
        printf("Loading %s%d... ", (i < MAX_TEXT) ? "TEXT" : "DATA", (i >= MAX_TEXT) ? i - MAX_TEXT : i);

        const u32 offsetDol = sizeof(u32) * i;

        const u32 sizeSection = GET32(dol, size, 0x90 + offsetDol);

        if (sizeSection == 0) {
            printf("skipped\n");

            continue;
        }

        const u32 offset = GET32(dol, size, offsetDol);
        const u32 addr = GET32(dol, size, 0x48 + offsetDol);

        printf("size: %u, offset: %08X, addr: %08X\n", sizeSection, offset, addr);

        memcpy(memory_GetPointer(TO_PHYSICAL(addr)), &dol[offset], sizeSection);
    }
    
    const u32 addrBss = GET32(dol, size, 0xD8);
    const u32 sizeBss = GET32(dol, size, 0xDC);

    printf("Clearing BSS (address: %08X, size: %u)\n", addrBss, sizeBss);

    memset(memory_GetPointer(TO_PHYSICAL(addrBss)), 0, size);

    entry = GET32(dol, size, 0xE0);

    printf("Entry: %08X\n", entry);
}

u32 loader_GetEntry() {
    return TO_PHYSICAL(entry);
}
