#ifndef EXT2_INODE_H
#define EXT2_INODE_H

#include <fractal.h>
#include <filesys/ext2.h>
#include <filesys/fileio.h>

BEGIN_C_HEADER

#define INODE_BLOCK_NUM_DIRECT 12

#define INODE_BLOCK_SINGLY_INDIRECT 12
#define INODE_BLOCK_DOUBLY_INDIRECT 13
#define INODE_BLOCK_TRIPLY_INDIRECT 14

#define INODE_N_BLOCKS_PER_TABLE(bsize) (( (bsize / sizeof(blocknum_t)) ))

#define INODE_DIRECT_REGION_NUMBLOCKS(bsize) (( INODE_BLOCK_NUM_DIRECT ))
#define INODE_SINGLY_INDIRECT_REGION_NUMBLOCKS(bsize) (( (INODE_N_BLOCKS_PER_TABLE(bsize)) ))
#define INODE_DOUBLY_INDIRECT_REGION_NUMBLOCKS(bsize) (( (INODE_N_BLOCKS_PER_TABLE(bsize)) * (INODE_N_BLOCKS_PER_TABLE(bsize)) ))
#define INODE_TRIPLY_INDIRECT_REGION_NUMBLOCKS(bsize) (( (INODE_N_BLOCKS_PER_TABLE(bsize)) * (INODE_N_BLOCKS_PER_TABLE(bsize)) * (INODE_N_BLOCKS_PER_TABLE(bsize)) ))

#define INODE_DIRECT_REGION_SIZE(bsize) (( bsize * (INODE_DIRECT_REGION_NUMBLOCKS(bsize)) ))
#define INODE_SINGLY_INDIRECT_REGION_SIZE(bsize) (( bsize * (INODE_SINGLY_INDIRECT_REGION_NUMBLOCKS(bsize)) ))
#define INODE_DOUBLY_INDIRECT_REGION_SIZE(bsize) (( bsize * (INODE_DOUBLY_INDIRECT_REGION_NUMBLOCKS(bsize)) ))
#define INODE_TRIPLY_INDIRECT_REGION_SIZE(bsize) (( bsize * (INODE_TRIPLY_INDIRECT_REGION_NUMBLOCKS(bsize)) ))

/* Inode mode access rights */
#define INODE_MODE_X_OTH 0x0001
#define INODE_MODE_W_OTH 0x0002
#define INODE_MODE_R_OTH 0x0004
#define INODE_MODE_X_GRP 0x0008
#define INODE_MODE_W_GRP 0x0010
#define INODE_MODE_R_GRP 0x0020
#define INODE_MODE_X_USR 0x0040
#define INODE_MODE_W_USR 0x0080
#define INODE_MODE_R_USR 0x0100

/* Inode mode process overrides */
#define INODE_MODE_SETUID 0x0800
#define INODE_MODE_SETGID 0x0400
#define INODE_MODE_STICKY 0x0200

/* Inode mode file formats */
#define INODE_MODE_SOCKET   0xC000
#define INODE_MODE_SYMLINK  0xA000
#define INODE_MODE_REGULAR  0x8000
#define INODE_MODE_BLOCK    0x6000
#define INODE_MODE_DIR      0x4000
#define INODE_MODE_CHAR     0x2000
#define INODE_MODE_FIFO     0x1000

typedef struct {
    u16 mode;  /* File kind */
    u16 uid;   /* Owner UID */
    u32 size;  /* Low 32 bits of size. Upper bits are in an "ACL" somewhere. */
    u32 atime; /* Access time */
    u32 ctime; /* Creation time */
    u32 mtime; /* Modify time */
    u32 dtime; /* Deletion time */
    u16 gid;   /* Owner GID */

    // Number of things that link to this file
    // Most files have this = 1, hard links have this = 1 + num hard links
    // If link count = 0 then we can safely delete this inode and its blocks
    u16 links_count;

    // Total number of 512 byte blocks used to contain the data of this inode
    // These are NOT the same as "filesystem blocks"!!
    // This seems to be 512 bytes because that's the sector size on floppies
    u32 block512s;

    u32 flags; /* Flags tell the implementation how to access this inode */
    u32 osd1;  /* OS-defined value (free for whatever) */

    // 15 32 bit block numbers that point to the blocks containing data for this inode
    // First 12 are direct blocks
    // 13th entry is singularly indirect
    // 14th entry is doubly indirect
    // 15th entry is triply indirect
    blocknum_t block[15];

    // Version of this file
    u32 generation;

    // Unused (set to 0) for revision 0 inode's
    // We can ignore anything related to "fragments" as well
    u32 file_acl;
    u32 dir_acl;
    u32 faddr;
    u8 osd2[12];
} inode_t;

END_C_HEADER

#endif // EXT2_INODE_H
