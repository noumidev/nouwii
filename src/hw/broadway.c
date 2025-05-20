/*
 * nouwii is a Nintendo Wii emulator.
 * Copyright (C) 2025  noumidev
 */

#include "hw/broadway.h"

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "common/bit.h"
#include "common/types.h"
#include "core/memory.h"

#define BROADWAY_DEBUG

#define NUM_GPRS (32)
#define NUM_FPRS (32)
#define NUM_BATS (8)
#define NUM_GQRS (8)
#define NUM_CRS  (8)
#define NUM_PS   (2)

#define INITIAL_PC (0x3400)

enum {
    PRIMARY_PAIREDSINGLE =  4,
    PRIMARY_ADDI         = 14,
    PRIMARY_ADDIS        = 15,
    PRIMARY_BC           = 16,
    PRIMARY_B            = 18,
    PRIMARY_SYSTEM       = 19,
    PRIMARY_RLWINM       = 21,
    PRIMARY_ORI          = 24,
    PRIMARY_ORIS         = 25,
    PRIMARY_REGISTER     = 31,
    PRIMARY_LWZ          = 32,
    PRIMARY_STW          = 36,
    PRIMARY_STWU         = 37,
    PRIMARY_LFD          = 50,
    PRIMARY_PSQL         = 56,
    PRIMARY_FLOAT        = 63,
};

enum {
    SECONDARY_MFMSR =  83,
    SECONDARY_MTMSR = 146,
    SECONDARY_MTSR  = 210,
    SECONDARY_MFSPR = 339,
    SECONDARY_MTSPR = 467,
    SECONDARY_SYNC  = 598,
};

enum {
    SYSTEM_BCLR  =  16,
    SYSTEM_ISYNC = 150,
};

enum {
    PAIREDSINGLE_PSMR = 72,
};

enum {
    FLOAT_FMR   =  72,
    FLOAT_MTFSF = 711,
};

enum {
    SPR_LR     =    8,
    SPR_IBAT0U =  528,
    SPR_IBAT3L =  535,
    SPR_DBAT0U =  536,
    SPR_DBAT3L =  543,
    SPR_IBAT4U =  560,
    SPR_IBAT7L =  567,
    SPR_DBAT4U =  568,
    SPR_DBAT7L =  575,
    SPR_GQR0   =  912,
    SPR_GQR7   =  919,
    SPR_HID2   =  920,
    SPR_HID0   = 1008,
};

enum {
    QUANT_TYPE_FLOAT = 0,
};

#define MAKEFUNC_BROADWAY_READ(size)                        \
static u##size Read##size(const u32 addr, const int code) { \
    assert((addr & ((size / 8) - 1)) == 0);                 \
    return memory_Read##size(Translate(addr, code));        \
}                                                           \

#define MAKEFUNC_BROADWAY_WRITE(size)                         \
static void Write##size(const u32 addr, const u##size data) { \
    assert((addr & ((size / 8) - 1)) == 0);                   \
    memory_Write##size(Translate(addr, NOUWII_FALSE), data);  \
}                                                             \

#define TO_IBM_POS(n) (31 - n)

static u32 GetMask(const u32 start, const u32 end) {
    if (start <= end) {
        return (0xFFFFFFFFU << TO_IBM_POS(end)) & (0xFFFFFFFFU >> start);
    }

    return (0xFFFFFFFFU << TO_IBM_POS(end)) | (0xFFFFFFFFU >> start); 
}

static u32 GetBits(const u32 n, const u32 start, const u32 end) {
    return (n & GetMask(start, end)) >> TO_IBM_POS(end);
}

static u32 SetBits(const u32 n, const u32 start, const u32 end, const u32 data) {
    const u32 mask = GetMask(start, end);

    return (n & ~mask) | ((data << TO_IBM_POS(end)) & mask);
}

