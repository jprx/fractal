#ifndef FRACTAL_X86_CPU_H
#define FRACTAL_X86_CPU_H

#include <cpu.h>
#include <types.h>
#include <task.h>

#if !ARCH_X86
#error "FractalCoreX86 is being included, but this isn't an X86_64 build!"
#endif // !ARCH_X86

class FractalCoreX86 : public FractalCore {
public:
    FractalAPIC apic;
    u8 per_cpu_data_area[64];
    usize pci_mmcfg_base;
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
    virtual kret_t DetectHardwareFramebuffer() override;
    virtual kret_t DetectPlatformIO() override;
    virtual void InitializeCaching() override;
};

class PTEX86 : AbstractPTE {
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
    void SetPrivilegeHelper(PagePrivilege);
};

#define NUM_PHYS_ADDR_BITS ((40ULL))
#define PHYS_ADDR_FIRST_BIT ((12ULL))

#define ADDR_TO_PAGENUM(x) (( ((x >> PHYS_ADDR_FIRST_BIT)) & ((1LL << NUM_PHYS_ADDR_BITS) - 1) ))
#define PAGENUM_TO_ADDR(x) (( (x << PHYS_ADDR_FIRST_BIT) ))

#define STRIP_FLAGS_MASK (( ((1LL << NUM_PHYS_ADDR_BITS) - 1) << PHYS_ADDR_FIRST_BIT ))
#define PTE_GET_PHYS(x) (( PAGENUM_TO_ADDR(ADDR_TO_PAGENUM(x)) ))

#define PAGE_FLAG_PRESENT        ((1ull << 0ull))
#define PAGE_FLAG_READ_WRITE     ((1ull << 1ull))
#define PAGE_FLAG_USER_ALLOWED   ((1ull << 2ull))
#define PAGE_FLAG_PWT            ((1ull << 3ull))
#define PAGE_FLAG_PCD            ((1ull << 4ull))
#define PAGE_FLAG_SIZE_LARGE     ((1ull << 7ull))
#define PAGE_FLAG_NO_EXEC        ((1ull << 63ull))

#define PTE_DEFAULT ((PAGE_FLAG_PRESENT | PAGE_FLAG_READ_WRITE))
#define PTE_UNMAPPED ((0))

#define X86_PTE_EXTRA_BITS_MASK MASK(3)
#define X86_PTE_EXTRA_BITS_SHIFT ((9ull))

#endif // FRACTAL_X86_CPU_H
