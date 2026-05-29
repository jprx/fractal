#include <cpu.h>
#include <arch.h>
#include <types.h>
#include <lib/mem.h>
#include <page.h>
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
    internal_repr |= PAGE_FLAG_READ_WRITE;
}

void FRACTALPTE_CLASS::SetReadOnly() {
    internal_repr &= ~PAGE_FLAG_READ_WRITE;
}

void FRACTALPTE_CLASS::SetSize(PageSize sz) {
    // Defer setting the size bit until we know if we are a leaf page or not
    // Until then, track the size in the internal_size field
    internal_size = sz;
}

void FRACTALPTE_CLASS::SetPrivilege(PagePrivilege p) {
    switch(p) {
        case KERNEL_PAGE:
        internal_repr &= ~PAGE_FLAG_USER_ALLOWED;
        break;

        case USER_PAGE:
        internal_repr |= PAGE_FLAG_USER_ALLOWED;
        break;
    }
}

void FRACTALPTE_CLASS::SetIntermediatePage() {
    internal_repr &= ~PAGE_FLAG_SIZE_LARGE;
    // Allow for leaf pages of this page to have user perms
    // Kernel pages will set the leaf page to be kernel only
    SetPrivilege(USER_PAGE);
}

void FRACTALPTE_CLASS::SetLeafPage(PagePrivilege p) {
    if (internal_size != SMALL_PAGE) {
        internal_repr |= PAGE_FLAG_SIZE_LARGE;
    }
    SetPrivilege(p);
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
    return (internal_repr & PAGE_FLAG_SIZE_LARGE) != 0;
}

void FRACTALPTE_CLASS::SetWriteback() {
    // Writeback is PAT0
    // We never touch PAT, so assume it is 0.
    // Need PCD = 0 and PWT = 0 as well
    internal_repr &= ~PAGE_FLAG_PCD;
    internal_repr &= ~PAGE_FLAG_PWT;
}

void FRACTALPTE_CLASS::SetUncacheable() {
    // Need PCD = 1
    internal_repr |= PAGE_FLAG_PCD;
    internal_repr |= PAGE_FLAG_PWT;
}

void FRACTALPTE_CLASS::SetWritecomb() {
    // Writecomb is PAT1
    // We never touch PAT, so assume it is 0.
    // Need PCD = 0 and PWT = 1 as well
    internal_repr &= ~PAGE_FLAG_PCD;
    internal_repr |= PAGE_FLAG_PWT;
}

PagePrivilege FRACTALPTE_CLASS::GetPrivilege() {
    return internal_repr & PAGE_FLAG_USER_ALLOWED ? USER_PAGE : KERNEL_PAGE;
}

void FRACTALPTE_CLASS::SetExtraBits(u8 extra) {
    if (extra & (~X86_PTE_EXTRA_BITS_MASK)) panic("SetExtraBits: provided value is too large");
    internal_repr |= (((u64)extra) << X86_PTE_EXTRA_BITS_SHIFT);
}

u8 FRACTALPTE_CLASS::GetExtraBits() {
    return (internal_repr >> X86_PTE_EXTRA_BITS_SHIFT) & X86_PTE_EXTRA_BITS_MASK;
}