// Instruction fields
#define OPCD (GetBits(instr,  0,  5))
#define   XO (GetBits(instr, 21, 30))
#define   RA (GetBits(instr, 11, 15))
#define   RB (GetBits(instr, 16, 20))
#define   RD (GetBits(instr,  6, 10))
#define   RS (GetBits(instr,  6, 10))
#define   SH (GetBits(instr, 16, 20))
#define   ME (GetBits(instr, 21, 25))
#define   MB (GetBits(instr, 26, 30))
#define   FM (GetBits(instr,  7, 14))
#define   BO (GetBits(instr,  6, 10))
#define   BI (GetBits(instr, 11, 15))
#define   BD (GetBits(instr, 16, 29))
#define   LI (GetBits(instr,  6, 29))
#define    D (GetBits(instr, 20, 31))
#define    I (GetBits(instr, 17, 19))
#define    W (GetBits(instr, 16, 16) != 0)
#define   AA (GetBits(instr, 30, 30) != 0)
#define   RC (GetBits(instr, 31, 31) != 0)
#define   LK (GetBits(instr, 31, 31) != 0)
#define UIMM (GetBits(instr, 16, 31))
#define  SPR (GetBits(instr, 11, 15) | (GetBits(instr, 16, 20) << 5))

// Branch control fields
#define BO_TEST_COND (GetBits(instr, 6, 6) == 0)
#define BO_COND_TRUE (GetBits(instr, 7, 7) != 0)
#define BO_TEST_CTR  (GetBits(instr, 8, 8) == 0)
#define BO_CTR_ZERO  (GetBits(instr, 9, 9) != 0)

// Address fields
#define ADDR_OFFSET  (addr & 0x0001FFFF)
#define ADDR_PAGE    (addr & 0x0FFE0000)
#define ADDR_SEGMENT (addr & 0xF0000000)

// SPRs
#define   CTR (ctx.sprs.ctr)
#define    LR (ctx.sprs.lr)
#define  HID0 (ctx.sprs.hid0)
#define  HID2 (ctx.sprs.hid2)
#define DBATL (ctx.sprs.dbatl)
#define DBATU (ctx.sprs.dbatu)
#define IBATL (ctx.sprs.ibatl)
#define IBATU (ctx.sprs.ibatu)
#define   GQR (ctx.sprs.gqr)

#define    IA (ctx.ia)
#define   CIA (ctx.cia)
#define    CR (ctx.cr)
#define FPSCR (ctx.fpscr)
#define   MSR (ctx.msr)

#define PS0 ps[0]
#define PS1 ps[1]

typedef union Batl {
    u32 raw;

    struct {
        u32   pp :  2; // Protection bits
        u32      :  1;
        u32    g :  1; // Guarded
        u32    m :  1; // Memory coherence
        u32    i :  1; // Inhibited
        u32    w :  1; // Write-through
        u32      : 10;
        u32 brpn : 15; // Block real page number
    };
} Batl;

typedef union Batu {
    u32 raw;

    struct {
        u32   vp :  1; // User valid
        u32   vs :  1; // Supervisor valid
        u32   bl : 11; // Block length
        u32      :  4;
        u32 bepi : 15; // Block effective page index
    };
} Batu;

