/*
 * nouwii is a Nintendo Wii emulator.
 * Copyright (C) 2025  noumidev
 */

#include "hw/broadway.h"

#include <assert.h>
#include <stdint.h>
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
    PRIMARY_MULLI        =  7,
    PRIMARY_CMPLI        = 10,
    PRIMARY_CMPI         = 11,
    PRIMARY_ADDICrc      = 13,
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
    PRIMARY_LBZ          = 34,
    PRIMARY_STW          = 36,
    PRIMARY_STWU         = 37,
    PRIMARY_STB          = 38,
    PRIMARY_STH          = 44,
    PRIMARY_LFS          = 48,
    PRIMARY_LFD          = 50,
    PRIMARY_STFS         = 52,
    PRIMARY_PSQL         = 56,
    PRIMARY_PSQST        = 60,
    PRIMARY_FLOAT        = 63,
};

enum {
    SECONDARY_LWZX  =  23,
    SECONDARY_CMPL  =  32,
    SECONDARY_MFMSR =  83,
    SECONDARY_NOR   = 124,
    SECONDARY_MTMSR = 146,
    SECONDARY_STWX  = 151,
    SECONDARY_MTSR  = 210,
    SECONDARY_MULLW = 235,
    SECONDARY_ADD   = 266,
    SECONDARY_MFSPR = 339,
    SECONDARY_STHX  = 407,
    SECONDARY_OR    = 444,
    SECONDARY_MTSPR = 467,
    SECONDARY_DIVW  = 491,
    SECONDARY_SYNC  = 598,
    SECONDARY_EXTSB = 954,
};

enum {
    SYSTEM_BCLR  =  16,
    SYSTEM_ISYNC = 150,
    SYSTEM_CRXOR = 193,
    SYSTEM_BCCTR = 528,
};

enum {
    PAIREDSINGLE_PSMR      =  72,
    PAIREDSINGLE_PSMERGE01 = 560,
    PAIREDSINGLE_PSMERGE10 = 592,
};

enum {
    FLOAT_FMR   =  72,
    FLOAT_MTFSF = 711,
};

enum {
    SPR_XER    =    1,
    SPR_LR     =    8,
    SPR_CTR    =    9,
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
    SPR_L2CR   = 1017,
};

enum {
    COND_SO = 0,
    COND_EQ = 1,
    COND_GT = 2,
    COND_LT = 3,
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
#define CRFD (GetBits(instr,  6,  8))
#define   SH (GetBits(instr, 16, 20))
#define   MB (GetBits(instr, 21, 25))
#define   ME (GetBits(instr, 26, 30))
#define   FM (GetBits(instr,  7, 14))
#define   BO (GetBits(instr,  6, 10))
#define   BI (GetBits(instr, 11, 15))
#define   BD (GetBits(instr, 16, 29))
#define   LI (GetBits(instr,  6, 29))
#define    D (GetBits(instr, 20, 31))
#define    I (GetBits(instr, 17, 19))
#define    W (GetBits(instr, 16, 16) != 0)
#define    L (GetBits(instr, 10, 10) != 0)
#define   AA (GetBits(instr, 30, 30) != 0)
#define   RC (GetBits(instr, 31, 31) != 0)
#define   LK (GetBits(instr, 31, 31) != 0)
#define UIMM (GetBits(instr, 16, 31))
#define SIMM ((i16)GetBits(instr, 16, 31))
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
#define   XER (ctx.sprs.xer)
#define   CTR (ctx.sprs.ctr)
#define    LR (ctx.sprs.lr)
#define  HID0 (ctx.sprs.hid0)
#define  HID2 (ctx.sprs.hid2)
#define DBATL (ctx.sprs.dbatl)
#define DBATU (ctx.sprs.dbatu)
#define IBATL (ctx.sprs.ibatl)
#define IBATU (ctx.sprs.ibatu)
#define   GQR (ctx.sprs.gqr)
#define  L2CR (ctx.sprs.l2cr)

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
            u32 bytecount :  7;
            u32           : 22;
            u32        ca :  1;
            u32        ov :  1;
            u32        so :  1;
        };
    } xer;

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

    union {
        u32 raw;

        struct {
            u32 l2ip :  1; // L2 global invalidate in progress
            u32      : 17;
            u32 l2ts :  1; // L2 test support
            u32 l2wt :  1; // L2 write-through
            u32      :  1;
            u32  l2i :  1; // L2 global invalidate
            u32      :  6;
            u32 l2ce :  1; // L2 checkstop enable
            u32  l2e :  1; // L2 enable
        };
    } l2cr;

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

    FPSCR = SetBits(FPSCR, index, index + 3, n);
}

