#ifndef EXT2_DIRECTORY_H
#define EXT2_DIRECTORY_H

#include <fractal.h>

BEGIN_C_HEADER

typedef enum {
    EXT2_FT_UNKNOWN = 0,
    EXT2_FT_REGULAR = 1,
    EXT2_FT_DIR     = 2,
    EXT2_FT_CHRDEV  = 3,
    EXT2_FT_BLKDEV  = 4,
    EXT2_FT_FIFO    = 5,
    EXT2_FT_SOCK    = 6,
    EXT2_FT_SYMLINK = 7,
} dir_filetype_t;

#define EXT2_DIR_NAMELEN 255

#define EXT2_MIN_DIR_ENTRY_LEN 8

typedef struct {
    u32 inode;
    u16 rec_len;
    u8  name_len;
    u8  file_type;
    char name[EXT2_DIR_NAMELEN];
} ext2_dir_t;

END_C_HEADER

#endif // EXT2_DIRECTORY_H
