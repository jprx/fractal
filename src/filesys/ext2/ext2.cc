#include <fractal.h>
#include <filesys/tar.h>
#include <filesys/fileio.h>
#include <filesys/ext2.h>
#include <filesys/ext2/superblock.h>
#include <filesys/ext2/blockgroup.h>
#include <filesys/ext2/inode.h>
#include <filesys/ext2/directory.h>

#ifdef USE_EXT2

// REMEMBER: inode numbers start at 1, not 0.

// Given an index, right shift it by this amount before indexing into the block/ inode alloc bitmaps
#define INDEX_TO_BITMAP_SHIFT ((3))

// Given an index, & it with this mask to find the bit within a given field
#define INDEX_TO_BITMAP_MASK ((BITS_PER_U8 - 1))

extern virt_t global_initrd;

// The global block_size for this file system
static u64 block_size = 0;

// The global block group descriptor table
static ext2_bgd_t *global_bdt = NULL;
static usize global_num_blockgroups = 0;

static inline bool is_block_allocated(blocknum_t num);
static inline void set_block_allocated(blocknum_t num, bool allocd);
static inline bool is_inode_allocated(inodenum_t num);
static inline void set_inode_allocated(inodenum_t num, bool allocd);

static inline superblock_t *get_superblock() {
    return (superblock_t *)((usize)global_initrd + EXT2_SUPERBLOCK_OFFSET);
}

// This method is super non-portable for when we move to virtio/ disks
// Only works for initramfs/ fs in RAM
static inline void *blocknum_to_ptr(blocknum_t idx) {
    return (void *)(((usize)global_initrd) + (idx * block_size));
}

// Given an inode number, get which blockgroup it belongs to
// Recall inodes start at 1, not 0, so subtract 1 from inode number before doing indexing math
static inline usize inode_num_to_blockgroup(inodenum_t i) {
    if (0 == i) panic("inode 0 does not exist in EXT2");
    return floor_div((i - 1), get_superblock()->inodes_per_group);
}

// Given an indode number, get its index within a local blockgroup inode table
// Recall inodes start at 1, not 0, so subtract 1 from inode number before doing indexing math
static inline usize inode_num_to_local_idx(inodenum_t i) {
    if (0 == i) panic("inode 0 does not exist in EXT2");
    return ((i - 1) % (get_superblock()->inodes_per_group));
}

// Given an block number, get which blockgroup it belongs to
static inline usize block_num_to_blockgroup(blocknum_t i) {
    // On X86 this was suddenly becoming zero when I wasn't handling
    // end of directory listing correctly
    // printf("blocks per group: %d\n", get_superblock()->blocks_per_group);
    return floor_div(i, get_superblock()->blocks_per_group);
}

/*
 * inode_num_to_ptr
 * Given an inode number, returns a pointer to its inode struct in-memory.
 * Assumes a RAM FS- will need to be refactored to return a disk offset
 * if we move to using a disk driver instead.
 *
 * Returns the memory location, assuming an initrd, of this inode
 * Will have to be modified when we move to virtio block driver!
 */
static inline inode_t *inode_num_to_ptr(inodenum_t i) {
    if (i > get_superblock()->inode_count) {
        // >, not >=, because inode counts begin at 1
        panic("Requested inode ptr for inode %d, but there are only %d inodes in the system", i, get_superblock()->inode_count);
    }
    usize bgroup    = inode_num_to_blockgroup(i);
    usize local_idx = inode_num_to_local_idx(i);

    if (!is_inode_allocated(i)) {
        panic("Tried to lookup pointer for a freed inode (num %d)", i);
    }

    usize inode_table_offset = ((global_bdt[bgroup].inode_table) * block_size);
    inode_t *inode_table = (inode_t *)((usize)global_initrd + inode_table_offset);

    return &inode_table[local_idx];
}

static inline bool is_block_allocated(blocknum_t num) {
    usize blockgroup_idx = block_num_to_blockgroup(num);
    num -= (blockgroup_idx * get_superblock()->blocks_per_group);
    usize bitmap_block = global_bdt[blockgroup_idx].block_bitmap;
    u8 *bitmap = (u8 *)blocknum_to_ptr(bitmap_block);

    return BIT_AT(num & INDEX_TO_BITMAP_SHIFT,
        bitmap[num>>INDEX_TO_BITMAP_SHIFT]
    );
}

static inline void set_block_allocated(blocknum_t num, bool allocd) {
    usize blockgroup_idx = block_num_to_blockgroup(num);
    num -= (blockgroup_idx * get_superblock()->blocks_per_group);
    usize bitmap_block = global_bdt[blockgroup_idx].block_bitmap;
    u8 *bitmap = (u8 *)blocknum_to_ptr(bitmap_block);

    if (allocd) {
        bitmap[num >> INDEX_TO_BITMAP_SHIFT] |= BIT(num & INDEX_TO_BITMAP_MASK);
    } else {
        bitmap[num >> INDEX_TO_BITMAP_SHIFT] &= ~BIT(num & INDEX_TO_BITMAP_MASK);
    }
}

static inline bool is_inode_allocated(inodenum_t num) {
    usize blockgroup_idx = inode_num_to_blockgroup(num);
    num -= (blockgroup_idx * get_superblock()->inodes_per_group);
    usize bitmap_block = global_bdt[blockgroup_idx].inode_bitmap;
    u8 *bitmap = (u8 *)blocknum_to_ptr(bitmap_block);
    num -= 1; // inodes begin at 1
    return BIT_AT(num & INDEX_TO_BITMAP_SHIFT,
        bitmap[num>>INDEX_TO_BITMAP_SHIFT]
    );
}

