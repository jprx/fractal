#ifndef MULTIBOOT_1_H
#define MULTIBOOT_1_H

#include <types.h>

// See: https://www.gnu.org/software/grub/manual/multiboot/multiboot.html

#define MULTIBOOT_FRAMEBUFFER_PRESENT_FLAG ((BIT(12)))

typedef struct {
    u32 magic;
    u32 flags;
    u32 checksum;
    u32 header_addr;        // if flags[16] set
    u32 load_addr;          // if flags[16] set
    u32 load_end_addr;      // if flags[16] set
    u32 bss_end_addr;       // if flags[16] set
    u32 entry_addr;         // if flags[16] set
    u32 mode_type;          // if flags[2] set
    u32 width;              // if flags[2] set
    u32 height;             // if flags[2] set
    u32 depth;              // if flags[2] set
} multiboot_header_t;

struct multiboot_aout_symbol_table
{
  u32 tabsize;
  u32 strsize;
  u32 addr;
  u32 reserved;
};
typedef struct multiboot_aout_symbol_table multiboot_aout_symbol_table_t;

struct multiboot_elf_section_header_table
{
  u32 num;
  u32 size;
  u32 addr;
  u32 shndx;
};
typedef struct multiboot_elf_section_header_table multiboot_elf_section_header_table_t;

struct multiboot_info
{
  /* Multiboot info version number */
  u32 flags;

  /* Available memory from BIOS */
  u32 mem_lower;
  u32 mem_upper;

  /* "root" partition */
  u32 boot_device;

  /* Kernel command line */
  u32 cmdline;

  /* Boot-Module list */
  u32 mods_count;
  u32 mods_addr;

  union
  {
    multiboot_aout_symbol_table_t aout_sym;
    multiboot_elf_section_header_table_t elf_sec;
  } u;

  /* Memory Mapping buffer */
  u32 mmap_length;
  u32 mmap_addr;

  /* Drive Info buffer */
  u32 drives_length;
  u32 drives_addr;

  /* ROM configuration table */
  u32 config_table;

  /* Boot Loader Name */
  u32 boot_loader_name;

  /* APM table */
  u32 apm_table;

  /* Video */
  u32 vbe_control_info;
  u32 vbe_mode_info;
  u16 vbe_mode;
  u16 vbe_interface_seg;
  u16 vbe_interface_off;
  u16 vbe_interface_len;

  u64 framebuffer_addr;
  u32 framebuffer_pitch;
  u32 framebuffer_width;
  u32 framebuffer_height;
  u8 framebuffer_bpp;
#define MULTIBOOT_FRAMEBUFFER_TYPE_INDEXED 0
#define MULTIBOOT_FRAMEBUFFER_TYPE_RGB     1
#define MULTIBOOT_FRAMEBUFFER_TYPE_EGA_TEXT     2
  u8 framebuffer_type;
  union
  {
    struct
    {
      u32 framebuffer_palette_addr;
      u16 framebuffer_palette_num_colors;
    };
    struct
    {
      u8 framebuffer_red_field_position;
      u8 framebuffer_red_mask_size;
      u8 framebuffer_green_field_position;
      u8 framebuffer_green_mask_size;
      u8 framebuffer_blue_field_position;
      u8 framebuffer_blue_mask_size;
    };
  };
};
typedef struct multiboot_info multiboot_info_t;

typedef struct {
    u32 mod_start;
    u32 mod_end;
    u32 cmdline;
    u32 __pad;
} multiboot_mod_t;

#endif // MULTIBOOT_1_H
