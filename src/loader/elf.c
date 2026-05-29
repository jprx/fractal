#include <fractal.h>
#include <filesys/fileio.h>
#include <loader/elf.h>
#include <lib/dlmalloc.h>
#include <page_alloc.h>
#include <paging.h>
#include <virt_mem.h>
#include <regs.h>
#include <copy_task.h>
#include <syscall.h>
#include <fthread.h>

// #define ELF_DEBUG_LOADING

kret_t load_elf_seg_ptload (void *file_handle, ElfHeader *elf, task_t *target_task, ElfSegment *pt) {
    if (!is_user_addr(pt->vaddr)) {
        panic("ELF tried to load itself into the kernel!");
        return OH_NO;
    }

    // Number of pages is dependent on the memory-mapped size as well as
    // how many bytes into the first page our vaddr already is
    usize n_pages = ceil_div(pt->memsz + (pt->vaddr % LARGE_PAGE_SIZE), LARGE_PAGE_SIZE);
    for (usize i = 0; i < n_pages; i++) {

        if (!IsPagePresent(target_task->t_pagetable, pt->vaddr + (LARGE_PAGE_SIZE * i))) {

#ifdef ELF_DEBUG_LOADING
        printf("Allocating page %d of %d (%d)\n", i+1, n_pages, pt->memsz);
#endif // ELF_DEBUG_LOADING

            virt_t new_page_virt = AllocPage(LARGE_PAGE);
            phys_t new_page_phys = KERN_V2P(new_page_virt);

            virt_t new_va_in_task = ALIGN_LARGE_PAGE(pt->vaddr) + (i * LARGE_PAGE_SIZE);

            // Map each page twice: once for user (unprivileged) threads,
            // and again for outer kernel (privileged) threads.
            PageMap(
                target_task->t_pagetable,
                FTHREAD_USER_MAP | new_va_in_task,
                new_page_phys,
                LARGE_PAGE,
                USER_PAGE
            );
            SetPageExtraBits(
                target_task->t_pagetable,
                FTHREAD_USER_MAP | new_va_in_task,
                ALLOCATED_PAGE
            );

            PageMap(
                target_task->t_pagetable,
                FTHREAD_KERN_MAP | new_va_in_task,
                new_page_phys,
                LARGE_PAGE,
                KERNEL_PAGE
            );
            SetPageExtraBits(
                target_task->t_pagetable,
                FTHREAD_KERN_MAP | new_va_in_task,
                DUPLICATED_PAGE
            );

        } else {
#ifdef ELF_DEBUG_LOADING
        printf("Skipping page %d of %d (%d)\n", i+1, n_pages, pt->memsz);
#endif // ELF_DEBUG_LOADING
        }
    }

#ifdef ELF_DEBUG_LOADING
        printf("Copying in %d bytes at 0x%X\n", pt->filesz, pt->vaddr);
#endif // ELF_DEBUG_LOADING

    copy_file_to_task(
        target_task,
        pt->vaddr,
        file_handle,
        pt->offset,
        pt->filesz
    );

    // ELF spec asks us to zero-init any PT_LOAD segments
    // where memsz > filesz
    if (pt->filesz < pt->memsz) {
        // Zero-init rest of bss section
        memset_to_task(
            target_task,
            pt->vaddr + pt->filesz,
            '\x00',
            pt->memsz - pt->filesz
        );
    }

    return ALL_GOOD;
}

/*
 * load_elf
 * Loads an ELF file off the filesystem into the task target_task.
 *
 * Arguments:
 *  - file_handle:   An opaque filesystem resource that will
 *                   work with filesys_read. Assumed valid,
 *                   but we will confirm it is in fact an elf_header.
 *  - target_task:   The task into which to load this elf_header.
 *
 * Must be called with interrupts disabled!
 *
 * For now, any issue with ELF loading panics the kernel,
 * but in the future these will simply return an error code.
 * That's why every panic is followed by return OH_NO.
 *
 * Return Value: ALL_GOOD if no problems, otherwise
 * one of the error codes.
 */
