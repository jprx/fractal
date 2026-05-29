#ifndef CPU_H
#define CPU_H

#include <types.h>
#include <interrupt.h>
#include <io/interrupt_source.h>
#include <page.h>
#include <task.h>
#include <board.h>

// The FractalCore class is a single common object that implements
// abstractions around everything a Fractal-compliant CPU
// needs to support.

// Different architectures should define the FRACTALCORE_CLASS macro
// in their arch.h file to the name of some class that inherits from
// FractalCore and implements the required features.

class FractalCore {
public:
    /*
     * TimerInterrupt
     * EOI should be sent *before* calling this method, as it may deschedule
     * this task.
     *
     * This may cause the current task to be rescheduled.
     */
    void TimerInterrupt();

    /*
     * MapKernel
     * Maps the kernel into the provided page table.
     *
     * If we are on a split-page table arch (eg. ARM),
     * this silently makes sure that the kernel page table
     * (stored internally within this FractalCore) is up to date.
     *
     * This method must *always* put the kernel in the same place
     * to avoid disagreements between whether or not the page table
     * provided is fungible from the perspective of locating the kernel.
     *
     * IE two page tables that call MapKernel should both always
     * put agree on where the kernel is when SetPageTable is called later.
     */
    virtual void MapKernel(u64 *pt);

    /*
     * UnmapKernel
     * Unmaps the kernel from the provided page table.
     *
     * If we are on a split-page table arch (eg. ARM),
     * this doesn't do anything.
     *
     * This method is called by FreeUserPageTable before
     * recursively freeing every physical page (not just the
     * pages making up the page table itself).
     *
     * We need to ensure the kernel is removed from the
     * page table before doing this.
     */
    virtual void UnmapKernel(u64 *pt);

    /*
     * GetPageTable
     * Given a virtual address, returns the currently loaded
     * page table that will be used to map that address (if
     * it were to be mapped in).
     *
     * On X86 and RISCV, this just returns the current pagetable,
     * as kernel pages and user pages use the same table.
     *
     * On ARM, this returns either TTBR0 or TTBR1, depending
     * on whether vaddr is a user or kernel address.
     */
    virtual u64 *GetPageTable(virt_t vaddr) = 0;

    /*
     * FlushTLB
     * Flush the TLB by reloading the page table.
     */
    virtual void FlushTLB() = 0;

    /*
     * FlushICache
     * Flush the entire CPU instruction cache.
     *
     * Call this after writing to memory that
     * could be executed later.
     */
    virtual void FlushICache() = 0;

    /*
     * InitializePaging
     * Sets up virtual memory from the C world (discarding
     * simple bringup page tables) and other various CPU
     * features early on. On most arch's this does nothing,
     * but on ARM it is used to setup TTBR1.
     */
    virtual void InitializePaging();

    /*
     * InitializeInterrupts
     * Call this after enabling paging (InitializePaging -> SetPageTable)
     * and virtual memory is enabled. This configures the interrupt tables.
     */
    virtual kret_t InitializeInterrupts() = 0;

    /*
    * SetPageTable
    * Change the page table pointer to a new page table.
    *
    * Assumption: The kernel is currently running one-to-one
    * mapped where we are in the higher half, but simply
    * stripping off the upper bits of the pointer will return
    * the physical address of the underlying memory.
    *
    * That is, the KERN_V2P macro does *not* perform a page
    * table walk currently, it only discards the upper pointer bits.
    */
    virtual kret_t SetPageTable(u64 *new_pt, u64 asid) = 0;

    /*
     * GetInterruptController
     * Returns a pointer to this core's InterruptSource object
     * that can be used for interfacing with interrupts.
     */
    virtual InterruptSource *GetInterruptController() = 0;

    /*
     * EnableInterrupt
     * Enables a specific interrupt.
     */
    virtual kret_t EnableInterrupt(InterruptKind i) = 0;

    /*
     * SetInterrupts
     * Change the interrupt mode (enabled / disabled),
     * returning the previous mode.
     */
    virtual InterruptMode SetInterrupts(InterruptMode i) = 0;

    /*
     * InitializeSyscalls
     * Sets up the system call handlers.
     */
    virtual kret_t InitializeSyscalls() = 0;

    /*
     * GetPCIMMCFG
     * Return the base address of the PCI Extended Configuration Space.
     * Discovered via ACPI / device tree, or hard-coded for
     * boards/ systems without a device tree / ACPI.
     */
    virtual usize GetPCIMMCFG() = 0;