static inline void set_inode_allocated(inodenum_t num, bool allocd) {
    usize blockgroup_idx = inode_num_to_blockgroup(num);
    num -= (blockgroup_idx * get_superblock()->inodes_per_group);
    usize bitmap_block = global_bdt[blockgroup_idx].inode_bitmap;
    u8 *bitmap = (u8 *)blocknum_to_ptr(bitmap_block);
    num -= 1; // inodes begin at 1
    if (allocd) {
        bitmap[num >> INDEX_TO_BITMAP_SHIFT] |= BIT(num & INDEX_TO_BITMAP_MASK);
    } else {
        bitmap[num >> INDEX_TO_BITMAP_SHIFT] &= ~BIT(num & INDEX_TO_BITMAP_MASK);
    }
}

static inline blocknum_t alloc_block_in_blockgroup(usize bgroup) {
    superblock_t *sb = get_superblock();
    for (usize i = 0; i < sb->blocks_per_group; i++) {
        blocknum_t block_idx_abs = i + (sb->blocks_per_group * bgroup);
        if (!is_block_allocated(block_idx_abs)) {
            set_block_allocated(block_idx_abs, true);
            u8 *new_block = (u8 *)blocknum_to_ptr(block_idx_abs);
            memset(new_block, '\x00', block_size);
            global_bdt[bgroup].num_free_blocks--;
            sb->free_block_count--;
            return block_idx_abs;
        }
    }
    panic("Out of memory: Could not allocate a block in blockgroup %d", bgroup);
}

static inline blocknum_t alloc_block() {
    for (usize i = 0; i < global_num_blockgroups; i++) {
        if (global_bdt[i].num_free_blocks > 0) {
            return alloc_block_in_blockgroup(i);
        }
    }
    panic("Failed to allocate a fs block- we must have run out of disk space");
}

static inline inodenum_t alloc_inode_in_blockgroup(usize bgroup) {
    superblock_t *sb = get_superblock();
    for (usize i = 0; i < sb->inodes_per_group; i++) {
        // inode numbers begin at 1
        blocknum_t inode_idx_abs = i + (sb->inodes_per_group * bgroup) + 1;
        if (!is_inode_allocated(inode_idx_abs)) {
            set_inode_allocated(inode_idx_abs, true);
            inode_t *ind = (inode_t *)inode_num_to_ptr(inode_idx_abs);
            memset(ind, '\x00', sizeof(*ind));
            global_bdt[bgroup].num_free_inodes--;
            sb->free_inode_count--;
            sb->inode_count++;
            return inode_idx_abs;
        }
    }
    panic("Out of memory: Could not allocate an inode in blockgroup %d", bgroup);
}

static inline inodenum_t alloc_inode() {
    for (usize i = 0; i < global_num_blockgroups; i++) {
        if (global_bdt[i].num_free_inodes > 0) {
            return alloc_inode_in_blockgroup(i);
        }
    }
    panic("Failed to allocate a fs inode- we must have run out of disk space");
}

// Returns the block number to read from for location in inode
static inline blocknum_t get_blocknum(inode_t *inode, usize offset_into_file) {
    if (offset_into_file > inode->size) return 0;
    isize block_idx = floor_div(offset_into_file, block_size);

    // Figure out which bin we fall into
    // After each bin, re-normalize block_idx so that 0 is the first block in that region
    // We do that by subtracting the number of blocks we have passed by from block_idx.
    if (block_idx < INODE_DIRECT_REGION_NUMBLOCKS(block_size)) {
        if (block_idx < 0) panic("negative block_idx (sanity check)");
        return inode->block[block_idx];
    }

    block_idx -= INODE_DIRECT_REGION_NUMBLOCKS(block_size);
    if (block_idx < 0) panic("negative block_idx (sanity check)");

    if (block_idx < INODE_SINGLY_INDIRECT_REGION_NUMBLOCKS(block_size)) {
        if (0 == inode->block[INODE_BLOCK_SINGLY_INDIRECT]) return 0;
        blocknum_t *level1_blocktable = (blocknum_t *)blocknum_to_ptr(inode->block[INODE_BLOCK_SINGLY_INDIRECT]);
        isize index_level1 = block_idx % INODE_N_BLOCKS_PER_TABLE(block_size);
        if (index_level1 >= INODE_N_BLOCKS_PER_TABLE(block_size)) panic("index oob (sanity check)");
        return level1_blocktable[index_level1];
    }

    block_idx -= INODE_SINGLY_INDIRECT_REGION_NUMBLOCKS(block_size);
    if (block_idx < 0) panic("negative block_idx (sanity check)");

    if (block_idx < INODE_DOUBLY_INDIRECT_REGION_NUMBLOCKS(block_size)) {
        // Blocks 0 -> (block_size / sizeof(u32))-1 are in the first entry
        // Blocks (block_size / sizeof(u32)) -> 2*(block_size / sizeof(u32))-1 are in the second
        // and so on
        if (0 == inode->block[INODE_BLOCK_DOUBLY_INDIRECT]) return 0;
        blocknum_t *level1_blocktable = (blocknum_t *)blocknum_to_ptr(inode->block[INODE_BLOCK_DOUBLY_INDIRECT]);
        isize index_level1 = floor_div(block_idx, INODE_N_BLOCKS_PER_TABLE(block_size));
        if (index_level1 >= INODE_N_BLOCKS_PER_TABLE(block_size) || index_level1 < 0) panic("index oob (sanity check)");
        if (0 == level1_blocktable[index_level1]) return 0;
        blocknum_t *level2_blocktable = (blocknum_t *)blocknum_to_ptr(level1_blocktable[index_level1]);
        isize index_level2 = block_idx % INODE_N_BLOCKS_PER_TABLE(block_size);
        if (index_level2 >= INODE_N_BLOCKS_PER_TABLE(block_size) || index_level2 < 0) panic("index oob (sanity check)");
        return level2_blocktable[index_level2];
    }

    block_idx -= INODE_DOUBLY_INDIRECT_REGION_NUMBLOCKS(block_size);
    if (block_idx < 0) panic("negative block_idx (sanity check)");

    if (block_idx < INODE_TRIPLY_INDIRECT_REGION_NUMBLOCKS(block_size)) {
        // let z = (block_size / sizeof(u32))
        // z is also known as INODE_N_BLOCKS_PER_TABLE
        // Blocks 0 -> z^2 are in the first entry
        // z^2 -> 2z^2 are in the second
        // and so on
        if (0 == inode->block[INODE_BLOCK_TRIPLY_INDIRECT]) return 0;
        blocknum_t *level1_blocktable = (blocknum_t *)blocknum_to_ptr(inode->block[INODE_BLOCK_TRIPLY_INDIRECT]);
        isize index_level1 = floor_div(block_idx, INODE_N_BLOCKS_PER_TABLE(block_size) * INODE_N_BLOCKS_PER_TABLE(block_size));

        // Being in the second level table, we have traversed idx * z^2 blocks
        // subtract that out and we are back at the doubly indirect case
        block_idx -= index_level1 * INODE_N_BLOCKS_PER_TABLE(block_size) * INODE_N_BLOCKS_PER_TABLE(block_size);
        if (block_idx < 0) panic("negative block_idx (sanity check)");

        if (index_level1 >= INODE_N_BLOCKS_PER_TABLE(block_size) || index_level1 < 0) panic("index oob (sanity check)");
        if (0 == level1_blocktable[index_level1]) return 0;
        blocknum_t *level2_blocktable = (blocknum_t *)blocknum_to_ptr(level1_blocktable[index_level1]);
        isize index_level2 = floor_div(block_idx, INODE_N_BLOCKS_PER_TABLE(block_size));

        if (index_level2 >= INODE_N_BLOCKS_PER_TABLE(block_size) || index_level2 < 0) panic("index oob (sanity check)");
        if (0 == level2_blocktable[index_level2]) return 0;
        blocknum_t *level3_blocktable = (blocknum_t *)blocknum_to_ptr(level2_blocktable[index_level2]);

        isize index_level3 = block_idx % INODE_N_BLOCKS_PER_TABLE(block_size);
        if (index_level3 >= INODE_N_BLOCKS_PER_TABLE(block_size) || index_level3 < 0) panic("index oob (sanity check)");
        return level3_blocktable[index_level3];
    }

    panic("Offset 0x%X is too large for this filesystem (blocksize is %d bytes)", offset_into_file, block_size);
}

