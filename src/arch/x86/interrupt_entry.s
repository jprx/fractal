.intel_syntax noprefix
.section .text
.global test_interrupts

#include <arch/x86/selector.h>
#include <arch/x86_asm_defines.h>
#include <arch/x86_asm_macros.h>

#define TO_STACK_BASE ((((((X86_LARGE_PAGE_SIZE - 16))))))

.global syscall_entry
syscall_entry:
    // RCX holds RIP, R11 holds RFLAGS
    // Stash userspace RSP and scratch reg (RAX) in gs
    // Locate the per-task kernel stack and jump to kernel
    swapgs
    mov gs:0, rsp
    mov gs:8, rdi

    // Load kernel stack if we are a user task
    // Otherwise, don't change stack and push where we are
    lea rdi, [rip + cur_thread]
    mov rdi, [rdi]
    mov rdi, [rdi + THREAD_KIND]
    cmp rdi, THREAD_KIND_USER_THREAD
    jne syscall_entry_for_kernel_task

    // Load stack out of task struct
syscall_entry_for_user_task:
    lea rdi, [rip + cur_thread]
    mov rdi, [rdi]
    mov rsp, [rdi + THREAD_INT_STACK_PAGE]
    lea rsp, [rsp + TO_STACK_BASE]
    jmp syscall_entry_stack_loaded

    // Fixup stack so that it is a kernel alias
syscall_entry_for_kernel_task:
    ALIAS_STACK_IF_KERNEL_TASK

    // By this point, the stack is correct
syscall_entry_stack_loaded:
    mov rdi, gs:8

    // Make some space on the stack for fake error code iret ctx
    add rsp, -X86_IRET_CONTEXT_SIZE_WITH_ERROR_AND_INTNUM
    PUSH_ALL_NO_IRETCTX

    // Fill in missing fields (rsp, rip, ss, cs, rflags)
    // If we are a USER_THREAD, use USER_DS and USER_CS,
    // otherwise use KERNEL_DS and KERNEL_CS
    mov rdi, gs:0
    mov [rsp+X86_REGS_RSP], rdi
    mov [rsp+X86_REGS_RIP], rcx
    mov [rsp+X86_REGS_RFL], r11

    lea rdi, [rip + cur_thread]
    mov rdi, [rdi]
    mov rsi, [rdi + THREAD_KIND]
    cmp rsi, THREAD_KIND_USER_THREAD
    je syscall_entry_save_user_segments

syscall_entry_save_kernel_segments:
    mov rdi, KERNEL_DS
    mov [rsp+X86_REGS_SS], rdi
    mov rdi, KERNEL_CS
    mov [rsp+X86_REGS_CS], rdi
    jmp syscall_entry_done_saving_segments

syscall_entry_save_user_segments:
    mov rdi, USER_DS
    mov [rsp+X86_REGS_SS], rdi
    mov rdi, USER_CS
    mov [rsp+X86_REGS_CS], rdi

syscall_entry_done_saving_segments:
    mov rdi, rsp
    call handle_syscall_wrapper

    lea rdi, [rip + cur_thread]
    mov rdi, [rdi]
    cmp rdi, 0
    je syscall_no_task_panic
    mov rsi, [rdi + THREAD_KIND]
    cmp rsi, THREAD_KIND_USER_THREAD
    je syscall_entry_return_user_task

syscall_entry_return_kernel_task:
    // Take the slow path, force an iretq because sysret
    // forces a restore of USER_CS and USER_DS.
    // Don't worry about rsp, we'll pop it from iret context
    POP_ALL_NO_IRETCTX
    add sp, X86_IRET_CONTEXT_JUST_ERROR_AND_INTNUM_SIZE
    swapgs
    iretq

syscall_entry_return_user_task:
    // Push saved regs rflag into R11,
    // in case software changed the flags during this syscall
    // (eg. if we changed the scheduler policy)
    mov rdi, [rsp+X86_REGS_RFL]
    mov [rsp+X86_REGS_R11], rdi

    // Stash userspace stack in gs:0 while we pop from kernel stack
    mov rdi, [rsp+X86_REGS_RSP]
    mov gs:0, rdi
    POP_ALL_NO_IRETCTX

    // Retrieve userspace stack, swap gs back, and we out
    mov rsp, gs:0
    swapgs

    // RIP <- RCX, RFLAGS <- R11
    sysretq

syscall_no_task_panic:
    jmp .

.global syscall_compat_entry
syscall_compat_entry:
    // It should be impossible to get here- we don't support this
    jmp .

