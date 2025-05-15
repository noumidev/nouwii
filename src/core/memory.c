/*
 * nouwii is a Nintendo Wii emulator.
 * Copyright (C) 2025  noumidev
 */

#include "core/memory.h"

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "common/bswap.h"
#include "common/buffer.h"
#include "common/file.h"

#define SIZE_ADDRESS_SPACE (0x100000000)
#define SIZE_PAGE (0x1000)
#define SIZE_PAGE_TABLE (SIZE_ADDRESS_SPACE / SIZE_PAGE)

enum {
    BASE_MEM1 = 0,
    BASE_MEM2 = 0x10000000,
};

enum {
    SIZE_MEM1 = 0x1800000,
    SIZE_MEM2 = 0x4000000,
};

#define MAKEFUNC_READ(size)                                         \
u##size memory_Read##size(const u32 addr) {                         \
    const u32 page = addr / SIZE_PAGE;                              \
    const u32 offset = addr & (SIZE_PAGE - 1);                      \
                                                                    \
    if (ctx.tableRd[page] != NULL) {                                \
        u##size data;                                               \
        memcpy(&data, &ctx.tableRd[page][offset], sizeof(u##size)); \
        return common_Bswap##size(data);                            \
    }                                                               \
                                                                    \
    printf("Unmapped read##size (address: %08X)\n", addr);          \
    exit(1);                                                        \
}                                                                   \

#define MAKEFUNC_WRITE(size)                                                  \
void memory_Write##size(const u32 addr, const u##size data) {                 \
    const u32 page = addr / SIZE_PAGE;                                        \
    const u32 offset = addr & (SIZE_PAGE - 1);                                \
                                                                              \
    if (ctx.tableWr[page] != NULL) {                                          \
        const u##size bswapData = common_Bswap##size(data);                   \
        memcpy(&ctx.tableWr[page][offset], &bswapData, sizeof(u##size));      \
        return;                                                               \
    }                                                                         \
                                                                              \
    printf("Unmapped write##size (address: %08X, data: %02X)\n", addr, data); \
    exit(1);                                                                  \
}                                                                             \

typedef struct Context {
    u8** tableRd;
    u8** tableWr;

    u8* mem1;
    u8* mem2;
} Context;

static Context ctx;

void memory_Initialize(const char* pathMem1, const char* pathMem2) {
    memset(&ctx, 0, sizeof(ctx));

    ctx.tableRd = malloc(SIZE_PAGE_TABLE * sizeof(usize));
    ctx.tableWr = malloc(SIZE_PAGE_TABLE * sizeof(usize));

    common_LoadFile(pathMem1, (void**)&ctx.mem1);
    common_LoadFile(pathMem2, (void**)&ctx.mem2);
}

void memory_Reset() {
    memset(ctx.tableRd, 0, SIZE_PAGE_TABLE * sizeof(usize));
    memset(ctx.tableWr, 0, SIZE_PAGE_TABLE * sizeof(usize));

    memory_Map(ctx.mem1, BASE_MEM1, SIZE_MEM1, NOUWII_TRUE, NOUWII_TRUE);
    memory_Map(ctx.mem2, BASE_MEM2, SIZE_MEM2, NOUWII_TRUE, NOUWII_TRUE);
}

void memory_Shutdown() {
    free(ctx.tableRd);
    free(ctx.tableWr);
    free(ctx.mem1);
    free(ctx.mem2);
}

MAKEFUNC_READ(8)
MAKEFUNC_READ(16)
MAKEFUNC_READ(32)

MAKEFUNC_WRITE(8)
MAKEFUNC_WRITE(16)
MAKEFUNC_WRITE(32)

void memory_Map(u8* mem, const u32 addr, const u32 size, const int read, const int write) {
    assert(common_IsAligned(addr, SIZE_PAGE));
    assert(common_IsAligned(size, SIZE_PAGE));

    const u32 firstPage = addr / SIZE_PAGE;
    const u32 numPages = size / SIZE_PAGE;

    printf("Mapping %X pages to %08X (%s/%s)\n", numPages, addr, (read) ? "R" : "-", (write) ? "W" : "-");

    for (u32 page = firstPage; page < (firstPage + numPages); page++) {
        const u32 memIdx = page - firstPage;

        if (read) {
            assert(ctx.tableRd[page] == NULL);

            ctx.tableRd[page] = &mem[SIZE_PAGE * memIdx];
        }

        if (write) {
            assert(ctx.tableWr[page] == NULL);

            ctx.tableWr[page] = &mem[SIZE_PAGE * memIdx];
        }
    }
}

void* memory_GetPointer(const u32 addr) {
    const u32 page = addr / SIZE_PAGE;
    const u32 offset = addr & (SIZE_PAGE - 1);

    if (ctx.tableRd[page] != NULL) {
        return &ctx.tableRd[page][offset];
    }

    if (ctx.tableWr[page] != NULL) {
        return &ctx.tableWr[page][offset];
    }

    return NULL;
}