// Allocates a block at a given offset into an inode
static inline void alloc_block_at_offset(inode_t *inode, usize offset_into_file) {
    isize block_idx = floor_div(offset_into_file, block_size);

    // Figure out which bin we fall into
    // After each bin, re-normalize block_idx so that 0 is the first block in that region
    // We do that by subtracting the number of blocks we have passed by from block_idx.
    if (block_idx < INODE_DIRECT_REGION_NUMBLOCKS(block_size)) {
        if (block_idx < 0) panic("negative block_idx (sanity check)");
        if (0 == inode->block[block_idx]) inode->block[block_idx] = alloc_block();
        return;
    }

    block_idx -= INODE_DIRECT_REGION_NUMBLOCKS(block_size);
    if (block_idx < 0) panic("negative block_idx (sanity check)");

    if (block_idx < INODE_SINGLY_INDIRECT_REGION_NUMBLOCKS(block_size)) {
        if (0 == inode->block[INODE_BLOCK_SINGLY_INDIRECT]) inode->block[INODE_BLOCK_SINGLY_INDIRECT] = alloc_block();
        blocknum_t *level1_blocktable = (blocknum_t *)blocknum_to_ptr(inode->block[INODE_BLOCK_SINGLY_INDIRECT]);
        isize index_level1 = block_idx % INODE_N_BLOCKS_PER_TABLE(block_size);
        if (index_level1 >= INODE_N_BLOCKS_PER_TABLE(block_size)) panic("index oob (sanity check)");
        if (0 == level1_blocktable[index_level1]) level1_blocktable[index_level1] = alloc_block();
        return;
    }

    block_idx -= INODE_SINGLY_INDIRECT_REGION_NUMBLOCKS(block_size);
    if (block_idx < 0) panic("negative block_idx (sanity check)");

    if (block_idx < INODE_DOUBLY_INDIRECT_REGION_NUMBLOCKS(block_size)) {
        // Blocks 0 -> (block_size / sizeof(u32))-1 are in the first entry
        // Blocks (block_size / sizeof(u32)) -> 2*(block_size / sizeof(u32))-1 are in the second
        // and so on
        if (0 == inode->block[INODE_BLOCK_DOUBLY_INDIRECT]) inode->block[INODE_BLOCK_DOUBLY_INDIRECT] = alloc_block();
        blocknum_t *level1_blocktable = (blocknum_t *)blocknum_to_ptr(inode->block[INODE_BLOCK_DOUBLY_INDIRECT]);
        isize index_level1 = floor_div(block_idx, INODE_N_BLOCKS_PER_TABLE(block_size));
        if (index_level1 >= INODE_N_BLOCKS_PER_TABLE(block_size) || index_level1 < 0) panic("index oob (sanity check)");
        if (0 == level1_blocktable[index_level1]) level1_blocktable[index_level1] = alloc_block();
        blocknum_t *level2_blocktable = (blocknum_t *)blocknum_to_ptr(level1_blocktable[index_level1]);
        isize index_level2 = block_idx % INODE_N_BLOCKS_PER_TABLE(block_size);
        if (index_level2 >= INODE_N_BLOCKS_PER_TABLE(block_size) || index_level2 < 0) panic("index oob (sanity check)");
        if (0 == level2_blocktable[index_level2]) level2_blocktable[index_level2] = alloc_block();
        return;
    }

    block_idx -= INODE_DOUBLY_INDIRECT_REGION_NUMBLOCKS(block_size);
    if (block_idx < 0) panic("negative block_idx (sanity check)");

    if (block_idx < INODE_TRIPLY_INDIRECT_REGION_NUMBLOCKS(block_size)) {
        // let z = (block_size / sizeof(u32))
        // z is also known as INODE_N_BLOCKS_PER_TABLE
        // Blocks 0 -> z^2 are in the first entry
        // z^2 -> 2z^2 are in the second
        // and so on
        if (0 == inode->block[INODE_BLOCK_TRIPLY_INDIRECT]) inode->block[INODE_BLOCK_TRIPLY_INDIRECT] = alloc_block();
        blocknum_t *level1_blocktable = (blocknum_t *)blocknum_to_ptr(inode->block[INODE_BLOCK_TRIPLY_INDIRECT]);
        isize index_level1 = floor_div(block_idx, INODE_N_BLOCKS_PER_TABLE(block_size) * INODE_N_BLOCKS_PER_TABLE(block_size));

        // Being in the second level table, we have traversed idx * z^2 blocks
        // subtract that out and we are back at the doubly indirect case
        block_idx -= index_level1 * INODE_N_BLOCKS_PER_TABLE(block_size) * INODE_N_BLOCKS_PER_TABLE(block_size);
        if (block_idx < 0) panic("negative block_idx (sanity check)");

        if (index_level1 >= INODE_N_BLOCKS_PER_TABLE(block_size) || index_level1 < 0) panic("index oob (sanity check)");
        if (0 == level1_blocktable[index_level1]) level1_blocktable[index_level1] = alloc_block();
        blocknum_t *level2_blocktable = (blocknum_t *)blocknum_to_ptr(level1_blocktable[index_level1]);
        isize index_level2 = floor_div(block_idx, INODE_N_BLOCKS_PER_TABLE(block_size));

        if (index_level2 >= INODE_N_BLOCKS_PER_TABLE(block_size) || index_level2 < 0) panic("index oob (sanity check)");
        if (0 == level2_blocktable[index_level2]) level2_blocktable[index_level2] = alloc_block();
        blocknum_t *level3_blocktable = (blocknum_t *)blocknum_to_ptr(level2_blocktable[index_level2]);

        isize index_level3 = block_idx % INODE_N_BLOCKS_PER_TABLE(block_size);
        if (index_level3 >= INODE_N_BLOCKS_PER_TABLE(block_size) || index_level3 < 0) panic("index oob (sanity check)");
        if (0 == level3_blocktable[index_level3]) level3_blocktable[index_level3] = alloc_block();
        return;
    }

    panic("Offset 0x%X is too large for this filesystem (blocksize is %d bytes)", offset_into_file, block_size);
}

