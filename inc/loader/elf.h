#ifndef ELF_H
#define ELF_H

#include <fractal.h>
#include <task.h>

BEGIN_C_HEADER

// See System V Application Binary Interface - DRAFT - 10 June 2013, Chapter 4:
// https://www.sco.com/developers/gabi/latest/contents.html

// Full ELF64 specification: https://uclibc.org/specs.html

// RISCV ELF specs: https://github.com/riscv-non-isa/riscv-elf-psabi-doc/blob/master/riscv-elf.adoc

#define ELF_IDENTLEN 16

#define ELF_IDENT_MAG0         0
#define ELF_IDENT_MAG1         1
#define ELF_IDENT_MAG2         2
#define ELF_IDENT_MAG3         3
#define ELF_IDENT_CLASS        4
#define ELF_IDENT_DATA         5
#define ELF_IDENT_VERSION      6
#define ELF_IDENT_OSABI        7
#define ELF_IDENT_ABIVERSION   8
#define ELF_IDENT_PAD          9

// First 4 bytes of ident must be this:
#define ELF_MAGIC 'FLE\x7F'

// ident[ELF_IDENT_CLASS]:
#define ELFCLASSNONE 0
#define ELFCLASS32 1
#define ELFCLASS64 2

// ident[ELF_IDENT_DATA]:
#define ELFDATA2LSB 1
#define ELFDATA2MSB 2

// ident[ELF_IDENT_OSABI]:
#define ELFOSABI_SYSV       0
#define ELFOSABI_HPUX       1
#define ELFOSABI_STANDALONE 255

// header.type:
#define ELF_ET_NONE   0
#define ELF_ET_REL    1
#define ELF_ET_EXEC   2
#define ELF_ET_DYN    3
#define ELF_ET_CORE   4

// header.machine (at least, the values we care about):
#define ELF_MACHINE_X86_64  62
#define ELF_MACHINE_AARCH64 183
#define ELF_MACHINE_RISCV   243

typedef struct {
    u8      ident[ELF_IDENTLEN];    /* ELF magic + identification */
    u16     type;                   /* Object file type */
    u16     machine;                /* Machine type */
    u32     version;                /* Object file version */
    u64     entrypoint;             /* Entry point address */
    u64     ph_offset;              /* Program header offset */
    u64     sh_offset;              /* Section header offset */
    u32     flags;                  /* Processor-specific flags */
    u16     header_size;            /* ELF header size */
    u16     ph_entsize;             /* Size of program header entry */
    u16     ph_num;                 /* Number of program header entries */
    u16     sh_entsize;             /* Size of section header entry */
    u16     sh_num;                 /* Number of section header entries */
    u16     stringtable_idx;        /* Section name string table index */
} ElfHeader;

// Segment types
#define ELF_PT_NULL    0
#define ELF_PT_LOAD    1
#define ELF_PT_DYN     2
#define ELF_PT_INTERP  3
#define ELF_PT_NOTE    4
#define ELF_PT_SHLIB   5
#define ELF_PT_PHDR    6
#define ELF_PT_TLS     7

// Processor-specific segments
// See: https://github.com/riscv-non-isa/riscv-elf-psabi-doc/blob/master/riscv-elf.adoc#rv-seg-type
#define ELF_PT_RISCV_ATTRIBUTES 0x70000003

// Segment flags (bit flags that define RWX perms)
#define ELF_PF_X 1
#define ELF_PF_W 2
#define ELF_PF_R 4

// An entry in the program header table defines a segment
typedef struct {
    u32     type;                   /* Type of segment */
    u32     flags;                  /* Segment attributes */
    u64     offset;                 /* Offset in file */
    u64     vaddr;                  /* Virt addr in memory */
    u64     paddr;                  /* Reserved, for systems with physical addressing only */
    u64     filesz;                 /* Size of segment in file */
    u64     memsz;                  /* Size of segment in memory */
    u64     align;                  /* Alignment of segment */
} ElfSegment;

/*
 * load_elf
 * Loads an ELF file off the filesystem into the task target_task.
 *
 * Arguments:
 *  - file_handle:   An opaque filesystem resource that will
 *                   work with filesys_read. Assumed valid,
 *                   but we will confirm it is in fact an ELF.
 *  - target_task:   The task into which to load this ELF.
 *
 * Must be called with interrupts disabled!
 *
 * Return Value: ALL_GOOD if no problems, otherwise
 * one of the error codes.
 */
kret_t load_elf(void *file_handle, task_t *target_task);

// A single relocation entry, used when rebasing a fully relocatable kernel
// Right now we only support this for ARM kernels, because iBoot on Apple Silicon
// always randomizes our load address (so we need to relocate ourselves during bringup).
typedef struct {
    u64 r_offset;
    u64 r_info;
    u64 r_addend;
} Elf64_Rela;

#define R_AARCH64_RELATIVE     1027

END_C_HEADER

#endif // ELF_H
