#include <out.h>
#include <cpu.h>
#include <arch.h>
#include <types.h>
#include <oxide.h>
#include <fractal.h>
#include <io/serial.h>
#include <version.h>
#include <io/mouse.h>
#include <lib/dlmalloc.h>
#include <lib/mem.h>
#include <lib/cxx_supp.h>
#include <stdarg.h>
#include <page_alloc.h>
#include <virt_mem.h>
#include <page.h>
#include <interrupt.h>
#include <task.h>
#include <syscall.h>
#include <io/pci.h>
#include <io/virtio.h>
#include <io/gpu/virtio-gpu.h>
#include <io/hid/virtio-hid.h>
#include <filesys/tar.h>
#include <oxide/compositor.h>
#include <filesys/fileio.h>
#include <oxide/catalog.h>
#include <stdio.h>
#include <paging.h>
#include <loader/elf.h>

// All static global objects
DECLARE_CORE(global_cpu);
DECLARE_SERIAL(global_serial_device);
FractalGPU *global_gpu = NULL;
VirtioHID *global_keyboard = NULL;
VirtioHID *global_mouse = NULL;
VirtioHID *global_tablet = NULL;
Compositor global_compositor;
virt_t global_initrd = (virt_t)NULL;

extern "C" void __global_out_putc(char c) {
    if ('\n' == c) global_serial_device.SendChar('\r');
    global_serial_device.SendChar(c);
}

extern "C" void __global_out_puts(char *s) {
    global_serial_device.WriteString(s);
}

extern "C" void __global_flush_tlb() {
    global_cpu.FlushTLB();
}

// These two methods are used during panics, but panic is a C
// file so it can't directly call C++ methods
extern "C" void __disable_global_interrupts() {
    global_cpu.SetInterrupts(INTS_DISABLED);
}

extern "C" void __refresh_gpu() {
    global_gpu->Repaint();
}

extern "C" void dump_page_allocator_info();
extern usize sbrk_heap_base, sbrk_heap_len;

extern u8 _last_kernel_address;

/*
 * DumpCoreInfo
 * Prints bringup debug info to serial.
 */
extern "C" void DumpCoreInfo() {
    printf("#### FRACTAL STARTED\n");
#ifndef QUIET_BOOT
    printf("%s\n", logo);
#endif // ! QUIET_BOOT
    printf("CPU: %s\n", ARCH_STRING);
    printf("Board: %s\n", BOARD_NAME);
    printf("Variant: %s\r\n", KERNEL_VARIANT);
    printf("Kernel: %s\r\n", KERNEL_NAME);
    printf("Version: %s\r\n", fractal_version_str);
#ifdef CONFIG_USE_ASID
    printf("ASIDs V2: ENABLED\n");
#else
    printf("ASIDs V2: DISABLED\n");
#endif
#ifndef QUIET_BOOT
    printf("       Graphics: %s\r\n", WINDOW_MANAGER_NAME);
    printf("            CPU: %s\r\n", ARCH_STRING);
    printf("          Board: %s\r\n", BOARD_NAME);
    printf("          Built: %s\r\n", fractal_version_build_date);
    printf("             By: %s\r\n", fractal_version_builder);
    printf("      Allocator: %s\r\n", HEAP_IMPLEMENTATION_NAME);
    printf("      Load Addr: 0x%X\r\n", KERNEL_LINK_ADDRESS);
    printf("     init_array: 0x%X-0x%X\r\n", &_init_array_begin, &_init_array_end);
    printf("         initrd: 0x%X\r\n", global_initrd);
    printf(" last kern addr: 0x%X\r\n", &_last_kernel_address);
    printf("Small page size: 0x%X\r\n", SMALL_PAGE_SIZE);
    printf("Large page size: 0x%X\r\n", LARGE_PAGE_SIZE);
    dump_page_allocator_info();
    printf(" sbrk allocator: 0x%X-0x%X\r\n", sbrk_heap_base, sbrk_heap_base + sbrk_heap_len);
#endif // ! QUIET_BOOT
}

extern "C" void idle_thread() {
    while(true) naptime();
}

extern "C" void oxide_init_hardware() {
    init_catalog();
    global_compositor.Initialize();
    global_cpu.DetectHardwareFramebuffer();

    if (NULL == global_gpu) {
        // If no hardware framebuffer was detected, try PCI
        InitPCI(KERN_P2DV(global_cpu.GetPCIMMCFG()));
    } else {
        // We found a hardware framebuffer, there's probably
        // more IO where that came from! (eg. legacy PS/2 IO)
        global_cpu.DetectPlatformIO();
    }

    if (NULL == global_gpu) {
        panic("Failed to locate a display device, but graphics are enabled");
    }
}