typedef struct SpecialRegs {
    union {
        u32 raw;

        struct {
            u32 noopti : 1; // No-op touch instructions
            u32        : 1;
            u32    bht : 1; // Branch history table enable
            u32    abe : 1; // Address broadcast enable
            u32        : 1;
            u32   btic : 1; // Branch target icache enable
            u32   dcfa : 1; // Dcache flush assist
            u32    sge : 1; // Store gathering enable
            u32   ifem : 1; // Instruction fetch M enable
            u32    spd : 1; // Speculative cache access disable
            u32   dcfi : 1; // Dcache flash invalidate (auto-clear after setting)
            u32   icfi : 1; // Icache flash invalidate (auto-clear after setting)
            u32  dlock : 1; // Dcache lock
            u32  ilock : 1; // Icache lock
            u32    dce : 1; // Dcache enable
            u32    ice : 1; // Icache enable
            u32    nhr : 1; // Not hard reset
            u32        : 3;
            u32    dpm : 1; // Dynamic power management
            u32  sleep : 1; // Sleep mode enable
            u32    nap : 1; // Nap mode enable
            u32   doze : 1; // Doze mode enable
            u32    par : 1; // Disable precharge
            u32   eclk : 1; // CLK_OUT enable
            u32        : 1;
            u32   bclk : 1; // CLK_OUT enable
            u32    ebd : 1; // Enable data parity checking
            u32    eba : 1; // Enable address parity checking
            u32    dbp : 1; // Disable bus parity generation
            u32   emcp : 1; // Enable MCP
        };
    } hid0;

    union {
        u32 raw;

        struct {
            u32        : 16;
            u32  dqoee :  1; // DMA queue overflow error enable
            u32  dcmee :  1; // DMA cache miss error enable
            u32  dncee :  1; // DMA normal cache access error enable
            u32  dchee :  1; // dcbz_l cache hit error enable
            u32 dqoerr :  1; // DMA queue overflow error
            u32 dcmerr :  1; // DMA cache miss error
            u32 dncerr :  1; // DMA normal cache access error
            u32 dcherr :  1; // dcbz_l cache hit error
            u32  dmaql :  4; // DMA queue length
            u32    lce :  1; // Locked cache enable
            u32    pse :  1; // Paired single enable
            u32    wpe :  1; // Write pipe enable
            u32   lsqe :  1; // Quantized load/store enable
        };
    } hid2;

    union {
        u32 raw;

        struct {
            u32  sttype : 3;
            u32         : 5;
            u32 stscale : 6;
            u32         : 2;
            u32  ldtype : 3;
            u32         : 5;
            u32 ldscale : 6;
            u32         : 2;
        };
    } gqr[NUM_GQRS];

    Batl dbatl[NUM_BATS], ibatl[NUM_BATS];
    Batu dbatu[NUM_BATS], ibatu[NUM_BATS];

    u32 lr;
    u32 ctr;
} SpecialRegs;

typedef struct Context {
    i64 cyclesToRun;

    u32 ia, cia;

    u32 r[NUM_GPRS];

    struct {
        f64 ps[NUM_PS];
    } fprs[NUM_FPRS];

    u32 cr, fpscr;

    SpecialRegs sprs;

    union {
        u32 raw;

        struct {
            u32  le :  1; // Little Endian
            u32  ri :  1; // Recoverable interrupt
            u32  pm :  1; // Process marked
            u32     :  1;
            u32  dr :  1; // Data address translation
            u32  ir :  1; // Data address translation
            u32  ip :  1; // Interrupt prefix
            u32     :  1;
            u32 fe1 :  1; // IEEE float exception mode 1
            u32  be :  1; // Branch trace enable
            u32  se :  1; // Single step enable
            u32 fe0 :  1; // IEEE float exception mode 0
            u32  me :  1; // Machine check enable
            u32  fp :  1; // Float available
            u32  pr :  1; // Privilege level
            u32  ee :  1; // External interrupt enable
            u32 ile :  1; // Interrupt Little Endian
            u32     :  1;
            u32 pow :  1; // Power management enable
            u32     : 13;
        };
    } msr;
} Context;

static Context ctx;

static void SetCr(const int cr, const u32 n) {
    const int index = 4 * cr;

    CR = SetBits(CR, index, index + 3, n);
}

static void SetFpscr(const int cr, const u32 n) {
    const int index = 4 * cr;

    CR = SetBits(CR, index, index + 3, n);
}

static void SetFlags(const int cr, const u32 n) {
    // TODO: Implement XER
    const u32 so = 0;
    const u32 eq = n == 0;
    const u32 gt = (i32)n > 0;
    const u32 lt = (i32)n < 0;

    SetCr(cr, (lt << 3) | (gt << 2) | (eq << 1) | so);
}

