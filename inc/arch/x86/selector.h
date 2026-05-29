#ifndef SELECTOR_H
#define SELECTOR_H

/*
 * Segment Selector
 * |15           3| 2  |1   0|
 * +------------------------+
 * |      SI      | TI | RPL |
 * +------------------------+
 * SI = Selector Index
 * TI = Table Index
 * RPL = Requestor Privilege Level
 */

// Shift out the RPL and TI bits
#define SELECTOR_TO_GDT_INDEX(s) ((s >> 3))

#define IS_USER_SELECTOR(s) (((s & 0x03) != 0))

#define DPL_KERNEL 0b00ull
#define DPL_USER   0b11ull

/*
 * +-----------+
 * | Kernel CS | <- star.syscall_cs points here
 * +-----------+
 * |   Any DS  |    (syscall_cs + 8)
 * +-----------+
 * |    TSS    | <- task register points here
 * +-----------+
 * | TSS (cont)|
 * +-----------+
 * |   Unused  | <- star.syret_cs points here
 * +-----------+
 * |  User DS  |    (sysret_cs + 8)
 * +-----------+
 * |  User CS  |    (sysret_cs + 16)
 * +-----------+
 *
 * On syscall, cs <- star.syscall_cs; ds <- star.syscall_cs + 8
 * On sysret,  cs <- star.sysret_cs + 16; ds <- star.sysret_cs + 8
 */
#define KERNEL_CS 0x08
#define KERNEL_DS 0x10

// TSS takes up two slots:
#define TSS_SELECTOR  0x18
#define TSS_SELECTOR2 0x20

#define USER_CS_COMPAT 0x2B
#define USER_DS 0x33
#define USER_CS 0x3B

#endif // SELECTOR_H