/*
 * read_from_inode
 * Reads sz bytes into buf from inode starting at offset off.
 * Returns number of bytes read.
 */
usize read_from_inode(inode_t *inode, u8 *buf, usize sz, usize off) {
    if (!inode) return 0;
    if (off >= inode->size) return 0;
    if (sz + off > inode->size) {
        sz = inode->size - off;
        // panic("Requested byte number %d but file is only %d byes large", sz + off, inode->size);
    }

    usize bytes_read_so_far = 0;
    usize bytes_remaining = sz;
    bool should_read_zeros = false;

    // Only (maybe) nonzero offset into block for first block
    usize offset_into_block = off % block_size;

    while (bytes_remaining > 0) {
        usize bytes_to_copy = min(block_size - offset_into_block, bytes_remaining);

        blocknum_t block_to_read_from = get_blocknum(inode, bytes_read_so_far + off);

        if (!is_block_allocated(block_to_read_from)) {
            panic("Tried to read from free block (%d, %d read, %d left)", block_to_read_from, bytes_read_so_far, bytes_remaining);
        }

        should_read_zeros = 0 == block_to_read_from;

        if (bytes_read_so_far + bytes_to_copy > sz) {
            panic("read_from_inode was about to overflow the buffer");
        }

        if (!should_read_zeros) {
            u8 *block_ptr = (u8 *)blocknum_to_ptr(block_to_read_from);
            if (NULL == block_ptr) panic("NULL block pointer");

            memcpy(
                &buf[bytes_read_so_far],
                (u8 *)((usize)blocknum_to_ptr(block_to_read_from) + offset_into_block),
                bytes_to_copy
            );
        } else {
            memset(
                &buf[bytes_read_so_far],
                '\x00',
                bytes_to_copy
            );
        }

        bytes_read_so_far += bytes_to_copy;
        bytes_remaining -= bytes_to_copy;
        offset_into_block = 0;
    }

    return bytes_read_so_far;
}

/*
 * write_to_inode
 * Writes sz bytes from buf into inode starting at offset off.
 * Returns number of bytes written.
 */
usize write_to_inode(inode_t *inode, u8 *buf, usize sz, usize off) {
    if (!inode) return 0;

    // We will allocate enough blocks to accomodate this, so just bump the size up now
    // @TODO: Undo size increase if the filesystem cannot handle it
    if (inode->size < sz + off) {
        inode->size = sz + off;
    }

    usize bytes_written_so_far = 0;
    usize bytes_remaining = sz;

    // Only (maybe) nonzero offset into block for first block
    usize offset_into_block = off % block_size;

    while (bytes_remaining > 0) {
        usize bytes_to_copy = min(block_size - offset_into_block, bytes_remaining);
        blocknum_t block_to_write_to = 0;

        // Lookup block, if it isn't present, re-allocate and try again
        block_to_write_to = get_blocknum(inode, bytes_written_so_far + off);
        if (!is_block_allocated(block_to_write_to) || 0 == block_to_write_to) {
            alloc_block_at_offset(inode, bytes_written_so_far + off);
            block_to_write_to = get_blocknum(inode, bytes_written_so_far + off);
        }

        // If block really isn't present, handle error condition (out of memory)
        if (!is_block_allocated(block_to_write_to) || 0 == block_to_write_to) {
            panic("Tried to alloc more space for inode 0x%X (block %d), but somehow it didn't work", inode, block_to_write_to);
        }

        if (bytes_written_so_far + bytes_to_copy > sz) {
            panic("write_to_inode was about to overflow the buffer");
        }

        u8 *block_ptr = (u8 *)blocknum_to_ptr(block_to_write_to);
        if (NULL == block_ptr) panic("NULL block pointer");

        memcpy(
            (u8 *)((usize)blocknum_to_ptr(block_to_write_to) + offset_into_block),
            &buf[bytes_written_so_far],
            bytes_to_copy
        );

        bytes_written_so_far += bytes_to_copy;
        bytes_remaining -= bytes_to_copy;
        offset_into_block = 0;
    }

    return bytes_written_so_far;
}