kret_t load_elf(void *file_handle, task_t *target_task) {
    ElfHeader elf_header;
    ElfSegment program_header;
    usize elf_file_len = 0;

    if (!file_handle) {
        panic("no elf");
        return THING_DOESNT_EXIST;
    }

    if (!target_task) {
        panic("no task");
        return THING_DOESNT_EXIST;
    }

    elf_file_len = filesys_filelen(file_handle);

    // Zero init elf header, just in case the file we read is really small
    // and uninitialized data in the page tricks the loader into thinking
    // we loaded an ELF (when really we loaded nothing / garbage)
    memset(&elf_header, '\x00', sizeof(elf_header));

    filesys_read(file_handle, (u8 *)&elf_header, sizeof(elf_header), 0);

    if (*(u32 *)(&elf_header.ident) != ELF_MAGIC) {
#ifdef ELF_DEBUG_LOADING
        panic("Not an ELF (0x%X)", *((u64 *)&elf_header.ident));
#endif // ELF_DEBUG_LOADING

        return OH_NO;
    }

    if (elf_header.ident[ELF_IDENT_CLASS] != ELFCLASS64) {
#ifdef ELF_DEBUG_LOADING
        panic("not a 64-bit elf");
#endif // ELF_DEBUG_LOADING

        return OH_NO;
    }

    if (elf_header.ident[ELF_IDENT_DATA] != ELFDATA2LSB) {
#ifdef ELF_DEBUG_LOADING
        panic("not a little-endian ELF");
#endif // ELF_DEBUG_LOADING

        return OH_NO;
    }

    if (elf_header.ident[ELF_IDENT_OSABI] != ELFOSABI_SYSV) {
#ifdef ELF_DEBUG_LOADING
        panic("not a sysv ELF");
#endif // ELF_DEBUG_LOADING

        return OH_NO;
    }

    if (elf_header.type != ELF_ET_EXEC) {
        panic("not an exec elf");
        return OH_NO;
    }

    // Check whether ELF matches this system
    switch(elf_header.machine) {

#ifdef ARCH_X86
        case ELF_MACHINE_X86_64:
#ifdef ELF_DEBUG_LOADING
        printf("Detected x86 elf\n");
#endif // ELF_DEBUG_LOADING
        break;
#endif // ARCH_X86

#ifdef ARCH_ARM
        case ELF_MACHINE_AARCH64:
#ifdef ELF_DEBUG_LOADING
        printf("Detected arm elf\n");
#endif // ELF_DEBUG_LOADING
        break;
#endif // ARCH_ARM

#ifdef ARCH_RISCV
        case ELF_MACHINE_RISCV:
#ifdef ELF_DEBUG_LOADING
        printf("Detected riscv elf\n");
#endif // ELF_DEBUG_LOADING
        break;
#endif // ARCH_RISCV

        default:
#ifdef ELF_DEBUG_LOADING
        panic("ELF architecture does not match this system (%d)", elf_header.machine);
#endif // ELF_DEBUG_LOADING

        return OH_NO;
        break;
    }

#ifdef ELF_DEBUG_LOADING
    printf("Entrypoint is 0x%X\r\n", elf_header.entrypoint);
#endif // ELF_DEBUG_LOADING

    if (sizeof(ElfSegment) != elf_header.ph_entsize) {
        panic("Program header size for Fractal differs from this ELF's program header size");
        return OH_NO;
    }

#ifdef ELF_DEBUG_LOADING
    printf("program header at 0x%X\r\n", program_header);
#endif // ELF_DEBUG_LOADING

    for (usize i = 0; i < elf_header.ph_num; i++) {
        filesys_read(file_handle, (u8*)&program_header, sizeof(program_header), elf_header.ph_offset + (sizeof(program_header) * i));

#ifdef ELF_DEBUG_LOADING
        printf("Segment %d\n", i);
        printf("\ttype: %d\r\n", program_header.type);
        printf("\tvaddr: 0x%X\r\n", program_header.vaddr);
#endif // ELF_DEBUG_LOADING

        switch(program_header.type) {
            case ELF_PT_NULL:
            case ELF_PT_NOTE:
            continue;
            break;

            case ELF_PT_LOAD:
            load_elf_seg_ptload(file_handle, &elf_header, target_task, &program_header);
            break;

#ifndef QUIET_BOOT
            default:
            printf("Unsupported program header (idx=%d, type=0x%X)\r\n", i, program_header.type);
            break;
#endif // !QUIET_BOOT
        }
    }

    set_pc(&target_task->t_threads[INIT_THREAD]->t_init_regs, elf_header.entrypoint);
    return ALL_GOOD;
}
