#ifndef REGS_H
#define REGS_H

#include <fractal.h>

#if ARCH_X86
#include <arch/x86/regs.h>
#endif // ARCH_X86

#if ARCH_ARM
#include <arch/arm/regs.h>
#endif // ARCH_ARM

#if ARCH_RISCV
#include <arch/rv/regs.h>
#endif // ARCH_RISCV

#include <task.h>

BEGIN_C_HEADER

void dump_regs(regs_t *regs);
void set_return_value(regs_t *r, u64 v);

/*
 * set_init_regs
 * Configure a set of registers for a newly created task according to whether or not
 * it belongs to a kernel task and whether interrupts should be enabled.
 */
void set_init_regs(regs_t *regs, ThreadKind task_kind, bool interrupts_on);

/*
 * get/set pc
 * Gets or sets the current program counter (whatever that means for a given arch)
 * inside of this saved register struct (eg. pushed to the stack during a syscall).
 */
void set_pc(regs_t *regs, u64 pc);
u64 get_pc(regs_t *regs);

// set the stack pointer used for servicing interrupts,
// given the thread's kind (user or kernel). This should be
// in kernel space, as we cannot assume the task's virtual address
// mappings are present while running kernel code.
// For kernel tasks, this does nothing (they use the runtime stack).
void set_interrupt_sp(void *the_thread, regs_t *regs, u64 new_sp);

// get / set the stack pointer used during runtime
// given the thread's kind (user or kernel) in the thread's
// virtual address space.
// WARNING: Calling set_runtime_sp for a kernel task could be disastrous,
// as kernel tasks use the same stack for interrupts as they do for the task itself.
// If we call set_runtime_sp for the current task, and it is a kernel task, we just
// tried to change the stack we are currently using.
// These methods are used for pushing arguments to the user stack during task creation.
// They take a pointer to the task the regs belong to so that we can distinguish which stack pointer to use.
// Eg. on ARM there are two stack pointers, need to know if we are a user or kernel task to know which is right.
// The task arguments are treated as a void * because this file needs to be used by asm_offsets.c
// so it can't include task.h
u64  get_runtime_sp(void *the_thread, regs_t *regs);
void set_runtime_sp(void *the_thread, regs_t *regs, u64 new_sp);

#ifdef ARCH_HAS_GLOBAL_POINTER
// Some architectures (eg. RISCV) make use of a global pointer that gets
// configured during crt0. As we don't yet have a linker the kernel doesn't
// know (and probably shouldn't know) where symbols are inside the file, so it
// can't set the global pointer manually.
// When creating a new thread, we copy the global pointer from an already running thread.
u64  get_gp(void *the_thread, regs_t *regs);
void set_gp(void *the_thread, regs_t *regs, u64 new_gp);
#endif // ARCH_HAS_GLOBAL_POINTER

// Set arguments to be passed to main by crt0 (eg. argc, argv, etc)
void set_task_arg(regs_t *regs, usize idx, u64 val);

// What is the stack pointer currently being used? (Approximately)
// Used for checking whether we are using the right alias of the stack during kernel code.
// Kernel tasks map their stack twice- a copy at 0x7FFFFFFF... in virt mem, and obviously
// the real stack in kernel memory somewhere.
// Context switches *must* be using the copy of the stack in the kernel memory.
// The ALIAS_STACK_IF_KERNEL_TASK macro will swap stacks appropriately and should be used
// before calling any functions that cause context switches.
u64 get_current_sp();

/*
 * disable/enable interrupts
 * Mask / set whatever bits are necessary in the saved reg struct
 * such that when the task returns to userspace, interrupts are disabled.
 * (Eg. on X86_64, set the IF bit in rflag).
 */
void disable_interrupts(regs_t *r);
void enable_interrupts(regs_t *r);

END_C_HEADER

#endif // REGS_H