// Recursively dump every file in the system
void dump_inode_dir(inode_t *ind) {
    char namebuf[MAX_FILENAME_LEN+1];
    ext2_dir_t tmp;
    ext2_dir_t *readbuf = &tmp;
    usize cursor = 0;
    while(true) {
        read_from_inode(ind, (u8 *)readbuf, sizeof(ext2_dir_t), cursor);
        if (readbuf->inode == 0) break;
        cursor += readbuf->rec_len;

        if (strequal(".", readbuf->name) || strequal("..", readbuf->name)) continue;
        memcpy(namebuf, readbuf->name, readbuf->name_len);
        namebuf[readbuf->name_len] = '\x00';

        printf("Found %s at offset 0x%X\r\n",  namebuf, cursor - readbuf->rec_len);

        inode_t *file_ind = inode_num_to_ptr(readbuf->inode);
        printf("%s (%d bytes)\r\n", namebuf, file_ind->size);

        if (file_ind->mode & INODE_MODE_DIR) {
            if (!strequal(".", readbuf->name) && !strequal("..", readbuf->name)) {
                dump_inode_dir(file_ind);
            }
        }

        if (cursor >= ind->size) break;
    }
    printf("done\n");
}

void dump_inode(inode_t *i) {
    printf("-- Dumping inode 0x%X --\n", i);
    printf("Type: ");
    if (i->mode & INODE_MODE_DIR)     printf("dir\n");
    if (i->mode & INODE_MODE_REGULAR) printf("reg\n");

    printf("Size: %d bytes\n", i->size);

    if (i->mode & INODE_MODE_DIR) dump_inode_dir(i);
}

void dump_inode_table(ext2_bgd_t *group) {
    printf("inode table is at %d\n", group->inode_table);
    usize inode_table_offset = (group->inode_table * block_size);
    inode_t *inode_table = (inode_t *)((usize)global_initrd + inode_table_offset);
}

// Scan a directory inode for a inode with name fname- returns inode num
inodenum_t _find_inode_in_dir_as_inodenum(inode_t *indir, char *fname) {
    char namebuf[MAX_FILENAME_LEN+1];
    ext2_dir_t readbuf;
    usize cursor = 0;
    usize bytes_read = 0;

    if (!indir->mode & INODE_MODE_DIR) {
        panic("Tried to search a non-directory inode for a file named %s", fname);
    }

    while(true) {
        bytes_read = read_from_inode(indir, (u8 *)&readbuf, sizeof(ext2_dir_t), cursor);
        if (0 == bytes_read) break;
        if (bytes_read < EXT2_MIN_DIR_ENTRY_LEN) panic("Failed to read a full ext2_dir_t");
        if (readbuf.inode == 0) break;
        if (0 == readbuf.rec_len) break;
        cursor += readbuf.rec_len;

        memcpy(namebuf, readbuf.name, readbuf.name_len);
        namebuf[readbuf.name_len] = '\x00';

        if (strequal(namebuf, fname)) return readbuf.inode;
    }

    return 0; // 0 is an invalid inode number
}

// Scan a directory inode for a inode with name fname
// Returns pointer to inode (rather than its inode number)
// (to get the inode num, use _find_inode_in_dir_as_inodenum)
inode_t *find_inode_in_dir(inode_t *indir, char *fname) {
    inodenum_t n = _find_inode_in_dir_as_inodenum(indir, fname);
    if (0 == n) return NULL;
    return inode_num_to_ptr(n);
}

// Read a libc_dirent_t from offset bytes into directory inode h
// Returns number of bytes read from the inode
// Sets reclen of the libc_dirent buf to be correct for libc, but this
// reclen will probably be larger than the number of bytes read from the fs
// File descriptor offset should be incremented by the val returned by this fn
usize filesys_readdir(void *h, libc_dirent_t *buf, usize offset) {
    usize bytes_read = 0;
    ext2_dir_t readbuf;
    readbuf.inode = 0;

    bytes_read = read_from_inode((inode_t *)h, (u8 *)&readbuf, sizeof(readbuf), offset);
    if (0 == bytes_read) return 0;
    if (bytes_read < EXT2_MIN_DIR_ENTRY_LEN) return 0;
    if (0 == readbuf.inode) return 0;

    if (readbuf.name_len > DIRENT_NAME_MAX) panic("Filesystem directory name too long");

    buf->d_fileno = readbuf.inode;
    buf->d_off = offset;
    buf->d_reclen = sizeof(*buf) - sizeof(buf->d_name) + readbuf.name_len;
    buf->d_namelen = readbuf.name_len;
    strncpy(buf->d_name, readbuf.name, DIRENT_NAME_MAX);

    if (filesys_is_dir(inode_num_to_ptr(readbuf.inode)))       { buf->d_type = DT_DIR; }
    else if (filesys_is_file(inode_num_to_ptr(readbuf.inode))) { buf->d_type = DT_REG; }
    else { buf->d_type = DT_UNKNOWN; panic("DT UNKNOWN"); }

    return readbuf.rec_len;
}

