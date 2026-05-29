#ifndef EXT2_BLOCKGROUP_H
#define EXT2_BLOCKGROUP_H

#include <fractal.h>

BEGIN_C_HEADER

typedef struct ext2_blockgroup_descriptor_table {
    u32 block_bitmap; /* Block ID of the block bitmap of this group */
    u32 inode_bitmap; /* Block ID of the inode bitmap of this group */
    u32 inode_table;  /* Block ID of the inode table of this group */
    u16 num_free_blocks;
    u16 num_free_inodes;
    u16 num_used_dirs;
    u16 __pad;
    u8  __reserved[12];
} ext2_bgd_t;

END_C_HEADER

#endif // EXT2_BLOCKGROUP_H