static u32 Translate(const u32 addr, const int code) {
    if (!((code && (MSR.ir != 0)) || (!code && (MSR.dr != 0)))) {
        return addr;
    }

    // https://mariokartwii.com/showthread.php?tid=1963

    const Batl* batl = DBATL;
    const Batu* batu = DBATU;

    if (code) {
        batl = IBATL;
        batu = IBATU;
    }

    for (int i = 0; i < NUM_BATS; i++) {
        const u32 length = batu[i].bl << 17;

        const u32 index = ADDR_SEGMENT | (ADDR_PAGE & ~length);

        if ((u32)(batu[i].bepi << 17) == index) {
            // BAT hit
            return (batl[i].brpn << 17) | (ADDR_PAGE & length) | ADDR_OFFSET;
        }
    }

    printf("BAT miss (address: %08X)\n", addr);

    exit(1);
}

MAKEFUNC_BROADWAY_READ(8)
MAKEFUNC_BROADWAY_READ(16)
MAKEFUNC_BROADWAY_READ(32)
MAKEFUNC_BROADWAY_READ(64)

static u32 FetchInstr() {
    CIA = IA;

    const u32 instr = Read32(IA, NOUWII_TRUE);

    IA += sizeof(instr);

    return instr;
}

MAKEFUNC_BROADWAY_WRITE(8)
MAKEFUNC_BROADWAY_WRITE(16)
MAKEFUNC_BROADWAY_WRITE(32)
MAKEFUNC_BROADWAY_WRITE(64)

static u32 GetSpr(const u32 spr) {
    switch (spr) {
        case SPR_LR:
            printf("LR read\n");

            return LR;
        case SPR_HID2:
            printf("HID2 read\n");

            return HID2.raw;
        case SPR_HID0:
            printf("HID0 read\n");

            return HID0.raw;
        default:
            printf("Unimplemented SPR%u read\n", spr);

            exit(1);
    }
}

static void SetSpr(const u32 spr, const u32 data) {
    if (((spr >= SPR_IBAT0U) && (spr <= SPR_IBAT3L)) ||
        ((spr >= SPR_IBAT4U) && (spr <= SPR_IBAT7L))) {
        u32 idx = (spr - SPR_IBAT0U) / 2;

        if (spr >= SPR_IBAT4U) {
            idx = 4 + (spr - SPR_IBAT4U) / 2;
        }

        if ((spr & 1) != 0) {
            printf("IBAT%uL write (data: %08X)\n", idx, data);

            IBATL[idx].raw = data;
        } else {
            printf("IBAT%uU write (data: %08X)\n", idx, data);

            IBATU[idx].raw = data;
        }

        return;
    }

    if (((spr >= SPR_DBAT0U) && (spr <= SPR_DBAT3L)) ||
        ((spr >= SPR_DBAT4U) && (spr <= SPR_DBAT7L))) {
        u32 idx = (spr - SPR_DBAT0U) / 2;

        if (spr >= SPR_DBAT4U) {
            idx = 4 + (spr - SPR_DBAT4U) / 2;
        }

        if ((spr & 1) != 0) {
            printf("DBAT%uL write (data: %08X)\n", idx, data);

            DBATL[idx].raw = data;
        } else {
            printf("DBAT%uU write (data: %08X)\n", idx, data);

            DBATU[idx].raw = data;
        }

        return;
    }

    if ((spr >= SPR_GQR0) && (spr <= SPR_GQR7)) {
        const u32 idx = spr - SPR_GQR0;

        printf("GQR%u write (data: %08X)\n", idx, data);

        GQR[idx].raw = data;

        return;
    }

    switch (spr) {
        case SPR_LR:
            printf("LR write (data: %08X)\n", data);

            LR = data;
            break;
        case SPR_HID2:
            printf("HID2 write (data: %08X)\n", data);

            HID2.raw = data;

            if (HID2.pse != 0) {
                printf("HID2 Paired Singles enabled\n");
            }

            if (HID2.wpe != 0) {
                printf("HID2 Write-gather pipe enabled\n");
            }

            if (HID2.lsqe != 0) {
                printf("HID2 Quantized loadstores enabled\n");
            }
            break;
        case SPR_HID0:
            printf("HID0 write (data: %08X)\n", data);

            HID0.raw = data;

            if (HID0.dce != 0) {
                printf("HID0 D$ enabled\n");
            }

            if (HID0.ice != 0) {
                printf("HID0 I$ enabled\n");
            }

            if (HID0.dcfi != 0) {
                printf("HID0 D$ flash invalidate\n");
            }

            if (HID0.icfi != 0) {
                printf("HID0 I$ flash invalidate\n");
            }

            // Clear flash invalidate bits
            HID0.dcfi = 0;
            HID0.icfi = 0;
            break;
        default:
            printf("Unimplemented SPR%u write (data: %08X)\n", spr, data);

            exit(1);
    }
}