extern "C" void oxide_main_thread() {
    struct virtio_gpu_rect cur_screen_dim;
    u32 prev_width  = VIRTIO_DEFAULT_SCREEN_W;
    u32 prev_height = VIRTIO_DEFAULT_SCREEN_H;
    while(true) {
        global_gpu->QueryScreensize(&cur_screen_dim);

        // Only update screen width and height from starting values
        // if they changed from the last frame. This is because
        // Qemu reports the screen as always starting at 1280 by 800,
        // but we want to start at 1920 by 1080. If we always trust
        // whatever the screen size says we will instantly shrink
        // to smaller than we want to start at.
        if (prev_width != cur_screen_dim.width || prev_height != cur_screen_dim.height) {
            // Hardware-controlled resolution override
            // Read new screen resolution from Virtio GPU
            SCREEN_W = cur_screen_dim.width;
            SCREEN_H = cur_screen_dim.height;
            oxide_should_adjust_resolution = false;
            global_compositor.blur_needs_repaint = true;
            global_compositor.background_will_change = true;
        }

        if (oxide_should_adjust_resolution) {
            // Software-controlled resolution override
            SCREEN_W = next_screen_w;
            SCREEN_H = next_screen_h;
            oxide_should_adjust_resolution = false;
            global_compositor.blur_needs_repaint = true;
            global_compositor.background_will_change = true;
        }

        // If either a hardware-controlled or software-controlled resolution update occurred,
        // ensure the screen size does not exceed our supported max
        if (SCREEN_W > SCREEN_MAX_W) SCREEN_W = SCREEN_MAX_W;
        if (SCREEN_H > SCREEN_MAX_H) SCREEN_H = SCREEN_MAX_H;

        global_compositor.Draw();
        global_gpu->Repaint();
        naptime();

        prev_width = cur_screen_dim.width;
        prev_height = cur_screen_dim.height;
    }
}

extern "C" void root_shell_thread() {
    #ifndef USE_GRAPHICS
        // Always need something to switch to, which normally is the graphics thread
        // However, if it doesn't exist, then we need to spawn an idle thread, because
        // if the init process we spawn ever blocks, we will have nothing to schedule,
        // and that is a bad thing(tm)
        thread_t *t_idle = create_kernel_inner_thread(true, (u64)&idle_thread);
        if (!t_idle) panic("failed to create idle thread");
        schedule_thread(t_idle);
        naptime(); // Ensure idle_thread is scheduled at least once before running
                   // main serial shell. This makes it easier to step into the
                   // program by breaking on iret (or whatever the insn is for
                   // the debugee arch)
    #else
        thread_t *graphics_task = create_kernel_inner_thread(true, (u64)oxide_main_thread);
        if (!graphics_task) panic("Failed to create oxide runtime");
        schedule_thread(graphics_task);
        naptime();
    #endif // !USE_GRAPHICS

    u64 rval = 0;

    while(true) {
        char *args[TASK_MAX_NUM_ARGS];
        args[0] = INIT_TASK_NAME;
        args[1] = "-l";
        args[2] = NULL;
#ifdef SINGLE_PROGRAM_MODE
        #define TASK_LAUNCH_FLAGS (((SPAWN_MODE_COOPERATIVE | SPAWN_MODE_WAIT_FINISH | SPAWN_MODE_USER_TASK)))
#else
        #define TASK_LAUNCH_FLAGS (((SPAWN_MODE_PREEMPTIVE | SPAWN_MODE_WAIT_FINISH | SPAWN_MODE_USER_TASK)))
#endif
        rval = do_sys_spawn(current_task(), INIT_TASK, args, TASK_LAUNCH_FLAGS);
        printf("\r\ninit task returned %d\r\nshutting down!\n", rval);

#ifdef SINGLE_PROGRAM_MODE
#ifdef BOARD_HAS_REBOOT
        extern void __board_reboot__();
        __board_reboot__();
#endif // BOARD_HAS_REBOOT
        while(1);
#endif // SINGLE_PROGRAM_MODE
    }
}

/*
 * fractal_main
 * Main entrypoint to C/C++ land.
 *
 * Responsible for bootstrapping the C++ features,
 * initializing devices, and handing control off
 * to the main boot program.
 *
 * Assumes we are running with a Fractal boot page
 * table, which maps the first 512GB of physical memory
 * copied across the entire address space, including
 * the upper half, and that a long jump was used to call
 * fractal_main's higher half alias.
 *
 * Should never return!
 */