// Everything in the IDT jumps here
// (so, all traps but the syscall instruction)
// Assumptions:
// - iret context is pushed including error code, if one
//   was not provided by hardware, software pushed a fake one.
// - interrupt number is pushed above the error code on iretctx.
// - extern "C" void x86_interrupt(regs_t *r) exists
.global x86_generic_int
x86_generic_int:
    PUSH_ALL_NO_IRETCTX

    // If we are a kernel task, this swaps rsp
    // to a kernel pointer from a user one so that
    // context switches later on will succeed (as
    // we are now servicing kernel code using this stack).
    // It's the same backing memory, just referred to via
    // a kernel pointer rather than the "user" one we were using.
    ALIAS_STACK_IF_KERNEL_TASK

    mov rdi,rsp
    call x86_interrupt
    POP_ALL_NO_IRETCTX
    add sp, 16
    iretq

DECLARE_INTERRUPT              IDT_ILLEGAL_INST_EXCEPTION,   illegalinst_irq
DECLARE_INTERRUPT_WITH_ERROR   IDT_PAGEFAULT_EXCEPTION,      pagefault_irq

DECLARE_INTERRUPT              UART0_IRQ,        uart0_irq
DECLARE_INTERRUPT              VIRTIO_IRQ,       virtio_irq
DECLARE_INTERRUPT              APIC_TIMER_IRQ,   lapic_timer_irq

// Processor exceptions
DECLARE_INTERRUPT             0, __unknown_int_0
DECLARE_INTERRUPT             1, __unknown_int_1
DECLARE_INTERRUPT             2, __unknown_int_2
DECLARE_INTERRUPT             3, __unknown_int_3
DECLARE_INTERRUPT             4, __unknown_int_4
DECLARE_INTERRUPT             5, __unknown_int_5
DECLARE_INTERRUPT             6, __unknown_int_6
DECLARE_INTERRUPT             7, __unknown_int_7
DECLARE_INTERRUPT_WITH_ERROR  8, __unknown_int_8
DECLARE_INTERRUPT             9, __unknown_int_9
DECLARE_INTERRUPT_WITH_ERROR  10, __unknown_int_10
DECLARE_INTERRUPT_WITH_ERROR  11, __unknown_int_11
DECLARE_INTERRUPT_WITH_ERROR  12, __unknown_int_12
DECLARE_INTERRUPT_WITH_ERROR  13, __unknown_int_13
DECLARE_INTERRUPT_WITH_ERROR  14, __unknown_int_14
DECLARE_INTERRUPT             15, __unknown_int_15
DECLARE_INTERRUPT             16, __unknown_int_16
DECLARE_INTERRUPT_WITH_ERROR  17, __unknown_int_17
DECLARE_INTERRUPT             18, __unknown_int_18
DECLARE_INTERRUPT             19, __unknown_int_19
DECLARE_INTERRUPT             20, __unknown_int_20
DECLARE_INTERRUPT_WITH_ERROR  21, __unknown_int_21
DECLARE_INTERRUPT             22, __unknown_int_22
DECLARE_INTERRUPT             23, __unknown_int_23
DECLARE_INTERRUPT             24, __unknown_int_24
DECLARE_INTERRUPT             25, __unknown_int_25
DECLARE_INTERRUPT             26, __unknown_int_26
DECLARE_INTERRUPT             27, __unknown_int_27
DECLARE_INTERRUPT             28, __unknown_int_28
DECLARE_INTERRUPT_WITH_ERROR  29, __unknown_int_29
DECLARE_INTERRUPT_WITH_ERROR  30, __unknown_int_30

