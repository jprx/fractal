#include <cpu.h>
#include <arch.h>
#include <types.h>
#include <lib/mem.h>
#include <page_alloc.h>
#include <virt_mem.h>
#include <out.h>

void FRACTALPTE_CLASS::From(u64 f) {
    Clear();
    internal_repr = f;
}

void FRACTALPTE_CLASS::Clear() {
    internal_size = SMALL_PAGE;
    internal_repr = PTE_DEFAULT;
}

void FRACTALPTE_CLASS::SetPresent(bool present) {
    if (present) {
        internal_repr |= PAGE_FLAG_PRESENT;
    }
    else {
        internal_repr &= ~PAGE_FLAG_PRESENT;
    }
}

void FRACTALPTE_CLASS::SetReadWrite() {
    panic("SetReadWrite not yet supported for RISC-V paging");
    // This is hard because permission bits reused for leaf page / large page
    // Need to track perms separately somehow in the future
}

void FRACTALPTE_CLASS::SetReadOnly() {
    panic("SetReadOnly not yet supported for RISC-V paging");
    // This is hard because permission bits reused for leaf page / large page
    // Need to track perms separately somehow in the future
}

void FRACTALPTE_CLASS::SetSize(PageSize sz) {
    internal_size = sz;
}

void FRACTALPTE_CLASS::SetIntermediatePage() {
    internal_repr &= ~PAGE_FLAG_READ;
    internal_repr &= ~PAGE_FLAG_WRITE;
    internal_repr &= ~PAGE_FLAG_EXEC;
    internal_repr &= ~PAGE_FLAG_ACCESSED;
    internal_repr &= ~PAGE_FLAG_DIRTY;
    internal_repr &= ~PAGE_FLAG_USER_ALLOWED;
}

void FRACTALPTE_CLASS::SetLeafPage(PagePrivilege p) {
    // Default to RWX for leaf pages
    // There is no "leaf page" bit on RV- just permission bits, if they
    // are nonzero then its a leaf page
    internal_repr |= (
                    PAGE_FLAG_READ   |
                    PAGE_FLAG_WRITE    |
                    PAGE_FLAG_EXEC     |
                    PAGE_FLAG_ACCESSED |
                    PAGE_FLAG_DIRTY
    );

    SetPrivilege(p);
}

void FRACTALPTE_CLASS::SetPrivilege(PagePrivilege p) {
    switch(p) {
        case USER_PAGE:
        internal_repr |= PAGE_FLAG_USER_ALLOWED;
        break;

        case KERNEL_PAGE:
        internal_repr &= ~PAGE_FLAG_USER_ALLOWED;
        break;
    }
}

bool FRACTALPTE_CLASS::IsPresent() {
    return (internal_repr & PAGE_FLAG_PRESENT) != 0;
}

phys_t FRACTALPTE_CLASS::GetPhysAddr() {
    return PTE_GET_PHYS(internal_repr);
}

void FRACTALPTE_CLASS::SetPhysAddr(phys_t p) {
    u64 shifted = p >> PHYS_TO_PTE_SHIFT;
    p = PTE_GET_PHYS(p);
    internal_repr &= ~STRIP_FLAGS_MASK;
    internal_repr |= shifted;
}

u64 FRACTALPTE_CLASS::Serialize() {
    return internal_repr;
}

bool FRACTALPTE_CLASS::IsLeaf() {
    return (internal_repr & PAGE_FLAG_ACCESSED) != 0;
}

void FRACTALPTE_CLASS::SetWriteback() {
    // Undefined for this architecture (for now)
    return;
}

void FRACTALPTE_CLASS::SetUncacheable() {
    // Undefined for this architecture (for now)
    return;
}

void FRACTALPTE_CLASS::SetWritecomb() {
    // Undefined for this architecture (for now)
    return;
}

PagePrivilege FRACTALPTE_CLASS::GetPrivilege() {
    return internal_repr & PAGE_FLAG_USER_ALLOWED ? USER_PAGE : KERNEL_PAGE;
}

void FRACTALPTE_CLASS::SetExtraBits(u8 extra) {
    if (extra & (~RISCV_PTE_EXTRA_BITS_MASK)) panic("SetExtraBits: provided value is too large");
    internal_repr |= (((u64)extra) << RISCV_PTE_EXTRA_BITS_SHIFT);
}

u8 FRACTALPTE_CLASS::GetExtraBits() {
    return (internal_repr >> RISCV_PTE_EXTRA_BITS_SHIFT) & RISCV_PTE_EXTRA_BITS_MASK;
}