extern "C" __attribute__((noreturn)) void fractal_main(u64 *__optional_boot_args, u64 *__fractal_base) {
    // Construct all global C++ objects, need to run this before
    // touching any global C++ objects (eg. serial ports or CPU classes).
    run_ctors();
    global_serial_device.Initialize();

    // check platform defs
    if (SMALL_PAGE_SIZE != (4ull * ONE_KB)) panic("small page isn't 4k");
    if (LARGE_PAGE_SIZE != (2ull * ONE_MB)) panic("large page isn't 2m");
    if (GIANT_PAGE_SIZE != (1ull * ONE_GB)) panic("giant page isn't 1g");
    if (SUPER_PAGE_SIZE != (512ull * ONE_GB)) panic("super page isn't 512g");

#ifndef BOARD_APPLE
    // Apple boards don't require newline fixups
    printf(SERIAL_FIXUP_NEWLINES);
#else
    printf("\n");
#endif // ! BOARD_APPLE

    global_initrd = (virt_t)global_cpu.FindInitrd();
    if ((virt_t)NULL == global_initrd) panic("Could not find initrd");

    // Fractal has two dynamic memory providers- the dlmalloc / sbrk heap and the page heap.
    // The page heap provides small (4K) and large (2M) pages
    // The sbrk heap is consumed by dlmalloc
    // page heap uses dlmalloc to manage allocations, so
    // page heap needs to be created first
    // Put the heap at INITRD_MAX_SIZE bytes after initrd
    // initrd on riscv/arm is at 512MB after kernel start (iirc), so this will be 768MB after kernel
    // On x86 it is hard to predict where multiboot will put it, so it'll just put it somewhere after
    // We will say the minimum memory required is 2GB to be more than safe to fit the page heap.
    usize first_heap_addr = ALIGN_LARGE_PAGE((usize)global_initrd + INITRD_MAX_SIZE);
    init_sbrk_heap(first_heap_addr, SBRK_HEAP_SIZE);
    init_page_heap(first_heap_addr + SBRK_HEAP_SIZE, PAGE_HEAP_SIZE);

    DumpCoreInfo();
#ifndef QUIET_BOOT
    printf("kernel base: 0x%X\n", __fractal_base);
#endif // QUIET_BOOT

#ifdef BOARD_HAS_BRINGUP_FUNCTION
    void __board_bringup__();
    __board_bringup__();
#endif // BOARD_HAS_BRINGUP_FUNCTION

    // Most of paging initialization removed- tasks will setup page tables
    // We assume we are still on the boot page tables for the time being
    // This method only exists for ARM to setup its split page table,
    // every other arch does nothing:
    global_cpu.InitializePaging();

    // Create a new kernel map and switch into it
    // Yes, this "wastes" a page, but I have had enough
    // bugs during init where we assume eg. devices or
    // framebuffers are mapped with certain memory attributes,
    // only to remember we are on the bringup page table still
    // where everything is flat.
    u64 *kernel_init_pagetable = (u64 *)AllocPage(SMALL_PAGE);

    memset(
        kernel_init_pagetable,
        '\x00',
        SMALL_PAGE_SIZE
    );

    global_cpu.MapKernel(kernel_init_pagetable);
    global_cpu.SetPageTable(kernel_init_pagetable, 0);
    global_cpu.InitializeCaching();
    global_cpu.FlushTLB();

    // Configure interrupts (and other various misc. processor tables)
    // We don't enable them until we switch out to the root task though.
    global_cpu.InitializeInterrupts();
    global_cpu.EnableInterrupt(INT_UART);
    global_cpu.EnableInterrupt(INT_TIMER);

    global_cpu.InitializeSyscalls();

    fs_init();

    create_kernel_root_task();

#ifdef USE_GRAPHICS
    oxide_init_hardware();
#endif // USE_GRAPHICS

    // Create and run init tasks
    thread_t *root_shell_task = create_kernel_inner_thread(true, (u64)root_shell_thread);
    if (!root_shell_task) panic("Failed to create root shell task");
    init_serial_stdio(kernel_task);

#ifdef ARCH_HAS_FAST_MEMSET
    // On ARM, memset causes misaligned stores, which cause faults
    // before MAIR is active which happens when we switch to the pagetable
    // for our first user task. So, this flag is used to set memset
    // to be the slow (but always aligned) version until we jump
    // into the first user task, where we can speed things up.
    // Can't just set MAIR because the boot page tables (which we are on)
    // store all code and device peripherals in the same giant page.
    __fast_memops_active = true;
#endif // ARCH_HAS_FAST_MEMSET

    switchto(root_shell_task);
    panic("Initial task somehow transferred control back to the initial kernel stack, that should literally be impossible");
    while(true);
}