// Processor interrupts (technically 32-255) but 31 is not a defined exception
DECLARE_INTERRUPT 31, __unknown_int_31
DECLARE_INTERRUPT 32, __unknown_int_32
DECLARE_INTERRUPT 33, __unknown_int_33
DECLARE_INTERRUPT 34, __unknown_int_34
DECLARE_INTERRUPT 35, __unknown_int_35
DECLARE_INTERRUPT 36, __unknown_int_36
DECLARE_INTERRUPT 37, __unknown_int_37
DECLARE_INTERRUPT 38, __unknown_int_38
DECLARE_INTERRUPT 39, __unknown_int_39
DECLARE_INTERRUPT 40, __unknown_int_40
DECLARE_INTERRUPT 41, __unknown_int_41
DECLARE_INTERRUPT 42, __unknown_int_42
DECLARE_INTERRUPT 43, __unknown_int_43
DECLARE_INTERRUPT 44, __unknown_int_44
DECLARE_INTERRUPT 45, __unknown_int_45
DECLARE_INTERRUPT 46, __unknown_int_46
DECLARE_INTERRUPT 47, __unknown_int_47
DECLARE_INTERRUPT 48, __unknown_int_48
DECLARE_INTERRUPT 49, __unknown_int_49
DECLARE_INTERRUPT 50, __unknown_int_50
DECLARE_INTERRUPT 51, __unknown_int_51
DECLARE_INTERRUPT 52, __unknown_int_52
DECLARE_INTERRUPT 53, __unknown_int_53
DECLARE_INTERRUPT 54, __unknown_int_54
DECLARE_INTERRUPT 55, __unknown_int_55
DECLARE_INTERRUPT 56, __unknown_int_56
DECLARE_INTERRUPT 57, __unknown_int_57
DECLARE_INTERRUPT 58, __unknown_int_58
DECLARE_INTERRUPT 59, __unknown_int_59
DECLARE_INTERRUPT 60, __unknown_int_60
DECLARE_INTERRUPT 61, __unknown_int_61
DECLARE_INTERRUPT 62, __unknown_int_62
DECLARE_INTERRUPT 63, __unknown_int_63
DECLARE_INTERRUPT 64, __unknown_int_64
DECLARE_INTERRUPT 65, __unknown_int_65
DECLARE_INTERRUPT 66, __unknown_int_66
DECLARE_INTERRUPT 67, __unknown_int_67
DECLARE_INTERRUPT 68, __unknown_int_68
DECLARE_INTERRUPT 69, __unknown_int_69
DECLARE_INTERRUPT 70, __unknown_int_70
DECLARE_INTERRUPT 71, __unknown_int_71
DECLARE_INTERRUPT 72, __unknown_int_72
DECLARE_INTERRUPT 73, __unknown_int_73
DECLARE_INTERRUPT 74, __unknown_int_74
DECLARE_INTERRUPT 75, __unknown_int_75
DECLARE_INTERRUPT 76, __unknown_int_76
DECLARE_INTERRUPT 77, __unknown_int_77
DECLARE_INTERRUPT 78, __unknown_int_78
DECLARE_INTERRUPT 79, __unknown_int_79
DECLARE_INTERRUPT 80, __unknown_int_80
DECLARE_INTERRUPT 81, __unknown_int_81
DECLARE_INTERRUPT 82, __unknown_int_82
DECLARE_INTERRUPT 83, __unknown_int_83
DECLARE_INTERRUPT 84, __unknown_int_84
DECLARE_INTERRUPT 85, __unknown_int_85
DECLARE_INTERRUPT 86, __unknown_int_86
DECLARE_INTERRUPT 87, __unknown_int_87
DECLARE_INTERRUPT 88, __unknown_int_88
DECLARE_INTERRUPT 89, __unknown_int_89
DECLARE_INTERRUPT 90, __unknown_int_90
DECLARE_INTERRUPT 91, __unknown_int_91
DECLARE_INTERRUPT 92, __unknown_int_92
DECLARE_INTERRUPT 93, __unknown_int_93
DECLARE_INTERRUPT 94, __unknown_int_94
DECLARE_INTERRUPT 95, __unknown_int_95
DECLARE_INTERRUPT 96, __unknown_int_96
DECLARE_INTERRUPT 97, __unknown_int_97
DECLARE_INTERRUPT 98, __unknown_int_98
DECLARE_INTERRUPT 99, __unknown_int_99
DECLARE_INTERRUPT 100, __unknown_int_100
DECLARE_INTERRUPT 101, __unknown_int_101
DECLARE_INTERRUPT 102, __unknown_int_102
DECLARE_INTERRUPT 103, __unknown_int_103
DECLARE_INTERRUPT 104, __unknown_int_104
DECLARE_INTERRUPT 105, __unknown_int_105
DECLARE_INTERRUPT 106, __unknown_int_106
DECLARE_INTERRUPT 107, __unknown_int_107
DECLARE_INTERRUPT 108, __unknown_int_108
DECLARE_INTERRUPT 109, __unknown_int_109
DECLARE_INTERRUPT 110, __unknown_int_110
DECLARE_INTERRUPT 111, __unknown_int_111
DECLARE_INTERRUPT 112, __unknown_int_112
DECLARE_INTERRUPT 113, __unknown_int_113
DECLARE_INTERRUPT 114, __unknown_int_114
DECLARE_INTERRUPT 115, __unknown_int_115
DECLARE_INTERRUPT 116, __unknown_int_116
DECLARE_INTERRUPT 117, __unknown_int_117
DECLARE_INTERRUPT 118, __unknown_int_118
DECLARE_INTERRUPT 119, __unknown_int_119
DECLARE_INTERRUPT 120, __unknown_int_120
DECLARE_INTERRUPT 121, __unknown_int_121
DECLARE_INTERRUPT 122, __unknown_int_122
DECLARE_INTERRUPT 123, __unknown_int_123
DECLARE_INTERRUPT 124, __unknown_int_124
DECLARE_INTERRUPT 125, __unknown_int_125
DECLARE_INTERRUPT 126, __unknown_int_126
DECLARE_INTERRUPT 127, __unknown_int_127
DECLARE_INTERRUPT 128, __unknown_int_128
DECLARE_INTERRUPT 129, __unknown_int_129
DECLARE_INTERRUPT 130, __unknown_int_130
DECLARE_INTERRUPT 131, __unknown_int_131
DECLARE_INTERRUPT 132, __unknown_int_132
DECLARE_INTERRUPT 133, __unknown_int_133
DECLARE_INTERRUPT 134, __unknown_int_134
DECLARE_INTERRUPT 135, __unknown_int_135
DECLARE_INTERRUPT 136, __unknown_int_136
DECLARE_INTERRUPT 137, __unknown_int_137
DECLARE_INTERRUPT 138, __unknown_int_138
DECLARE_INTERRUPT 139, __unknown_int_139
DECLARE_INTERRUPT 140, __unknown_int_140
DECLARE_INTERRUPT 141, __unknown_int_141
DECLARE_INTERRUPT 142, __unknown_int_142
DECLARE_INTERRUPT 143, __unknown_int_143
DECLARE_INTERRUPT 144, __unknown_int_144
DECLARE_INTERRUPT 145, __unknown_int_145
DECLARE_INTERRUPT 146, __unknown_int_146
DECLARE_INTERRUPT 147, __unknown_int_147
DECLARE_INTERRUPT 148, __unknown_int_148
DECLARE_INTERRUPT 149, __unknown_int_149
DECLARE_INTERRUPT 150, __unknown_int_150
DECLARE_INTERRUPT 151, __unknown_int_151
DECLARE_INTERRUPT 152, __unknown_int_152
DECLARE_INTERRUPT 153, __unknown_int_153
DECLARE_INTERRUPT 154, __unknown_int_154
DECLARE_INTERRUPT 155, __unknown_int_155
DECLARE_INTERRUPT 156, __unknown_int_156
DECLARE_INTERRUPT 157, __unknown_int_157
DECLARE_INTERRUPT 158, __unknown_int_158
DECLARE_INTERRUPT 159, __unknown_int_159
DECLARE_INTERRUPT 160, __unknown_int_160
DECLARE_INTERRUPT 161, __unknown_int_161
DECLARE_INTERRUPT 162, __unknown_int_162
DECLARE_INTERRUPT 163, __unknown_int_163
DECLARE_INTERRUPT 164, __unknown_int_164
DECLARE_INTERRUPT 165, __unknown_int_165
DECLARE_INTERRUPT 166, __unknown_int_166
DECLARE_INTERRUPT 167, __unknown_int_167
DECLARE_INTERRUPT 168, __unknown_int_168
DECLARE_INTERRUPT 169, __unknown_int_169
DECLARE_INTERRUPT 170, __unknown_int_170
DECLARE_INTERRUPT 171, __unknown_int_171
DECLARE_INTERRUPT 172, __unknown_int_172
DECLARE_INTERRUPT 173, __unknown_int_173
DECLARE_INTERRUPT 174, __unknown_int_174
DECLARE_INTERRUPT 175, __unknown_int_175
DECLARE_INTERRUPT 176, __unknown_int_176
DECLARE_INTERRUPT 177, __unknown_int_177
DECLARE_INTERRUPT 178, __unknown_int_178
DECLARE_INTERRUPT 179, __unknown_int_179
DECLARE_INTERRUPT 180, __unknown_int_180
DECLARE_INTERRUPT 181, __unknown_int_181
DECLARE_INTERRUPT 182, __unknown_int_182
DECLARE_INTERRUPT 183, __unknown_int_183
DECLARE_INTERRUPT 184, __unknown_int_184
DECLARE_INTERRUPT 185, __unknown_int_185
DECLARE_INTERRUPT 186, __unknown_int_186
DECLARE_INTERRUPT 187, __unknown_int_187
DECLARE_INTERRUPT 188, __unknown_int_188
DECLARE_INTERRUPT 189, __unknown_int_189
DECLARE_INTERRUPT 190, __unknown_int_190
DECLARE_INTERRUPT 191, __unknown_int_191
DECLARE_INTERRUPT 192, __unknown_int_192
DECLARE_INTERRUPT 193, __unknown_int_193
DECLARE_INTERRUPT 194, __unknown_int_194
DECLARE_INTERRUPT 195, __unknown_int_195
DECLARE_INTERRUPT 196, __unknown_int_196
DECLARE_INTERRUPT 197, __unknown_int_197
DECLARE_INTERRUPT 198, __unknown_int_198
DECLARE_INTERRUPT 199, __unknown_int_199
DECLARE_INTERRUPT 200, __unknown_int_200
DECLARE_INTERRUPT 201, __unknown_int_201
DECLARE_INTERRUPT 202, __unknown_int_202
DECLARE_INTERRUPT 203, __unknown_int_203
DECLARE_INTERRUPT 204, __unknown_int_204
DECLARE_INTERRUPT 205, __unknown_int_205
DECLARE_INTERRUPT 206, __unknown_int_206
DECLARE_INTERRUPT 207, __unknown_int_207
DECLARE_INTERRUPT 208, __unknown_int_208
DECLARE_INTERRUPT 209, __unknown_int_209
DECLARE_INTERRUPT 210, __unknown_int_210
DECLARE_INTERRUPT 211, __unknown_int_211
DECLARE_INTERRUPT 212, __unknown_int_212
DECLARE_INTERRUPT 213, __unknown_int_213
DECLARE_INTERRUPT 214, __unknown_int_214
DECLARE_INTERRUPT 215, __unknown_int_215
DECLARE_INTERRUPT 216, __unknown_int_216
DECLARE_INTERRUPT 217, __unknown_int_217
DECLARE_INTERRUPT 218, __unknown_int_218
DECLARE_INTERRUPT 219, __unknown_int_219
DECLARE_INTERRUPT 220, __unknown_int_220
DECLARE_INTERRUPT 221, __unknown_int_221
DECLARE_INTERRUPT 222, __unknown_int_222
DECLARE_INTERRUPT 223, __unknown_int_223
DECLARE_INTERRUPT 224, __unknown_int_224
DECLARE_INTERRUPT 225, __unknown_int_225
DECLARE_INTERRUPT 226, __unknown_int_226
DECLARE_INTERRUPT 227, __unknown_int_227
DECLARE_INTERRUPT 228, __unknown_int_228
DECLARE_INTERRUPT 229, __unknown_int_229
DECLARE_INTERRUPT 230, __unknown_int_230
DECLARE_INTERRUPT 231, __unknown_int_231
DECLARE_INTERRUPT 232, __unknown_int_232
DECLARE_INTERRUPT 233, __unknown_int_233
DECLARE_INTERRUPT 234, __unknown_int_234
DECLARE_INTERRUPT 235, __unknown_int_235
DECLARE_INTERRUPT 236, __unknown_int_236
DECLARE_INTERRUPT 237, __unknown_int_237
DECLARE_INTERRUPT 238, __unknown_int_238
DECLARE_INTERRUPT 239, __unknown_int_239
DECLARE_INTERRUPT 240, __unknown_int_240
DECLARE_INTERRUPT 241, __unknown_int_241
DECLARE_INTERRUPT 242, __unknown_int_242
DECLARE_INTERRUPT 243, __unknown_int_243
DECLARE_INTERRUPT 244, __unknown_int_244
DECLARE_INTERRUPT 245, __unknown_int_245
DECLARE_INTERRUPT 246, __unknown_int_246
DECLARE_INTERRUPT 247, __unknown_int_247
DECLARE_INTERRUPT 248, __unknown_int_248
DECLARE_INTERRUPT 249, __unknown_int_249
DECLARE_INTERRUPT 250, __unknown_int_250
DECLARE_INTERRUPT 251, __unknown_int_251
DECLARE_INTERRUPT 252, __unknown_int_252
DECLARE_INTERRUPT 253, __unknown_int_253
DECLARE_INTERRUPT 254, __unknown_int_254
DECLARE_INTERRUPT 255, __unknown_int_255