    /*
     * FindInitrd
     * Locate the initial ramdisk (for now we assume a FAT32 archive)
     * Returns a pointer to the initrd in memory.
     */
    virtual void *FindInitrd() = 0;

    /*
     * DetectHardwareFramebuffer
     * If there is a RAM framebuffer detected for this platform,
     * setup the global_gpu to point to an instance of a FractalGPU
     * that supports driving it.
     *
     * Examples: VESA VBE framebuffer set by Multiboot-compliant
     *           bootloader on X86_64 (eg. GRUB), or iBoot's
     *           framebuffer on Apple Silicon.
     *
     * Otherwise, don't do anything.
     *
     * If the global_gpu remains NULL after this method is called,
     * we will scan PCI for any VirtioGPU cards and use that instead.
     */
    virtual kret_t DetectHardwareFramebuffer();

    /*
     * DetectPlatformIO
     * Detect any platform-specific HID devices.
     * For now, this is just for PS/2 controller devices on X86_64.
     *
     * In the future, this could be expanded to interact with our USB
     * stack for detecting USB controllers, disabling USB Legacy Support,
     * etc.
     */
    virtual kret_t DetectPlatformIO();

    /*
     * InitializeCaching
     * Mark main memory as write-back cacheable,
     * data memory as uncacheable, and the framebuffer
     * (if it exists) as write-combining.
     *
     * What this does is very platform-specific. A default
     * implementation is provided in gencpu that does nothing,
     * so hopefully if your platform's default behavior is uncacheable
     * then nothing will break.
     */
    virtual void InitializeCaching();
};

/*
 * An abstract page table entry
 * Each architecture needs to implement the virtual methods here,
 * and export the FRACTALPTE_CLASS macro (in arch/$(ARCH).h) to
 * the derived class that fulfills the pager requirements.
 *
 * Page table traversal methods and platform-independent
 * paging routines will call into the AbstractPTE methods,
 * based on whatever the FRACTALPTE_CLASS is.
 */
class AbstractPTE {
public:
    // Construct a new PTE
    virtual void From(u64) = 0;
    virtual void Clear() = 0;
    virtual void SetPresent(bool present) = 0;

    virtual void SetReadWrite() = 0;
    virtual void SetReadOnly() = 0;

    virtual void SetSize(PageSize sz) = 0;

    // Call this to tell this PTE it is an intermediate
    // page in the middle of the hierarchy pointing to
    // not a huge page but another table:
    // We also pass along the privilege level- some architectures
    // need the privilege level set at all parts of the page table,
    // whereas others need it just at the leaf.
    virtual void SetIntermediatePage() = 0;
    virtual void SetLeafPage(PagePrivilege) = 0;
    virtual void SetPrivilege(PagePrivilege) = 0;
    virtual PagePrivilege GetPrivilege() = 0;

    // Get/ Set OS-Specific data from this PTE
    virtual void SetExtraBits(u8) = 0;
    virtual u8 GetExtraBits() = 0;

    // Inform this page table entry it should refer to either normal or device memory
    // You should only ever call these for leaf PTEs!
    virtual void SetWriteback() = 0;
    virtual void SetUncacheable() = 0;
    virtual void SetWritecomb() = 0;

    virtual bool IsPresent() = 0;
    virtual u64 GetPhysAddr() = 0;
    virtual void SetPhysAddr(u64) = 0;

    // Return true if this intermediate page is a leaf,
    // false otherwise. Only defined for intermediate pages,
    // last level pages are always leaves. This method is not
    // defined for last level pages and should not be called on them.
    virtual bool IsLeaf() = 0;

    /*
     * Serialize
     * Gets a u64 representation of this abstract page table entry,
     * appropriate for the particular target architecture.
     */
    virtual u64 Serialize() = 0;
};

/*
 * DECLARE_CORE
 * Creates a new FractalCore subclass instance
 *
 * Inputs: `name` is the name of the new object
 * being created at the call site.
 *
 * We expect FRACTALCORE_CLASS to have been
 * defined in the processor family's arch.h file
 * (at inc/arch/$(ARCH).h)
 *
 * The idea is for clients of the FractalCore API to
 * be able to simply declare a new FractalCore object
 * anywhere they'd like without having to worry about
 * what the actual derived class is, They just use
 * DECLARE_CORE and cast it to a FractalCore pointer.
 *
 * Example:
 * DECLARE_CORE(my_cpu_wrapper);
 * FractalCore *core = (FractalCore *)&my_cpu_wrapper;
 * core->Map(some_virt, some_phys, some_size);
 */
#define DECLARE_CORE(name) FRACTALCORE_CLASS name;

#endif // CPU_H