static void SetFlags(const int cr, const u32 n) {
    const u32 so = XER.so;
    const u32 eq = n == 0;
    const u32 gt = (i32)n > 0;
    const u32 lt = (i32)n < 0;

    SetCr(cr, (lt << COND_LT) | (gt << COND_GT) | (eq << COND_EQ) | so);
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
        case SPR_XER:
            printf("XER read\n");

            return XER.raw;
        case SPR_LR:
            printf("LR read\n");

            return LR;
        case SPR_CTR:
            printf("CTR read\n");

            return CTR;
        case SPR_HID2:
            printf("HID2 read\n");

            return HID2.raw;
        case SPR_HID0:
            printf("HID0 read\n");

            return HID0.raw;
        case SPR_L2CR:
            printf("L2CR read\n");

            return L2CR.raw;
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
        case SPR_XER:
            printf("XER write (data: %08X)\n", data);

            XER.raw = data;
            break;
        case SPR_LR:
            printf("LR write (data: %08X)\n", data);

            LR = data;
            break;
        case SPR_CTR:
            printf("CTR write (data: %08X)\n", data);

            CTR = data;
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
        case SPR_L2CR:
            printf("L2CR write (data: %08X)\n", data);

            L2CR.raw = data;

            if (L2CR.l2i != 0) {
                printf("L2CR global invalidate\n");

                // Simulate L2 invalidation
                L2CR.l2ip = 0;
            }

            if (L2CR.l2e != 0) {
                printf("L2CR L2 enabled\n");
            }
            break;
        default:
            printf("Unimplemented SPR%u write (data: %08X)\n", spr, data);

            exit(1);
    }
}

static void ADD(const u32 instr) {
    ctx.r[RD] = ctx.r[RA] + ctx.r[RB];

    if (RC) {
        SetFlags(0, ctx.r[RD]);
    }

#ifdef BROADWAY_DEBUG
    printf("PPC [%08X] add%s r%u, r%u, r%u; r%u: %08X\n", CIA, (RC) ? "." : "", RD, RA, RB, RD, ctx.r[RD]);
#endif
}

static void ADDI(const u32 instr) {
    u32 n = SIMM;

    if (RA != 0) {
        n += ctx.r[RA];
    }

    ctx.r[RD] = n;

#ifdef BROADWAY_DEBUG
    printf("PPC [%08X] addi r%u, r%u, %X; r%u: %08X\n", CIA, RD, RA, UIMM, RD, ctx.r[RD]);
#endif
}

static void ADDICrc(const u32 instr) {
    const u64 n = (u64)ctx.r[RA] + (u64)SIMM;

    XER.ca = (n >> 32) & 1;

    ctx.r[RD] = (u32)n;

    SetFlags(0, ctx.r[RD]);

#ifdef BROADWAY_DEBUG
    printf("PPC [%08X] addic. r%u, r%u, %X; r%u: %08X\n", CIA, RD, RA, UIMM, RD, ctx.r[RD]);
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
    printf("PPC [%08X] bc%s%s %08X; nia: %08X, lr: %08X\n", CIA, (LK) ? "l" : "", (AA) ? "a" : "", target, IA, LR);
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
    printf("PPC [%08X] bc%s%s %u, %u, %08X; nia: %08X, lr: %08X\n", CIA, (LK) ? "l" : "", (AA) ? "a" : "", BO, BI, target, IA, LR);
#endif
}

