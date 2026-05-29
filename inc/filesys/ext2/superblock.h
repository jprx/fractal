#ifndef EXT2_SUPERBLOCK_H
#define EXT2_SUPERBLOCK_H

#include <fractal.h>

BEGIN_C_HEADER

// 1024 bytes from the top of the disk
#define EXT2_SUPER_MAGIC       0xEF53
#define EXT2_SUPERBLOCK_SIZE   1024
#define EXT2_BLOCK_LOG_SIZE    1024

/* Ref: https://www.nongnu.org/ext2-doc/ext2.html#superblock */
// Always at 1024 bytes into the disk
// Copies of this are stored at the start of each block group as well (as a backup)
typedef struct {
    // Total # of inodes, used and free, in the system
    // Must be <= inodes_per_group * number of block groups
    u32 inode_count;

    // Total # of blocks, used and free, in the system
    // Must be <= blocks_per_group * number of block groups
    u32 block_count;

    // # of reserved blocks for the superuser
    // If a malicious user fills the filesystem up these blocks are
    // unaffected so the system can recover
    u32 r_block_count;

    // Total # of freed blocks across all block groups
    // (including reserved blocks, see r_blocks_count)
    u32 free_block_count;

    // Total # of freed inodes across all block groups
    u32 free_inode_count;

    // ID of the block containing the superblock structure
    // Always 0 for systems with block size > 1KB, and 1
    // for systems with block size = 1 KB
    u32 first_data_block;

    // Block size = 1024 << log_block_size
    u32 log_block_size;

    // Fragment size = 1024 << log_frag_size
    u32 log_frag_size;

    // # of blocks in each block group
    // Last block group may have less than this
    u32 blocks_per_group;

    // # of fragments in each block group
    // Also determines size of bitmap of each block group
    u32 frags_per_group;

    // # of inodes in each block group
    // Also determines the size of the inode bitmap of each block group
    // Max # of inodes per group = 8 * block size (inode bitmap must
    // itself fit in a block!)
    u32 inodes_per_group;

    // Last mount time (in unix time)
    u32 mtime;

    // Last write time (in unix time)
    u32 wtime;

    // How many times the system has been mounted since it was last
    // fully verified
    u16 mnt_count;

    // How many times it can be mounted before a full verification check
    u16 max_mnt_count;

    // Should always be EXT2_SUPER_MAGIC (0xEF53)
    u16 magic;

    // State of system, basically whether the last unmount was clean or not
    u16 state;

    // What to do if an error is detected (continue, readonly, or panic)
    // See docs for more info on error handling in ext2
    u16 errors;

    // Minor revision level of ext2 version
    u16 minor_rev_level;

    // Unix time of last system check
    u32 lastcheck;

    // Max time interval allowed between filesystem checks
    u32 checkinterval;

    // Who made this OS? 0 = Linux, there are some other ones defined too
    // See ext2 spec
    u32 creator_os;

    // EXT2_GOOD_OLD_REV is 0, and EXT2_DYNAMIC_REV is 1
    // Dynamic revision adds support for variable sized inodes, extended attributes,
    // and some other stuff. See spec
    u32 rev_level;

    // User ID for reserved blocks
    u16 def_resuid;

    // Group ID for reserved blocks
    u16 def_resgid;
} superblock_t;

END_C_HEADER

#endif // EXT2_SUPERBLOCK_H
