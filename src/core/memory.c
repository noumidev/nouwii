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

#include "hw/ai.h"
#include "hw/di.h"
#include "hw/dsp.h"
#include "hw/exi.h"
#include "hw/hollywood.h"
#include "hw/mi.h"
#include "hw/pi.h"
#include "hw/si.h"
#include "hw/vi.h"

#define SIZE_ADDRESS_SPACE (0x100000000)
#define SIZE_PAGE (0x1000)
#define SIZE_PAGE_TABLE (SIZE_ADDRESS_SPACE / SIZE_PAGE)

enum {
    BASE_MEM1 = 0x00000000,
    BASE_VI   = 0x0C002000,
    BASE_PI   = 0x0C003000,
    BASE_MI   = 0x0C004000,
    BASE_DSP  = 0x0C005000,
    BASE_HW   = 0x0D000000,
    BASE_DI   = 0x0D006000,
    BASE_SI   = 0x0D006400,
    BASE_EXI  = 0x0D006800,
    BASE_AI   = 0x0D006C00,
    BASE_MEM2 = 0x10000000,
};

enum {
    SIZE_MEM1 = 0x1800000,
    SIZE_VI   = 0x0000100,
    SIZE_PI   = 0x0001000,
    SIZE_MI   = 0x0000080,
    SIZE_DSP  = 0x0000200,
    SIZE_HW   = 0x0000400,
    SIZE_DI   = 0x0000040,
    SIZE_SI   = 0x0000100,
    SIZE_EXI  = 0x0000080,
    SIZE_AI   = 0x0000020,
    SIZE_MEM2 = 0x4000000,
};

#define MAKEFUNC_READ(size)                                           \
u##size memory_Read##size(const u32 addr) {                           \
    const u32 page = addr / SIZE_PAGE;                                \
    const u32 offset = addr & (SIZE_PAGE - 1);                        \
                                                                      \
    if (ctx.tableRd[page] != NULL) {                                  \
        u##size data;                                                 \
        memcpy(&data, &ctx.tableRd[page][offset], sizeof(u##size));   \
        return common_Bswap##size(data);                              \
    }                                                                 \
                                                                      \
    return ReadIo##size(addr);                                        \
}                                                                     \

#define MAKEFUNC_READIO(size)                                \
u##size ReadIo##size(const u32 addr) {                       \
    if ((addr & ~(SIZE_VI - 1)) == BASE_VI) {                \
        return vi_ReadIo##size(addr);                        \
    }                                                        \
                                                             \
    if ((addr & ~(SIZE_PI - 1)) == BASE_PI) {                \
        return pi_ReadIo##size(addr);                        \
    }                                                        \
                                                             \
    if ((addr & ~(SIZE_MI - 1)) == BASE_MI) {                \
        return mi_ReadIo##size(addr);                        \
    }                                                        \
                                                             \
    if ((addr & ~(SIZE_DSP - 1)) == BASE_DSP) {              \
        return dsp_ReadIo##size(addr);                       \
    }                                                        \
                                                             \
    if ((addr & ~((SIZE_HW - 1) | (1 << 23))) == BASE_HW) {  \
        return hollywood_ReadIo##size(addr);                 \
    }                                                        \
                                                             \
    if ((addr & ~(SIZE_DI - 1)) == BASE_DI) {                \
        return di_ReadIo##size(addr);                        \
    }                                                        \
                                                             \
    if ((addr & ~(SIZE_SI - 1)) == BASE_SI) {                \
        return si_ReadIo##size(addr);                        \
    }                                                        \
                                                             \
    if ((addr & ~(SIZE_EXI - 1)) == BASE_EXI) {              \
        return exi_ReadIo##size(addr);                       \
    }                                                        \
                                                             \
    if ((addr & ~(SIZE_AI - 1)) == BASE_AI) {                \
        return ai_ReadIo##size(addr);                        \
    }                                                        \
                                                             \
    printf("Unmapped read%d (address: %08X)\n", size, addr); \
    exit(1);                                                 \
}                                                            \

#define MAKEFUNC_WRITE(size)                                                    \
void memory_Write##size(const u32 addr, const u##size data) {                   \
    const u32 page = addr / SIZE_PAGE;                                          \
    const u32 offset = addr & (SIZE_PAGE - 1);                                  \
                                                                                \
    if (ctx.tableWr[page] != NULL) {                                            \
        const u##size bswapData = common_Bswap##size(data);                     \
        memcpy(&ctx.tableWr[page][offset], &bswapData, sizeof(u##size));        \
        return;                                                                 \
    }                                                                           \
                                                                                \
    WriteIo##size(addr, data);                                                  \
}                                                                               \

#define MAKEFUNC_WRITEIO(size)                                                  \
void WriteIo##size(const u32 addr, const u##size data) {                        \
    if ((addr & ~(SIZE_VI - 1)) == BASE_VI) {                                   \
        vi_WriteIo##size(addr, data);                                           \
        return;                                                                 \
    }                                                                           \
                                                                                \
    if ((addr & ~(SIZE_PI - 1)) == BASE_PI) {                                   \
        pi_WriteIo##size(addr, data);                                           \
        return;                                                                 \
    }                                                                           \
                                                                                \
    if ((addr & ~(SIZE_MI - 1)) == BASE_MI) {                                   \
        mi_WriteIo##size(addr, data);                                           \
        return;                                                                 \
    }                                                                           \
                                                                                \
    if ((addr & ~(SIZE_DSP - 1)) == BASE_DSP) {                                 \
        dsp_WriteIo##size(addr, data);                                          \
        return;                                                                 \
    }                                                                           \
                                                                                \
    if ((addr & ~((SIZE_HW - 1) | (1 << 23))) == BASE_HW) {                     \
        hollywood_WriteIo##size(addr, data);                                    \
        return;                                                                 \
    }                                                                           \
                                                                                \
    if ((addr & ~(SIZE_DI - 1)) == BASE_DI) {                                   \
        di_WriteIo##size(addr, data);                                           \
        return;                                                                 \
    }                                                                           \
                                                                                \
    if ((addr & ~(SIZE_SI - 1)) == BASE_SI) {                                   \
        si_WriteIo##size(addr, data);                                           \
        return;                                                                 \
    }                                                                           \
                                                                                \
    if ((addr & ~(SIZE_EXI - 1)) == BASE_EXI) {                                 \
        exi_WriteIo##size(addr, data);                                          \
        return;                                                                 \
    }                                                                           \
                                                                                \
    if ((addr & ~(SIZE_AI - 1)) == BASE_AI) {                                   \
        ai_WriteIo##size(addr, data);                                           \
        return;                                                                 \
    }                                                                           \
                                                                                \
    printf("Unmapped write%d (address: %08X, data: %02X)\n", size, addr, data); \
    exit(1);                                                                    \
}                                                                               \

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

MAKEFUNC_READIO(8)
MAKEFUNC_READIO(16)
MAKEFUNC_READIO(32)
MAKEFUNC_READIO(64)

MAKEFUNC_READ(8)
MAKEFUNC_READ(16)
MAKEFUNC_READ(32)
MAKEFUNC_READ(64)

MAKEFUNC_WRITEIO(8)
MAKEFUNC_WRITEIO(16)
MAKEFUNC_WRITEIO(32)
MAKEFUNC_WRITEIO(64)

MAKEFUNC_WRITE(8)
MAKEFUNC_WRITE(16)
MAKEFUNC_WRITE(32)
MAKEFUNC_WRITE(64)

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
