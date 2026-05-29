#include <fractal.h>
#include <loader/elf.h>
#include <stdarg.h>
#include <types.h>
#include <out.h>
#include <lib/mem.h>
#include <kasan.h>

//#define APPLESI_MODE_VMAPPLE
#define APPLESI_MODE_M4

#define TEXT_START __attribute__((section(".text.start_c")))

// Take relocs_begin/ end as arguments from assembly, rather than using something like &_relocs_begin
// to read from linker script, as this way we can guarantee PC-relative loads for these
TEXT_START KASAN_IGNORE void arm_handle_relocs(u64 lowest_kernel_addr, Elf64_Rela *relocs_begin, Elf64_Rela *relocs_end) {
    Elf64_Rela *ptr = relocs_begin;

    while (ptr < relocs_end) {
        switch(ptr->r_info) {
            case R_AARCH64_RELATIVE:
                *(u64 *)(lowest_kernel_addr + ptr->r_offset) = lowest_kernel_addr + ptr->r_addend;
            break;

            default:
                // Unknown relocation kind- and it's too early to panic!
                while(1);
            break;
        }
        ptr++;
    }
}