// Create a new directory entry the end of this directory's inode list pointing to new_inode_idx
// Returns true on success, false on failure
bool place_inode_in_dir(inode_t *dir_inode, char *new_inode_name, inodenum_t new_inode_idx) {
    char namebuf[MAX_FILENAME_LEN+1];
    ext2_dir_t readbuf;
    usize cursor = 0;
    usize bytes_read = 0;

    if (strlen(new_inode_name) > MAX_FILENAME_LEN) return NULL;

    if (!dir_inode->mode & INODE_MODE_DIR) {
        panic("Tried to create file %s in a non-directory inode", new_inode_name);
    }

    while(true) {
        bytes_read = read_from_inode(dir_inode, (u8 *)&readbuf, sizeof(ext2_dir_t), cursor);
        if (0 == bytes_read) panic("Failed to create file %s (1)", new_inode_name);
        if (bytes_read < EXT2_MIN_DIR_ENTRY_LEN) panic("Failed to read a full ext2_dir_t");
        if (0 == readbuf.inode) {
            panic ("Failed to create inode %s (2)", new_inode_name);
        }

        if (readbuf.rec_len + cursor >= dir_inode->size) {
            // This is the end of the list- make new inode here

            // Update previous record's rec_len
            // Ensure new inode can fit in block
            // Write it into block, or allocate a new block and copy it over
            // Make sure to keep directory entries 4-byte aligned

            // Round up to nearest 4-byte alignment, skipping ahead
            // 1 directory entry minus the total name length, plus the length
            // of the previous record
            usize offset_for_new_inode = (cursor + (sizeof(ext2_dir_t) - EXT2_DIR_NAMELEN + readbuf.name_len) + 0x10) & ~0x0F;

            // If we actually care about full spec compliance, make sure no ext2_dir_t's cross a block:
#if CARE_ABOUT_COMPLYING_WITH_EXT2_SPEC
            if (offset_for_new_inode + sizeof(ext2_dir_t) + strlen(new_inode_name) - cursor > block_size) {
                // @TODO: Handle this case, as necessary
                panic("This allocation would cross two blocks, which is forbidden by the spec");
            }
#endif // CARE_ABOUT_COMPLYING_WITH_EXT2_SPEC

            readbuf.rec_len = offset_for_new_inode - cursor;
            write_to_inode(
                dir_inode,
                (u8 *)&readbuf,
                sizeof(ext2_dir_t) - EXT2_DIR_NAMELEN + readbuf.name_len,
                cursor
            );

            // Populate new ext2_dir_t (ignoring file_type)
            memset(&readbuf, '\x00', sizeof(ext2_dir_t));
            readbuf.inode = new_inode_idx;
            readbuf.name_len = strlen(new_inode_name);
            strncpy(readbuf.name, new_inode_name, sizeof(readbuf.name));

            // Special marker to tell that the directory ends here:
            // Just use a large value that goes beyond the bounds of the directory inode
            readbuf.rec_len = dir_inode->size + 1;

            write_to_inode(
                dir_inode,
                (u8*)&readbuf,
                sizeof(ext2_dir_t) - EXT2_DIR_NAMELEN + readbuf.name_len,
                offset_for_new_inode
            );

            return true;
        }

        cursor += readbuf.rec_len;

        memcpy(namebuf, readbuf.name, readbuf.name_len);
        namebuf[readbuf.name_len] = '\x00';

        inode_t *file_ind = inode_num_to_ptr(readbuf.inode);
        if (strequal(namebuf, new_inode_name)) return false;
    }

    return false;
}

extern "C" void fs_init() {
    superblock_t *sb = get_superblock();
#ifndef QUIET_BOOT
    printf("superblock is at 0x%X\n", (u64)sb);
#endif // !QUIET_BOOT
    block_size = EXT2_BLOCK_LOG_SIZE << sb->log_block_size;
    global_num_blockgroups = max(
        ceil_div(sb->block_count, sb->blocks_per_group),
        ceil_div(sb->inode_count, sb->inodes_per_group)
    );

    usize block_group_table_offset = ((sb->first_data_block + 1) * block_size);
    global_bdt = (ext2_bgd_t *)((usize)global_initrd + block_group_table_offset);
}

usize filesys_filelen(void *resource) {
    return ((inode_t *)resource)->size;
}

// Returns pointer to just the base filename of this file
extern "C" char *filesys_basename(char *path) {
    char *cursor = (char *)((usize)path + strlen(path));

    // path-1 to handle case where path is of the form "/file.txt"
    while (cursor != path-1) {
        if (*cursor == '/') return cursor+1;
        cursor--;
    }

    return path;
}

// Copies the directory name from path into outs, eg.
// everything but the basename
extern "C" void filesys_dirname(char *outs, char *path, usize outs_sz) {
    strncpy(outs, path, outs_sz);
    char *basename = filesys_basename(outs);
    *basename = '\x00';
}

/*
 * split_path
 * Given a path like "a/b/c/d.txt", returns "a".
 * Given a path like "file.txt", returns "file.txt".
 * Given a path like "/a/b/c.txt", returns "a".
 * Given a path like "/a.txt", returns "a.txt".
 *
 * Essentially, this strips any leading slashes, and returns
 * everything until the next slash encountered.
 *
 * Returns true if we encountered a slash (this is a directory),
 * and false if we didn't (this is the end of the string).
 *
 * Updates path_in to point to the next slash encountered,
 * or to the NULL terminator if none.
 */
bool split_path(char *outs, char **path_in, usize outsz) {
    while ((*path_in)[0] == '/') (*path_in)++;
    strncpy(outs, *path_in, outsz);
    char *cursor = outs;
    while (*cursor) {
        if (*cursor == '/') {
            *cursor = '\x00';
            *path_in += (cursor - outs);
            return true;
        }
        cursor++;
    }
    return false;
}

