#ifndef FRACTAL_ARM
#error "Including ARM ultrathread without being the FRACTAL_ARM target"
#endif // !FRACTAL_ARM

.global ultra_vbar
.global ultra_switch
.global ultra_ustack
.global ultra_kstack
.global ultra_next_sp

.section .text

#define REGS_SIZE ((34 * 8))
#define REG_FIELD(i) [sp,#(8*i)]

.balign 0x1000
ultra_vbar:
  .rept 2048
  nop
  .endr

ultra_switch:
  add sp, sp, -(REGS_SIZE)
  str x0,  REG_FIELD(0)
  str x1,  REG_FIELD(1)
  str x2,  REG_FIELD(2)
  str x3,  REG_FIELD(3)
  str x4,  REG_FIELD(4)
  str x5,  REG_FIELD(5)
  str x6,  REG_FIELD(6)
  str x7,  REG_FIELD(7)
  str x8,  REG_FIELD(8)
  str x9,  REG_FIELD(9)
  str x10, REG_FIELD(10)
  str x11, REG_FIELD(11)
  str x12, REG_FIELD(12)
  str x13, REG_FIELD(13)
  str x14, REG_FIELD(14)
  str x15, REG_FIELD(15)
  str x16, REG_FIELD(16)
  str x17, REG_FIELD(17)
  str x18, REG_FIELD(18)
  str x19, REG_FIELD(19)
  str x20, REG_FIELD(20)
  str x21, REG_FIELD(21)
  str x22, REG_FIELD(22)
  str x23, REG_FIELD(23)
  str x24, REG_FIELD(24)
  str x25, REG_FIELD(25)
  str x26, REG_FIELD(26)
  str x27, REG_FIELD(27)
  str x28, REG_FIELD(28)
  str x29, REG_FIELD(29)
  str x30, REG_FIELD(30)
  mrs x0, sp_el0
  str x0, REG_FIELD(31)
  mrs x0, spsr_el1
  str x0, REG_FIELD(32)
  mrs x0, elr_el1
  str x0, REG_FIELD(33)

  adr x0, ultra_next_sp
  mov x18, sp
  ldr x19, [x0]
  str x18, [x0]
  mov sp, x19

  ldr x0, REG_FIELD(33)
  msr elr_el1, x0
  ldr x0, REG_FIELD(32)
  msr spsr_el1, x0
  ldr x0, REG_FIELD(31)
  msr sp_el0, x0
  ldr x30, REG_FIELD(30)
  ldr x29, REG_FIELD(29)
  ldr x28, REG_FIELD(28)
  ldr x27, REG_FIELD(27)
  ldr x26, REG_FIELD(26)
  ldr x25, REG_FIELD(25)
  ldr x24, REG_FIELD(24)
  ldr x23, REG_FIELD(23)
  ldr x22, REG_FIELD(22)
  ldr x21, REG_FIELD(21)
  ldr x20, REG_FIELD(20)
  ldr x19, REG_FIELD(19)
  ldr x18, REG_FIELD(18)
  ldr x17, REG_FIELD(17)
  ldr x16, REG_FIELD(16)
  ldr x15, REG_FIELD(15)
  ldr x14, REG_FIELD(14)
  ldr x13, REG_FIELD(13)
  ldr x12, REG_FIELD(12)
  ldr x11, REG_FIELD(11)
  ldr x10, REG_FIELD(10)
  ldr x9,  REG_FIELD(9)
  ldr x8,  REG_FIELD(8)
  ldr x7,  REG_FIELD(7)
  ldr x6,  REG_FIELD(6)
  ldr x5,  REG_FIELD(5)
  ldr x4,  REG_FIELD(4)
  ldr x3,  REG_FIELD(3)
  ldr x2,  REG_FIELD(2)
  ldr x1,  REG_FIELD(1)
  ldr x0,  REG_FIELD(0)
  add sp, sp, REGS_SIZE
  eret

.section .data

// Thread used while running user code in thread 2
.balign 0x1000
  .rept 512
  .quad 0
  .endr
ultra_ustack:
  .rept 4
  .quad 0
  .endr

// Thread used for storing user regs when entering our nanokernel from the user thread
.balign 0x1000
  .rept 512
  .quad 0
  .endr
ultra_kstack:
  .rept 4
  .quad 0
  .endr

ultra_next_sp:
  .quad ultra_kstack - REGS_SIZE