static void BCCTR(const u32 instr) {
    assert(!BO_TEST_CTR);

    const int condOk = !BO_TEST_COND || (GetBits(CR, BI, BI) == BO_COND_TRUE);

    if (condOk) {
        IA = CTR & ~3;

        if (LK) {
            LR = CIA + sizeof(instr);
        }
    }

#ifdef BROADWAY_DEBUG
    printf("PPC [%08X] bcctr%s %u, %u; nia: %08X, lr: %08X\n", CIA, (LK) ? "l" : "", BO, BI, IA, LR);
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
    printf("PPC [%08X] bclr%s %u, %u; nia: %08X, lr: %08X\n", CIA, (LK) ? "l" : "", BO, BI, IA, LR);
#endif
}

void CMPI(const u32 instr) {
    assert(!L);

    const i32 a = (i32)ctx.r[RA];

    u32 n = XER.so;

    if (a < SIMM) {
        n |= 1 << COND_LT;
    } else if (a > SIMM) {
        n |= 1 << COND_GT;
    } else {
        n |= 1 << COND_EQ;
    }

    SetCr(CRFD, n);

#ifdef BROADWAY_DEBUG
    printf("PPC [%08X] cmpi crf%u, %d, r%u, %X; cr: %08X\n", CIA, CRFD, L, RA, SIMM, CR);
#endif
}

void CMPL(const u32 instr) {
    assert(!L);

    const u32 a = ctx.r[RA];
    const u32 b = ctx.r[RB];

    u32 n = XER.so;

    if (a < b) {
        n |= 1 << COND_LT;
    } else if (a > b) {
        n |= 1 << COND_GT;
    } else {
        n |= 1 << COND_EQ;
    }

    SetCr(CRFD, n);

#ifdef BROADWAY_DEBUG
    printf("PPC [%08X] cmpl crf%u, %d, r%u, r%u; cr: %08X\n", CIA, CRFD, L, RA, RB, CR);
#endif
}

void CMPLI(const u32 instr) {
    assert(!L);

    const u32 a = ctx.r[RA];

    u32 n = XER.so;

    if (a < UIMM) {
        n |= 1 << COND_LT;
    } else if (a > UIMM) {
        n |= 1 << COND_GT;
    } else {
        n |= 1 << COND_EQ;
    }

    SetCr(CRFD, n);

#ifdef BROADWAY_DEBUG
    printf("PPC [%08X] cmpli crf%u, %d, r%u, %X; cr: %08X\n", CIA, CRFD, L, RA, UIMM, CR);
#endif
}

static void CRXOR(const u32 instr) {
    CR = SetBits(CR, RD, RD, GetBits(CR, RA, RA) ^ GetBits(CR, RB, RB));

#ifdef BROADWAY_DEBUG
    printf("PPC [%08X] crxor crb%u, crb%u, crb%u; cr: %08X\n", CIA, RD, RA, RB, CR);
#endif
}

static void DIVW(const u32 instr) {
    const i32 n = (i32)ctx.r[RA];
    const i32 d = (i32)ctx.r[RB];

    assert(d != 0);
    assert(!((n == INT32_MIN) && (d == -1)));

    ctx.r[RD] = (u32)(n / d);

    if (RC) {
        SetFlags(0, ctx.r[RD]);
    }

#ifdef BROADWAY_DEBUG
    printf("PPC [%08X] divw%s r%u, r%u, r%u; r%u: %08X\n", CIA, (RC) ? "." : "", RD, RA, RB, RD, ctx.r[RD]);
#endif
}