/*
 * filesys_open_absolute
 * Locate and return a pointer to the inode for a file f.
 * If no such file exists, return NULL.
 *
 * This requires an absolute path to the resource-
 * callers should append the current working directory
 * to any requests prior to calling this method.
 */
void *filesys_open_absolute(char *path) {
    if (path[0] != '/') return NULL;
    return filesys_open_relative(path, inode_num_to_ptr(EXT2_ROOT_INODE_NUM));
}

/*
 * _filesys_open_relative
 * Internal method that returns the
 * inode number of a resource, instead
 * of a handle to it (its inode_t ptr).
 */
inodenum_t _filesys_open_relative(char *path, void *cur_dir) {
    char pathcpy[MAX_FILENAME_LEN];
    char *path_top = pathcpy;
    char cur_fname[MAX_FILENAME_LEN];

    inodenum_t cur_inodenum = 0;

    if (NULL == cur_dir || path[0] == '/') {
        cur_dir = inode_num_to_ptr(EXT2_ROOT_INODE_NUM);
        cur_inodenum = EXT2_ROOT_INODE_NUM;
    }

    if (strequal("", path)) {
        panic("This should return cur_dir's inode, but we currently take cur_dir as an inode pointer. TODO: refactor this to take an inode number instead of a pointer as the argument (I don't want to do that right now though- lazy)");
    }

    inode_t *cur_inode = (inode_t *)cur_dir;

    strncpy(pathcpy, path, MAX_FILENAME_LEN);
    while (split_path(cur_fname, (char **)&path_top, MAX_FILENAME_LEN)) {
        // Need to find cur_fname in the cur_inode directory
        cur_inodenum = _find_inode_in_dir_as_inodenum(cur_inode, cur_fname);
        if (0 == cur_inodenum) return 0; // 0 is an invalid inode number
        cur_inode = inode_num_to_ptr(cur_inodenum);
    }

    if (cur_fname[0] == '\x00') {
        // f ended in '/', which means we wanted a directory, so
        // we found the file already- proceeding from here would
        // search for the empty string as a child
        // of the current directory.
        return cur_inodenum;
    }

    // cur_inode now points to the directory in which we can find the file
    // Check if the file exists
    cur_inodenum = _find_inode_in_dir_as_inodenum(cur_inode, cur_fname);
    return cur_inodenum;
}

/*
 * filesys_open_relative
 * Open a resource relative to another one.
 * Acts like filesys_open_absolute if the first character is '/' or cur_dir is NULL.
 * Otherwise this returns the path-relative file from cur_dir at path.
 */
void *filesys_open_relative(char *path, void *cur_dir) {
    inodenum_t result_inode = _filesys_open_relative(path, cur_dir);
    if (0 == result_inode) return NULL;
    return inode_num_to_ptr(result_inode);
}

// Get the inode number of a resource rather than a handle to it
// Usually this is very much not what you want to do.
inodenum_t filesys_open_inodenum_relative(char *path, void *cur_dir) {
    return _filesys_open_relative(path, cur_dir);
}

/*
 * _filesys_create_internal
 * Creates a new file at path "f".
 * Assumes path is of the form "/path/to/file"
 *
 * Returns a pointer to the inode if successful, false otherwise
 * (eg. file already exists, or couldn't create the path, or something).
 */
inodenum_t _filesys_create_internal(char *filename_in, void *cur_dir, filekind_t kind) {
    char dirname[MAX_FILENAME_LEN];
    char *basename = filesys_basename(filename_in);
    filesys_dirname(dirname, filename_in, MAX_FILENAME_LEN);
    inode_t *dir_inode = NULL;

    // If there is no directory at all (eg no '/' in the path), use cur_dir
    if (strequal("", dirname)) {
        dir_inode = (inode_t *)cur_dir;
    } else {
        dir_inode = (inode_t *)filesys_open_relative(dirname, cur_dir);
    }

    if (!dir_inode) {
        return 0;
    }

    inodenum_t new_inode_idx = alloc_inode();
    inode_t *new_inode = inode_num_to_ptr(new_inode_idx);

    switch(kind) {
        case FILE_NONE:
        panic("Tried to create a FILE_NONE inode at %s", filename_in);
        break;

        case FILE_REGULAR:
        new_inode->mode = INODE_MODE_REGULAR;
        break;

        case FILE_DIRECTORY:
        new_inode->mode = INODE_MODE_DIR;
        break;
    }

    place_inode_in_dir(dir_inode, basename, new_inode_idx);
    return new_inode_idx;
}

void *filesys_create(char *filename_in, void *cur_dir, filekind_t kind) {
    inodenum_t new_inode_num = _filesys_create_internal(filename_in, cur_dir, kind);
    if (0 == new_inode_num) return NULL;
    return inode_num_to_ptr(new_inode_num);
}

bool filesys_mkdir(char *path, void *cur_dir) {
    // new_inode_num is the newly created file that will be the directory
    inodenum_t new_inode_num = _filesys_create_internal(path, cur_dir, FILE_DIRECTORY);
    if (0 == new_inode_num) return false;

    inode_t *new_dir = inode_num_to_ptr(new_inode_num);

    // Create . and ..
    ext2_dir_t dirbuf = { 0 };
    dirbuf.inode = new_inode_num;
    dirbuf.name_len = strlen(".");
    strncpy((char *)&dirbuf.name, ".", sizeof(dirbuf.name));
    // Something well beyonds the end of the file
    dirbuf.rec_len = block_size;
    write_to_inode(
        new_dir,
        (u8 *)&dirbuf,
        dirbuf.rec_len,
        0
    );

    return true;
}

