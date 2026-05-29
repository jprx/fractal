#ifndef TSS_H
#define TSS_H

#include <fractal.h>

BEGIN_C_HEADER

typedef struct __attribute__((packed)) {
    u32 _reserved0;
    u64 rsp0;
    u64 rsp1;
    u64 rsp2;
    u64 _reserved1;
    u64 ist1;
    u64 ist2;
    u64 ist3;
    u64 ist4;
    u64 ist5;
    u64 ist6;
    u64 ist7;
    u64 _reserved2;
    u16 _reserved3;
    u16 iomap_base_addr;
} TSS;

extern TSS tss;

void InitTSS();

typedef u8 TSSEntryOptions;

#define TSS_OPTION_TYPE_MASK ((0x0FF))
#define TSS_OPTION_TYPE_SHIFT 0
#define TSS_OPTION_TYPE_64BIT_TSS  ((0x9 << TSS_OPTION_TYPE_SHIFT))

#define TSS_OPTION_PRESENT_MASK ((0x80))
#define TSS_OPTION_PRESENT ((1 << 7ull))

#define TSS_OPTION_DPL_MASK ((0x60))
#define TSS_OPTION_DPL_SHIFT ((5))

#define TSS_OPTION_DEFAULT ((TSS_OPTION_PRESENT | TSS_OPTION_TYPE_64BIT_TSS | ((DPL_KERNEL << TSS_OPTION_DPL_SHIFT))))

typedef struct __attribute__((packed)) TSSDescriptor_t {
public:
	u16 limit_low;
	u16 addr_low;
    u8 addr_mid1;
	TSSEntryOptions options;
	u8 limit_high;
	u8 addr_mid2;
	u32 addr_high;
    u32 res0;

    static TSSDescriptor_t New();
    void set_addr (u64 new_addr);
    void set_limit (u64 new_limit);
} TSSDescriptor;

END_C_HEADER

#endif // TSS_H