static void EXTSB(const u32 instr) {
    ctx.r[RA] = (i8)ctx.r[RS];

    if (RC) {
        SetFlags(0, ctx.r[RA]);
    }

#ifdef BROADWAY_DEBUG
    printf("PPC [%08X] extsb%s r%u, r%u; r%u: %08X\n", CIA, (RC) ? "." : "", RA, RS, RA, ctx.r[RA]);
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

static void LBZ(const u32 instr) {
    u32 addr = SIMM;

    if (RA != 0) {
        addr += ctx.r[RA];
    }

    ctx.r[RD] = (u32)Read8(addr, NOUWII_FALSE);

#ifdef BROADWAY_DEBUG
    printf("PPC [%08X] lbz r%u, %d(r%u); r%u: %08X [%08X]\n", CIA, RD, SIMM, RA, RD, ctx.r[RD], addr);
#endif
}

static void LFD(const u32 instr) {
    u32 addr = SIMM;

    if (RA != 0) {
        addr += ctx.r[RA];
    }

    ctx.fprs[RD].PS0 = common_ToF64(Read64(addr, NOUWII_FALSE));

#ifdef BROADWAY_DEBUG
    printf("PPC [%08X] lfd f%u, %d(r%u); f%u: %lf [%08X]\n", CIA, RD, SIMM, RA, RD, ctx.fprs[RD].PS0, addr);
#endif
}

static void LFS(const u32 instr) {
    u32 addr = SIMM;

    if (RA != 0) {
        addr += ctx.r[RA];
    }

    const f32 data = common_ToF32(Read32(addr, NOUWII_FALSE));

    ctx.fprs[RD].PS0 = (f64)data;

    if (HID2.pse != 0) {
        ctx.fprs[RD].PS1 = (f64)data;
    }

#ifdef BROADWAY_DEBUG
    printf("PPC [%08X] lfs f%u, %d(r%u); f%u: %lf [%08X]\n", CIA, RD, SIMM, RA, RD, ctx.fprs[RD].PS0, addr);
#endif
}

static void LWZ(const u32 instr) {
    u32 addr = SIMM;

    if (RA != 0) {
        addr += ctx.r[RA];
    }

    ctx.r[RD] = Read32(addr, NOUWII_FALSE);

#ifdef BROADWAY_DEBUG
    printf("PPC [%08X] lwz r%u, %d(r%u); r%u: %08X [%08X]\n", CIA, RD, SIMM, RA, RD, ctx.r[RD], addr);
#endif
}

static void LWZX(const u32 instr) {
    u32 addr = ctx.r[RB];

    if (RA != 0) {
        addr += ctx.r[RA];
    }

    ctx.r[RD] = Read32(addr, NOUWII_FALSE);

#ifdef BROADWAY_DEBUG
    printf("PPC [%08X] lwzx r%u, r%u, r%u; r%u: %08X [%08X]\n", CIA, RD, RA, RB, RD, ctx.r[RD], addr);
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
    assert(!RC);

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

static void MULLI(const u32 instr) {
    ctx.r[RD] = ctx.r[RA] * SIMM;

#ifdef BROADWAY_DEBUG
    printf("PPC [%08X] mulli r%u, r%u, %X; r%u: %08X\n", CIA, RD, RA, SIMM, RD, ctx.r[RD]);
#endif
}

static void MULLW(const u32 instr) {
    ctx.r[RD] = ctx.r[RA] * ctx.r[RB];

    if (RC) {
        SetFlags(0, ctx.r[RD]);
    }

#ifdef BROADWAY_DEBUG
    printf("PPC [%08X] mullw%s r%u, r%u, r%u; r%u: %08X\n", CIA, (RC) ? "." : "", RD, RA, RB, RD, ctx.r[RD]);
#endif
}

static void NOR(const u32 instr) {
    ctx.r[RA] = ~(ctx.r[RS] | ctx.r[RB]);

    if (RC) {
        SetFlags(0, ctx.r[RA]);
    }

#ifdef BROADWAY_DEBUG
    printf("PPC [%08X] nor%s r%u, r%u, r%u; r%u: %08X\n", CIA, (RC) ? "." : "", RA, RS, RB, RA, ctx.r[RA]);
#endif
}

static void OR(const u32 instr) {
    ctx.r[RA] = ctx.r[RS] | ctx.r[RB];

    if (RC) {
        SetFlags(0, ctx.r[RA]);
    }

#ifdef BROADWAY_DEBUG
    printf("PPC [%08X] or%s r%u, r%u, r%u; r%u: %08X\n", CIA, (RC) ? "." : "", RA, RS, RB, RA, ctx.r[RA]);
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

static void PSMERGE01(const u32 instr) {
    assert(HID2.pse != 0);

    // TODO: Implement flags
    assert(!RC);

    ctx.fprs[RD].PS0 = ctx.fprs[RA].PS0;
    ctx.fprs[RD].PS1 = ctx.fprs[RB].PS1;

#ifdef BROADWAY_DEBUG
    printf("PPC [%08X] ps_merge01%s f%u, f%u, f%u; ps0: %lf, ps1: %lf\n", CIA, (RC) ? "." : "", RD, RA, RB, ctx.fprs[RD].PS0, ctx.fprs[RD].PS1);
#endif
}

static void PSMERGE10(const u32 instr) {
    assert(HID2.pse != 0);

    // TODO: Implement flags
    assert(!RC);

    ctx.fprs[RD].PS0 = ctx.fprs[RA].PS1;
    ctx.fprs[RD].PS1 = ctx.fprs[RB].PS0;

#ifdef BROADWAY_DEBUG
    printf("PPC [%08X] ps_merge10%s f%u, f%u, f%u; ps0: %lf, ps1: %lf\n", CIA, (RC) ? "." : "", RD, RA, RB, ctx.fprs[RD].PS0, ctx.fprs[RD].PS1);
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

static void PSQST(const u32 instr) {
    assert((HID2.pse != 0) && (HID2.lsqe != 0));

    u32 addr = (i32)(D << 20) >> 20;

    if (RA != 0) {
        addr += ctx.r[RA];
    }

    // TODO: Implement scaling
    assert(GQR[I].stscale == 0);

    switch (GQR[I].sttype) {
        case QUANT_TYPE_FLOAT:
            Write32(addr, common_FromF32((f32)ctx.fprs[RS].PS0));
            break;
        default:
            printf("Unimplemented psq_st quantization type %u\n", GQR[I].sttype);
            exit(1);
    }

    if (!W) {
        switch (GQR[I].sttype) {
            case QUANT_TYPE_FLOAT:
                Write32(addr + sizeof(f32), common_FromF32((f32)ctx.fprs[RS].PS1));
                break;
            default:
                printf("Unimplemented psq_st quantization type %u\n", GQR[I].sttype);
                exit(1);
        }
    }

#ifdef BROADWAY_DEBUG
    printf("PPC [%08X] psq_st f%u, %d(r%u), %d, %u;[%08X]: %lf, %lf\n", CIA, RS, (i32)(D << 20) >> 20, RA, W, I, addr, ctx.fprs[RS].PS0, ctx.fprs[RS].PS1);
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

static void STB(const u32 instr) {
    u32 addr = SIMM;

    if (RA != 0) {
        addr += ctx.r[RA];
    }

    Write8(addr, (u8)ctx.r[RS]);

#ifdef BROADWAY_DEBUG
    printf("PPC [%08X] stb r%u, %d(r%u); [%08X]: %02X\n", CIA, RS, SIMM, RA, addr, (u8)ctx.r[RS]);
#endif
}

static void STFS(const u32 instr) {
    u32 addr = SIMM;

    if (RA != 0) {
        addr += ctx.r[RA];
    }

    Write32(addr, common_FromF32((f32)ctx.fprs[RS].PS0));

#ifdef BROADWAY_DEBUG
    printf("PPC [%08X] stfs f%u, %d(r%u); [%08X]: %lf\n", CIA, RS, SIMM, RA, addr, ctx.fprs[RD].PS0);
#endif
}

static void STH(const u32 instr) {
    u32 addr = SIMM;

    if (RA != 0) {
        addr += ctx.r[RA];
    }

    Write16(addr, (u16)ctx.r[RS]);

#ifdef BROADWAY_DEBUG
    printf("PPC [%08X] sth r%u, %d(r%u); [%08X]: %04X\n", CIA, RS, SIMM, RA, addr, (u16)ctx.r[RS]);
#endif
}

static void STHX(const u32 instr) {
    u32 addr = ctx.r[RB];

    if (RA != 0) {
        addr += ctx.r[RA];
    }

    Write16(addr, (u16)ctx.r[RS]);

#ifdef BROADWAY_DEBUG
    printf("PPC [%08X] sthx r%u, r%u, r%u; [%08X]: %04X\n", CIA, RS, RA, RB, addr, (u16)ctx.r[RS]);
#endif
}

static void STW(const u32 instr) {
    u32 addr = SIMM;

    if (RA != 0) {
        addr += ctx.r[RA];
    }

    Write32(addr, ctx.r[RS]);

#ifdef BROADWAY_DEBUG
    printf("PPC [%08X] stw r%u, %d(r%u); [%08X]: %08X\n", CIA, RS, SIMM, RA, addr, ctx.r[RS]);
#endif
}

static void STWU(const u32 instr) {
    assert(RA != 0);

    const u32 addr = SIMM + ctx.r[RA];
    const u32 data = ctx.r[RS];

    Write32(addr, data);

    ctx.r[RA] = addr;

#ifdef BROADWAY_DEBUG
    printf("PPC [%08X] stwu r%u, %d(r%u); [%08X]: %08X\n", CIA, RS, SIMM, RA, addr, ctx.r[RS]);
#endif
}

static void STWX(const u32 instr) {
    u32 addr = ctx.r[RB];

    if (RA != 0) {
        addr += ctx.r[RA];
    }

    Write32(addr, ctx.r[RS]);

#ifdef BROADWAY_DEBUG
    printf("PPC [%08X] stwx r%u, r%u, r%u; [%08X]: %08X\n", CIA, RS, RA, RB, addr, ctx.r[RS]);
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
                case PAIREDSINGLE_PSMERGE01:
                    PSMERGE01(instr);
                    break;
                case PAIREDSINGLE_PSMERGE10:
                    PSMERGE10(instr);
                    break;
                default:
                    printf("Unimplemented Broadway Paired Single opcode %u (IA: %08X, instruction: %08X)\n", XO, CIA, instr);

                    exit(1);
            }
            break;
        case PRIMARY_MULLI:
            MULLI(instr);
            break;
        case PRIMARY_CMPLI:
            CMPLI(instr);
            break;
        case PRIMARY_CMPI:
            CMPI(instr);
            break;
        case PRIMARY_ADDICrc:
            ADDICrc(instr);
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
                case SYSTEM_CRXOR:
                    CRXOR(instr);
                    break;
                case SYSTEM_BCCTR:
                    BCCTR(instr);
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
                case SECONDARY_LWZX:
                    LWZX(instr);
                    break;
                case SECONDARY_CMPL:
                    CMPL(instr);
                    break;
                case SECONDARY_MFMSR:
                    MFMSR(instr);
                    break;
                case SECONDARY_NOR:
                    NOR(instr);
                    break;
                case SECONDARY_MTMSR:
                    MTMSR(instr);
                    break;
                case SECONDARY_STWX:
                    STWX(instr);
                    break;
                case SECONDARY_MTSR:
                    MTSR(instr);
                    break;
                case SECONDARY_MULLW:
                    MULLW(instr);
                    break;
                case SECONDARY_ADD:
                    ADD(instr);
                    break;
                case SECONDARY_MFSPR:
                    MFSPR(instr);
                    break;
                case SECONDARY_STHX:
                    STHX(instr);
                    break;
                case SECONDARY_OR:
                    OR(instr);
                    break;
                case SECONDARY_MTSPR:
                    MTSPR(instr);
                    break;
                case SECONDARY_DIVW:
                    DIVW(instr);
                    break;
                case SECONDARY_SYNC:
                    SYNC(instr);
                    break;
                case SECONDARY_EXTSB:
                    EXTSB(instr);
                    break;
                default:
                    printf("Unimplemented Broadway secondary opcode %u (IA: %08X, instruction: %08X)\n", XO, CIA, instr);

                    exit(1);
            }
            break;
        case PRIMARY_LWZ:
            LWZ(instr);
            break;
        case PRIMARY_LBZ:
            LBZ(instr);
            break;
        case PRIMARY_STW:
            STW(instr);
            break;
        case PRIMARY_STWU:
            STWU(instr);
            break;
        case PRIMARY_STB:
            STB(instr);
            break;
        case PRIMARY_STH:
            STH(instr);
            break;
        case PRIMARY_LFS:
            LFS(instr);
            break;
        case PRIMARY_LFD:
            LFD(instr);
            break;
        case PRIMARY_STFS:
            STFS(instr);
            break;
        case PRIMARY_PSQL:
            PSQL(instr);
            break;
        case PRIMARY_PSQST:
            PSQST(instr);
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

    // Clear init semaphore
    Write32(0x3160, 0);
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
