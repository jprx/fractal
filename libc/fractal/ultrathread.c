#if defined(__aarch64__)

#include <ultrathread.h>
#include <stdint.h>
#include <string.h>

typedef uint64_t u64;

extern void ultra_vbar();
extern u64 ultra_kstack;
extern u64 ultra_ustack;
extern u64 ultra_next_sp;

#define FTHREAD_USER_MAP ((0x000000000000ull))
#define FTHREAD_KERN_MAP ((0x5FFF00000000ull))

#define REGS_SIZE ((34 * 8))

#define read_msr(r) ({ uint64_t ___tmp_msr_val; \
    asm volatile ("mrs %0, " #r : "=r"(___tmp_msr_val)); \
    ___tmp_msr_val; })

#define write_msr(r,v) ({asm volatile("msr " #r ", %0" :: "r"(v));})

static uint64_t vbar_cache = 0;

void ultrathread_begin(void __attribute__((noreturn)) (*init_pc)(void)) {
  vbar_cache = read_msr(vbar_el1);

  ultra_next_sp = (uintptr_t)&ultra_kstack - REGS_SIZE;
  ultra_regs_t *ureg = (ultra_regs_t*)(ultra_next_sp);
  memset(ureg, '\x00', sizeof(*ureg));
  ureg->elr = (u64)init_pc;
  ureg->sp_el0 = (u64)&ultra_ustack;
  ureg->spsr = 0xC0; // EL0, no interrupts
 // sp and pc must be in user half:
  ureg->elr &= ~(FTHREAD_KERN_MAP);
  ureg->sp_el0 &= ~(FTHREAD_KERN_MAP);

  asm volatile("isb");
  write_msr(vbar_el1, ultra_vbar);
  asm volatile("isb");
}

void ultrathread_end() {
  asm volatile("isb");
  write_msr(vbar_el1, vbar_cache);
  asm volatile("isb");
}

#endif // __aarch64__
