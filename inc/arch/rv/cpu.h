#ifndef FRACTAL_RV_CPU_H
#define FRACTAL_RV_CPU_H

#include <cpu.h>
#include <interrupt.h>
#include <io/interrupt/plic.h>

#if !ARCH_RISCV
#error "FractalCoreRV is being included, but this isn't a RISCV build!"
#endif // !ARCH_RISCV

class FractalCoreRV : public FractalCore {
public:
    FractalPLIC plic;
    FractalCoreRV() : plic(PLIC_DEFAULT_BASEADDR) {}
    virtual InterruptSource *GetInterruptController() override;
    virtual kret_t InitializeInterrupts() override;
    virtual kret_t SetPageTable(u64 *new_pt, u64 asid) override;
    virtual u64 *GetPageTable(virt_t vaddr) override;
    virtual void FlushTLB() override;
    virtual void FlushICache() override;
    virtual kret_t EnableInterrupt(InterruptKind i) override;
    virtual InterruptMode SetInterrupts(InterruptMode i) override;
    virtual kret_t InitializeSyscalls() override;
    virtual usize GetPCIMMCFG() override;
    virtual void *FindInitrd() override;
};

class PTERV : AbstractPTE {
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

#define NUM_PHYS_ADDR_BITS_IN_PTE ((43ULL))
#define PHYS_ADDR_FIRST_BIT_IN_PTE ((10ULL))

#define ADDR_TO_PAGENUM(x) (( ((x >> PHYS_ADDR_FIRST_BIT_IN_PTE)) & ((1LL << NUM_PHYS_ADDR_BITS_IN_PTE) - 1) ))
#define PAGENUM_TO_ADDR(x) (( (x << PHYS_ADDR_FIRST_BIT_IN_PTE) ))

#define PHYS_TO_PTE_SHIFT ((2ull))
#define PTE_GET_PHYS(x) (( PAGENUM_TO_ADDR(ADDR_TO_PAGENUM(x)) << PHYS_TO_PTE_SHIFT))

#define STRIP_FLAGS_MASK (( ((1LL << NUM_PHYS_ADDR_BITS_IN_PTE) - 1) << PHYS_ADDR_FIRST_BIT_IN_PTE ))

#define PAGE_FLAG_PRESENT        ((1ull << 0ull))
#define PAGE_FLAG_READ           ((1ull << 1ull))
#define PAGE_FLAG_WRITE          ((1ull << 2ull))
#define PAGE_FLAG_EXEC           ((1ull << 3ull))
#define PAGE_FLAG_USER_ALLOWED   ((1ull << 4ull))
#define PAGE_FLAG_GLOBAL         ((1ull << 5ull))
#define PAGE_FLAG_ACCESSED       ((1ull << 6ull))
#define PAGE_FLAG_DIRTY          ((1ull << 7ull))

#define PTE_DEFAULT ((PAGE_FLAG_PRESENT | PAGE_FLAG_READ | PAGE_FLAG_WRITE | PAGE_FLAG_EXEC | PAGE_FLAG_ACCESSED | PAGE_FLAG_DIRTY))
#define PTE_UNMAPPED ((0))

#define RISCV_PTE_EXTRA_BITS_MASK MASK(2)
#define RISCV_PTE_EXTRA_BITS_SHIFT ((8ull))

#endif // FRACTAL_RV_CPU_H
