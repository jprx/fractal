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
    internal_repr &= ~PAGE_FLAG_READ_ONLY;
}

void FRACTALPTE_CLASS::SetReadOnly() {
    internal_repr |= PAGE_FLAG_READ_ONLY;
}

void FRACTALPTE_CLASS::SetSize(PageSize sz) {
    internal_size = sz;
}

void FRACTALPTE_CLASS::SetIntermediatePage() {
    internal_repr |= PAGE_FLAG_KEEP_WALKING;
}

void FRACTALPTE_CLASS::SetLeafPage(PagePrivilege p) {
    if (internal_size == SMALL_PAGE) {
        internal_repr |= PAGE_FLAG_KEEP_WALKING;
    } else {
        internal_repr &= ~PAGE_FLAG_KEEP_WALKING;
    }

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
    p = PTE_GET_PHYS(p); // Discard any non-aligned bits from p
    internal_repr &= ~STRIP_FLAGS_MASK;
    internal_repr |= p;
}

u64 FRACTALPTE_CLASS::Serialize() {
    return internal_repr;
}

bool FRACTALPTE_CLASS::IsLeaf() {
    return (internal_repr & PAGE_FLAG_KEEP_WALKING) == 0;
}

void FRACTALPTE_CLASS::SetWriteback() {
    internal_repr &= ~PAGE_MEMATTR_MASK;
    internal_repr |= PAGE_MEMATTR_NORMAL;
}

void FRACTALPTE_CLASS::SetUncacheable() {
    internal_repr &= ~PAGE_MEMATTR_MASK;
    internal_repr |= PAGE_MEMATTR_DEVICE;
}

void FRACTALPTE_CLASS::SetWritecomb() {
    // @TODO: Support this when we add support for hardware
    // framebuffers on ARM platforms (eg. iBoot)
    return;
}

PagePrivilege FRACTALPTE_CLASS::GetPrivilege() {
    return internal_repr & PAGE_FLAG_USER_ALLOWED ? USER_PAGE : KERNEL_PAGE;
}

void FRACTALPTE_CLASS::SetExtraBits(u8 extra) {
    if (extra & (~ARM_PTE_EXTRA_BITS_MASK)) panic("SetExtraBits: provided value is too large");
    internal_repr |= (((u64)extra) << ARM_PTE_EXTRA_BITS_SHIFT);
}

u8 FRACTALPTE_CLASS::GetExtraBits() {
    return (internal_repr >> ARM_PTE_EXTRA_BITS_SHIFT) & ARM_PTE_EXTRA_BITS_MASK;
}
