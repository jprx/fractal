#ifndef FILESYS_FAT32_H
#define FILESYS_FAT32_H

#include <fractal.h>

BEGIN_C_HEADER

#define FAT32_EXPECTED_NUM_FATS 2

// How far into the BPB we should find the EBPB
#define EXPECTED_EBPB_OFFSET 36

#define FAT32_VOLUME_NAMELEN 11

#define FAT32_MAGIC 0xAA55

// First sector / bytes on a FAT32 volume are the "BIOS Parameter Block" (BPB)
typedef struct __attribute__((packed)) {
    u8 jmpboot[3];
    char oemname[8];
    u16 bytes_per_sector;
    u8 sectors_per_cluster;
    u16 reserved_sector_count;
    u8 num_fats;
    u16 root_entry_count;
    u16 mbz1; // mbz == "must be zero" (for FAT32)
    u8 media_type;
    u16 mbz2;
    u16 seconds_per_track_int13;
    u16 num_heads_int13;
    u32 num_hidden_sectors;
    u32 total_sectors;

    // Extended BPB area- assumed present for FAT32
    struct {
        u32 table_size;
        u16 extflags;
        u16 mbz4;
        u32 root_cluster; // should be 2
        u16 fsinfo_secnum;
        u16 bootsector;
        u8 reserved0[12];
        u8 int_num;
        u8 reserved1;
        u8 boot_signature;
        u8 volume_id[4];
        char volume_label[FAT32_VOLUME_NAMELEN];
        u8 filesystype[8]; // "FAT32  "
        u8 mbz5[420];
        u16 signature_word; // Should be FAT32_MAGIC (0x55AA except flipped bc endianness)
    } ebpb;
} fat32_bpb_t;

// An entry in the FAT
typedef u32 fat_entry_t;

typedef struct {
    u8 name[11];
    u8 attr;
    u8 res0;
    u16 create_time;
    u16 create_date;
    u16 last_access_date;
    u16 first_cluster_hi;
    u16 write_time;
    u16 write_date;
    u16 first_cluster_low;
    u32 file_size;
} fat_dir_t;

void init_fat32(fat32_bpb_t *);

END_C_HEADER

#endif // FILESYS_FAT32_H
