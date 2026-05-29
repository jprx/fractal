#ifndef FRACTAL_ARM_CPU_H
#define FRACTAL_ARM_CPU_H

#include <cpu.h>
#include <interrupt.h>
#include <io/interrupt_source.h>

#ifdef ARM_BOARD_HAS_GIC
#include <io/interrupt/gic.h>
#endif // #ifdef ARM_BOARD_HAS_GIC

#if !ARCH_ARM
#error "FractalCoreARM is being included, but this isn't an ARM build!"
#endif // !ARCH_ARM

class FractalCoreARM : public FractalCore {
public:
    u64 *ttbr1_table;
    bool initialized_ttbr1;
#ifdef ARM_BOARD_HAS_GIC
    FractalGIC gic;
    FractalCoreARM() : gic(GICD_DEFAULT_BASE, GICC_DEFAULT_BASE) {}
#else // ARM_BOARD_HAS_GIC
    FractalCoreARM() {}
#endif // !ARM_BOARD_HAS_GIC
    virtual InterruptSource *GetInterruptController() override;
    virtual void InitializePaging() override;
    virtual kret_t InitializeInterrupts() override;
    virtual kret_t SetPageTable(u64 *new_pt, u64 asid) override;
    virtual u64 *GetPageTable(virt_t vaddr) override;
    virtual void FlushTLB() override;
    virtual void FlushICache() override;
    virtual void MapKernel(u64 *pt) override;
    virtual void UnmapKernel(u64 *pt) override;
    virtual kret_t EnableInterrupt(InterruptKind i) override;
    virtual InterruptMode SetInterrupts(InterruptMode i) override;
    virtual kret_t InitializeSyscalls() override;
    virtual usize GetPCIMMCFG() override;
    virtual void *FindInitrd() override;
};

class PTEARM : AbstractPTE {
public:
    u64 internal_repr, internal_size;
    virtual void From(u64) override;
    virtual void Clear() override;
    virtual void SetPresent(bool present) override;
    virtual void SetReadWrite() override;
    virtual void SetReadOnly() override;
    virtual void SetSize(PageSize sz) override;
    virtual void SetIntermediatePage() override;
    virtual void SetLeafPage(PagePrivilege) override;
    virtual void SetPrivilege(PagePrivilege) override;
    virtual PagePrivilege GetPrivilege() override;
    virtual void SetExtraBits(u8) override;
    virtual u8 GetExtraBits() override;
    virtual void SetWriteback() override;
    virtual void SetUncacheable() override;
    virtual void SetWritecomb() override;
    virtual bool IsPresent() override;
    virtual bool IsLeaf() override;
    virtual u64 GetPhysAddr() override;
    virtual void SetPhysAddr(u64) override;
    virtual u64 Serialize() override;
};

#define NUM_PHYS_ADDR_BITS ((40ULL))
#define PHYS_ADDR_FIRST_BIT ((12ULL))

#define ADDR_TO_PAGENUM(x) (( ((x >> PHYS_ADDR_FIRST_BIT)) & ((1LL << NUM_PHYS_ADDR_BITS) - 1) ))
#define PAGENUM_TO_ADDR(x) (( (x << PHYS_ADDR_FIRST_BIT) ))

#define STRIP_FLAGS_MASK (( ((1LL << NUM_PHYS_ADDR_BITS) - 1) << PHYS_ADDR_FIRST_BIT ))

#define PTE_GET_PHYS(x) (( PAGENUM_TO_ADDR(ADDR_TO_PAGENUM(x)) ))

#define AP_KERNEL ((0b00))
#define AP_USER ((0b01))

#define PAGE_FLAG_PRESENT        ((1ull << 0ull))
#define PAGE_FLAG_KEEP_WALKING   ((1ull << 1ull))
#define PAGE_FLAG_READ_ONLY      ((1ull << 7ull))
#define PAGE_FLAG_USER_ALLOWED   ((1ull << 6ull))
#define PAGE_FLAG_AF             ((1ull << 10ull))

#define PTE_DEFAULT ((PAGE_FLAG_PRESENT | PAGE_FLAG_AF))
#define PTE_UNMAPPED ((0))

#define PAGE_MEMATTR_NORMAL ((0b000 << 2ull))
#define PAGE_MEMATTR_DEVICE ((0b001 << 2ull))
#define PAGE_MEMATTR_MASK   ((0b00011100))

#define ARM_PTE_EXTRA_BITS_MASK MASK(4)
#define ARM_PTE_EXTRA_BITS_SHIFT ((55ull))

#endif // FRACTAL_ARM_CPU_H