static void ADDI(const u32 instr) {
    u32 n = (i16)UIMM;

    if (RA != 0) {
        n += ctx.r[RA];
    }

    ctx.r[RD] = n;

#ifdef BROADWAY_DEBUG
    printf("PPC [%08X] addi r%u, r%u, %X; r%u: %08X\n", CIA, RD, RA, UIMM, RD, ctx.r[RD]);
#endif
}

static void ADDIS(const u32 instr) {
    u32 n = UIMM << 16;

    if (RA != 0) {
        n += ctx.r[RA];
    }

    ctx.r[RD] = n;

#ifdef BROADWAY_DEBUG
    printf("PPC [%08X] addis r%u, r%u, %X; r%u: %08X\n", CIA, RD, RA, UIMM, RD, ctx.r[RD]);
#endif
}

static void B(const u32 instr) {
    u32 target = (i32)(LI << 8) >> 6;

    if (!AA) {
        target += CIA;
    }

    IA = target;

    if (LK) {
        LR = CIA + sizeof(instr);
    }

#ifdef BROADWAY_DEBUG
    printf("PPC [%08X] bc%s%s %08X; NIA: %08X, LR: %08X\n", CIA, (LK) ? "l" : "", (AA) ? "a" : "", target, IA, LR);
#endif
}

static void BC(const u32 instr) {
    if (BO_TEST_CTR) {
        CTR--;
    }

    const int ctrOk = !BO_TEST_CTR || ((CTR != 0) != BO_CTR_ZERO);
    const int condOk = !BO_TEST_COND || (GetBits(CR, BI, BI) == BO_COND_TRUE);

    u32 target = (i16)(BD << 2);

    if (!AA) {
        target += CIA;
    }

    if (ctrOk && condOk) {
        IA = target;

        if (LK) {
            LR = CIA + sizeof(instr);
        }
    }

#ifdef BROADWAY_DEBUG
    printf("PPC [%08X] bc%s%s %u, %u, %08X; NIA: %08X, LR: %08X\n", CIA, (LK) ? "l" : "", (AA) ? "a" : "", BO, BI, target, IA, LR);
#endif
}

static void BCLR(const u32 instr) {
    if (BO_TEST_CTR) {
        CTR--;
    }

    const int ctrOk = !BO_TEST_CTR || ((CTR != 0) != BO_CTR_ZERO);
    const int condOk = !BO_TEST_COND || (GetBits(CR, BI, BI) == BO_COND_TRUE);

    if (ctrOk && condOk) {
        IA = LR;

        if (LK) {
            LR = CIA + sizeof(instr);
        }
    }

#ifdef BROADWAY_DEBUG
    printf("PPC [%08X] bclr%s %u, %u; NIA: %08X, LR: %08X\n", CIA, (LK) ? "l" : "", BO, BI, IA, LR);
#endif
}