// Remove the given inode from its directory so it won't be found again.
// Returns true on success, false on failure
bool remove_inode_from_dir(inode_t *dir_inode, inodenum_t unlink_inodenum, char *unlink_basename) {
    char namebuf[MAX_FILENAME_LEN+1];
    ext2_dir_t readbuf;
    usize cursor = 0;
    usize prev_cursor = 0;
    bool found_inode = false;
    usize bytes_read = 0;

    if (!dir_inode->mode & INODE_MODE_DIR) {
        panic("Tried to unlink from a non-directory inode");
    }

    while(!found_inode) {
        bytes_read = read_from_inode(dir_inode, (u8 *)&readbuf, sizeof(ext2_dir_t), cursor);
        if (0 == bytes_read) panic("Failed to unlink (1)");
        if (0 == readbuf.inode) panic ("Failed to unlink (2)");

        if (cursor >= dir_inode->size) {
            // This is the end of the list- we failed to find the right thing
            // Given the checks in filesys_unlink, this shouldn't be possible, so we panic.
            panic("unlink failed to find the file (3)");
            return false;
        }

        // Check if this record is the one to remove
        // For now, we do a sanity check by comparing both the inode number and file name
        memcpy(namebuf, readbuf.name, readbuf.name_len);
        namebuf[readbuf.name_len] = '\x00';

        if (strequal(namebuf, unlink_basename) && readbuf.inode != unlink_inodenum) {
            panic("unlink found the file, but the inode number and name don't match");
        }

        if (readbuf.inode == unlink_inodenum) {
            found_inode = true;
            break;
        } else {
            // Skip to next record
            prev_cursor = cursor;
            cursor += readbuf.rec_len;
        }
    }

    // cursor now points to the inode to remove
    // for every inode in this directory after it, shift it back by the length
    // of the record currently in cursor.
    // Then, we will set the rec_len of the last record to point beyond dir_inode->size to mark the end of the linked list.
    usize shift_len = sizeof(ext2_dir_t) - EXT2_DIR_NAMELEN + readbuf.name_len;

    // Move to next record (first one to shift back)
    // Note that we traverse the cursor using the record length, NOT the shift length
    // Shift length is how large the record to remove actually is; rec_len really
    // just points to the next thing in the linked list
    // if this is the end of the list, doing this shift will set cursor to be larger than dir_inode->size.
    // Do NOT increment prev_cursor to be cursor, because this is the element we are removing
    cursor += readbuf.rec_len;

    while(cursor < dir_inode->size) {
        bytes_read = read_from_inode(dir_inode, (u8*)&readbuf, sizeof(ext2_dir_t), cursor);
        if (0 == bytes_read) panic("Failed to unlink (3)");
        if (bytes_read < EXT2_MIN_DIR_ENTRY_LEN) panic("Failed to read a full ext2_dir_t");
        if (0 == readbuf.inode) {
            panic ("Failed to unlink (4)");
        }

#ifdef DEBUG_UNLINK
        printf("\t\tShifting %s\n", readbuf.name);
#endif // DEBUG_UNLINK

        write_to_inode(dir_inode,
            (u8 *)&readbuf,
            sizeof(ext2_dir_t) - EXT2_DIR_NAMELEN + readbuf.name_len,
            cursor - shift_len
        );

        prev_cursor = cursor;
        cursor += readbuf.rec_len;
#ifdef DEBUG_UNLINK
        printf("\t\tinc by %d\n", readbuf.rec_len);
#endif // DEBUG_UNLINK
    }

    dir_inode->size -= shift_len;

    // Set rec_len of the next cursor to be larger than dir_inode->size
    read_from_inode(dir_inode, (u8 *)&readbuf, sizeof(readbuf), prev_cursor);
    readbuf.rec_len = dir_inode->size - prev_cursor + 1;
    write_to_inode(dir_inode, (u8 *)&readbuf, sizeof(ext2_dir_t) - EXT2_DIR_NAMELEN + readbuf.name_len, prev_cursor);

    // @TODO: Free up any blocks that may have been released by removing dentries
    // For now, directory entries only grow in number of blocks, but never shrink.

    return true;
}

int filesys_unlink(char *filename_in, void *cur_dir) {
    if (!filename_in) return -1;
    if (!cur_dir) return -1;

    char dirname[MAX_FILENAME_LEN];
    char *basename = filesys_basename(filename_in);
    filesys_dirname(dirname, filename_in, MAX_FILENAME_LEN);
    inode_t *dir_inode = NULL;

    // If there is no directory at all (eg no '/' in the path), use cur_dir
    if (strequal("", dirname)) {
        dir_inode = (inode_t *)cur_dir;
    } else {
        dir_inode = (inode_t *)filesys_open_relative(dirname, cur_dir);
    }

    if (!dir_inode) {
        // File not found?
        return -1;
    }

    inodenum_t unlink_inodenum = filesys_open_inodenum_relative(basename, cur_dir);
    if (0 == unlink_inodenum) return -1;

    remove_inode_from_dir(dir_inode, unlink_inodenum, basename);

    // @TODO: Clean up blocks from this file, rather than just deleting its dentry.

    return 0;
}

void *filesys_root_resource() {
    return inode_num_to_ptr(EXT2_ROOT_INODE_NUM);
}

usize filesys_read(void *h, u8 *buf, usize n_bytes, usize offset) {
    return read_from_inode((inode_t *)h, buf, n_bytes, offset);
}

usize filesys_write(void *h, u8 *buf, usize n_bytes, usize offset) {
    return write_to_inode((inode_t *)h, buf, n_bytes, offset);
}

bool filesys_is_file(void *h) {
    return ((inode_t *)h)->mode & INODE_MODE_REGULAR;
}

bool filesys_is_dir(void *h) {
    return ((inode_t *)h)->mode & INODE_MODE_DIR;
}

void filesys_close(void *resource) {
    // Nothing to do (yet)
    return;
}

bool filesys_exists(char *path, void *cur_dir) {
    return filesys_open_relative(path, cur_dir) != NULL;
}

#endif // USE_EXT2