static void FMR(const u32 instr) {
    // TODO: Implement flags
    assert(!RC);

    ctx.fprs[RD].PS0 = ctx.fprs[RB].PS0;

    if (HID2.pse == 0) {
        ctx.fprs[RD].PS1 = ctx.fprs[RB].PS0;
    }

#ifdef BROADWAY_DEBUG
    printf("PPC [%08X] fmr%s f%u, f%u; ps0: %lf, ps1: %lf\n", CIA, (RC) ? "." : "", RD, RB, ctx.fprs[RD].PS0, ctx.fprs[RD].PS1);
#endif
}

static void ISYNC(const u32 instr) {
    (void)instr;

#ifdef BROADWAY_DEBUG
    printf("PPC [%08X] isync\n", CIA);
#endif
}

static void LFD(const u32 instr) {
    u32 addr = (i16)UIMM;

    if (RA != 0) {
        addr += ctx.r[RA];
    }

    ctx.fprs[RD].PS0 = common_ToF64(Read64(addr, NOUWII_FALSE));

#ifdef BROADWAY_DEBUG
    printf("PPC [%08X] lfd f%u, %d(r%u); f%u: %lf [%08X]\n", CIA, RD, (i16)UIMM, RA, RD, ctx.fprs[RD].PS0, addr);
#endif
}

static void LWZ(const u32 instr) {
    u32 addr = (i16)UIMM;

    if (RA != 0) {
        addr += ctx.r[RA];
    }

    ctx.r[RD] = Read32(addr, NOUWII_FALSE);

#ifdef BROADWAY_DEBUG
    printf("PPC [%08X] lzw r%u, %d(r%u); r%u: %08X [%08X]\n", CIA, RD, (i16)UIMM, RA, RD, ctx.r[RD], addr);
#endif
}

static void MFMSR(const u32 instr) {
    ctx.r[RD] = MSR.raw;

#ifdef BROADWAY_DEBUG
    printf("PPC [%08X] mfmsr r%u; r%u: %08X\n", CIA, RD, RD, ctx.r[RD]);
#endif
}

static void MFSPR(const u32 instr) {
    ctx.r[RD] = GetSpr(SPR);

#ifdef BROADWAY_DEBUG
    printf("PPC [%08X] mfspr r%u, spr%u; r%u: %08X\n", CIA, RD, SPR, RD, ctx.r[RD]);
#endif
}

static void MTFSF(const u32 instr) {
    const u32 n = (u32)common_FromF64(ctx.fprs[RB].PS0);

    for (int i = 0; i < NUM_CRS; i++) {
        if ((FM & (1 << (NUM_CRS - i - 1))) != 0) {
            SetFpscr(i, n);
        }
    }

#ifdef BROADWAY_DEBUG
    printf("PPC [%08X] mtfsf %02X, f%u; fpscr: %08X\n", CIA, FM, RB, FPSCR);
#endif
}

static void MTMSR(const u32 instr) {
    MSR.raw = ctx.r[RS];

#ifdef BROADWAY_DEBUG
    printf("PPC [%08X] mtmsr r%u; msr: %08X\n", CIA, RS, ctx.r[RS]);
#endif
}

static void MTSPR(const u32 instr) {
    SetSpr(SPR, ctx.r[RS]);

#ifdef BROADWAY_DEBUG
    printf("PPC [%08X] mtspr spr%u, r%u; spr%u: %08X\n", CIA, SPR, RS, SPR, ctx.r[RS]);
#endif
}

static void MTSR(const u32 instr) {
    // TODO: Implement SRs
#ifdef BROADWAY_DEBUG
    printf("PPC [%08X] mtsr sr%u, r%u; sr%u: %08X\n", CIA, RA, RS, RA, ctx.r[RS]);
#endif
}

static void ORI(const u32 instr) {
    ctx.r[RA] = ctx.r[RS] | UIMM;

#ifdef BROADWAY_DEBUG
    printf("PPC [%08X] ori r%u, r%u, %X; r%u: %08X\n", CIA, RA, RS, UIMM, RA, ctx.r[RA]);
#endif
}

static void ORIS(const u32 instr) {
    ctx.r[RA] = ctx.r[RS] | (UIMM << 16);

#ifdef BROADWAY_DEBUG
    printf("PPC [%08X] oris r%u, r%u, %X; r%u: %08X\n", CIA, RA, RS, UIMM, RA, ctx.r[RA]);
#endif
}

static void PSMR(const u32 instr) {
    assert(HID2.pse != 0);

    // TODO: Implement flags
    assert(!RC);

    ctx.fprs[RD] = ctx.fprs[RB];

#ifdef BROADWAY_DEBUG
    printf("PPC [%08X] ps_mr%s f%u, f%u; ps0: %lf, ps1: %lf\n", CIA, (RC) ? "." : "", RD, RB, ctx.fprs[RD].PS0, ctx.fprs[RD].PS1);
#endif
}

static void PSQL(const u32 instr) {
    assert((HID2.pse != 0) && (HID2.lsqe != 0));

    u32 addr = (i32)(D << 20) >> 20;

    if (RA != 0) {
        addr += ctx.r[RA];
    }

    // TODO: Implement scaling
    assert(GQR[I].ldscale == 0);

    switch (GQR[I].ldtype) {
        case QUANT_TYPE_FLOAT:
            ctx.fprs[RD].PS0 = (f64)common_ToF32(Read32(addr, NOUWII_FALSE));
            break;
        default:
            printf("Unimplemented psq_l dequantization type %u\n", GQR[I].ldtype);
            exit(1);
    }

    if (W) {
        ctx.fprs[RD].PS1 = 1.0;
    } else {
        switch (GQR[I].ldtype) {
            case QUANT_TYPE_FLOAT:
                ctx.fprs[RD].PS1 = (f64)common_ToF32(Read32(addr + sizeof(f32), NOUWII_FALSE));
                break;
            default:
                printf("Unimplemented psq_l dequantization type %u\n", GQR[I].ldtype);
                exit(1);
        }
    }

#ifdef BROADWAY_DEBUG
    printf("PPC [%08X] psq_l f%u, %d(r%u), %d, %u; ps0: %lf, ps1: %lf [%08X]\n", CIA, RD, (i32)(D << 20) >> 20, RA, W, I, ctx.fprs[RD].PS0, ctx.fprs[RD].PS1, addr);
#endif
}

static void RLWINM(const u32 instr) {
    ctx.r[RA] = common_Rotl(ctx.r[RS], SH) & GetMask(MB, ME);

    if (RC) {
        SetFlags(0, ctx.r[RA]);
    }

#ifdef BROADWAY_DEBUG
    printf("PPC [%08X] rlwinm%s r%u, r%u, %u, %u, %u; r%u: %08X, cr: %08X\n", CIA, (RC) ? "." : "", RA, RS, SH, MB, ME, RA, ctx.r[RA], CR);
#endif
}

static void STW(const u32 instr) {
    u32 addr = (i16)UIMM;

    if (RA != 0) {
        addr += ctx.r[RA];
    }

    Write32(addr, ctx.r[RS]);

#ifdef BROADWAY_DEBUG
    printf("PPC [%08X] stw r%u, %d(r%u); [%08X]: %08X\n", CIA, RS, (i16)UIMM, RA, addr, ctx.r[RS]);
#endif
}

static void STWU(const u32 instr) {
    assert(RA != 0);

    const u32 addr = (i16)UIMM + ctx.r[RA];
    const u32 data = ctx.r[RS];

    Write32(addr, data);

    ctx.r[RA] = addr;

#ifdef BROADWAY_DEBUG
    printf("PPC [%08X] stwu r%u, %d(r%u); [%08X]: %08X\n", CIA, RS, (i16)UIMM, RA, addr, ctx.r[RS]);
#endif
}

static void SYNC(const u32 instr) {
    (void)instr;

#ifdef BROADWAY_DEBUG
    printf("PPC [%08X] sync\n", CIA);
#endif
}

static void ExecInstr(const u32 instr) {
    switch (OPCD) {
        case PRIMARY_PAIREDSINGLE:
            switch (XO) {
                case PAIREDSINGLE_PSMR:
                    PSMR(instr);
                    break;
                default:
                    printf("Unimplemented Broadway Paired Single opcode %u (IA: %08X, instruction: %08X)\n", XO, CIA, instr);

                    exit(1);
            }
            break;
        case PRIMARY_ADDI:
            ADDI(instr);
            break;
        case PRIMARY_ADDIS:
            ADDIS(instr);
            break;
        case PRIMARY_BC:
            BC(instr);
            break;
        case PRIMARY_B:
            B(instr);
            break;
        case PRIMARY_SYSTEM:
            switch (XO) {
                case SYSTEM_BCLR:
                    BCLR(instr);
                    break;
                case SYSTEM_ISYNC:
                    ISYNC(instr);
                    break;
                default:
                    printf("Unimplemented Broadway system opcode %u (IA: %08X, instruction: %08X)\n", XO, CIA, instr);

                    exit(1);
            }
            break;
        case PRIMARY_RLWINM:
            RLWINM(instr);
            break;
        case PRIMARY_ORI:
            ORI(instr);
            break;
        case PRIMARY_ORIS:
            ORIS(instr);
            break;
        case PRIMARY_REGISTER:
            switch (XO) {
                case SECONDARY_MFMSR:
                    MFMSR(instr);
                    break;
                case SECONDARY_MTMSR:
                    MTMSR(instr);
                    break;
                case SECONDARY_MTSR:
                    MTSR(instr);
                    break;
                case SECONDARY_MFSPR:
                    MFSPR(instr);
                    break;
                case SECONDARY_MTSPR:
                    MTSPR(instr);
                    break;
                case SECONDARY_SYNC:
                    SYNC(instr);
                    break;
                default:
                    printf("Unimplemented Broadway secondary opcode %u (IA: %08X, instruction: %08X)\n", XO, CIA, instr);

                    exit(1);
            }
            break;
        case PRIMARY_LWZ:
            LWZ(instr);
            break;
        case PRIMARY_STW:
            STW(instr);
            break;
        case PRIMARY_STWU:
            STWU(instr);
            break;
        case PRIMARY_LFD:
            LFD(instr);
            break;
        case PRIMARY_PSQL:
            PSQL(instr);
            break;
        case PRIMARY_FLOAT:
            switch (XO) {
                case FLOAT_FMR:
                    FMR(instr);
                    break;
                case FLOAT_MTFSF:
                    MTFSF(instr);
                    break;
                default:
                    printf("Unimplemented Broadway float opcode %u (IA: %08X, instruction: %08X)\n", XO, CIA, instr);

                    exit(1);
            }
            break;
        default:
            printf("Unimplemented Broadway primary opcode %u (IA: %08X, instruction: %08X)\n", OPCD, CIA, instr);

            exit(1);
    }
}

void broadway_Initialize() {

}

void broadway_Reset() {
    memset(&ctx, 0, sizeof(ctx));

    // HLE boot stub
    IA = INITIAL_PC;
}

void broadway_Shutdown() {

}

void broadway_Run() {
    for (; ctx.cyclesToRun > 0; ctx.cyclesToRun--) {
        const u32 instr = FetchInstr();

        ExecInstr(instr);
    }
}

i64* broadway_GetCyclesToRun() {
    return &ctx.cyclesToRun;
}
