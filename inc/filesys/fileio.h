#ifndef FRACTAL_FILEIO_H
#define FRACTAL_FILEIO_H

#define USE_EXT2

// These methods are common to any filesystem supported by Fractal
// We return and consume void *'s which are transparent filesystem objects
// that are implementation-defined.
// For EXT2, these are inode_t's, and for tar these are struct tar_posix_header's.

#include <fractal.h>

BEGIN_C_HEADER

// Recall that inode numbers for ext2 begin at 1, not 0!
typedef u32 inodenum_t;

// Fractal file kinds
typedef enum {
    FILE_NONE = 0,
    FILE_REGULAR,
    FILE_DIRECTORY
} filekind_t;

#define DIRENT_NAME_MAX 255

// Userspace-visible cross-platform directory type
// libc headers have this same struct as `struct dirent`
typedef struct {
    u64 d_off;
    u32 d_fileno;
    u16 d_namelen;
    u16 d_reclen;
    u8 d_type;
    char d_name[DIRENT_NAME_MAX + 1];
} libc_dirent_t;

/*
 * d_type file types
 */
#define DT_UNKNOWN   0
#define DT_FIFO      1
#define DT_CHR       2
#define DT_DIR       4
#define DT_BLK       6
#define DT_REG       8
#define DT_LNK      10
#define DT_SOCK     12
#define DT_WHT      14

#define INITRD_MAX_SIZE ((512 * LARGE_PAGE_SIZE))

// Ensure this is longer than whatever the max name length is on
// all supported filesystems
#define MAX_FILENAME_LEN 256

/*
 * fs_init
 * Discover all filesystem tables and initialize global state.
 * Must be called after discovering the global initrd and before
 * attempting to use any filesystem resources.
 */
void fs_init(void);

/*
 * filesys_filelen
 * Get length of a file
 */
usize filesys_filelen(void *resource);

// Returns pointer to just the base filename of this file
char *filesys_basename(char *path);

// Set outs to the directory name preceding the file name in path
void filesys_dirname(char *outs, char *path, usize outs_sz);

/*
 * filesys_open_absolute
 * Returns the void header for a given file, or NULL if the file doesn't exist.
 * Must be an absolute path.
 */
void *filesys_open_absolute(char *path);

/*
 * filesys_open_relative
 * Open a resource relative to another one.
 * Acts like filesys_open_absolute if the first character is '/' or cur_dir is NULL.
 * Otherwise this returns the path-relative file from cur_dir at path.
 */
void *filesys_open_relative(char *path, void *cur_dir);

usize filesys_read(void *h, u8 *buf, usize n_bytes, usize offset);
usize filesys_write(void *h, u8 *buf, usize n_bytes, usize offset);

/*
 * filesys_readdir
 * Read out a libc_dirent_t from a directory inode.
 *
 * Inputs:
 *  - h: A pointer to a directory inode
 *  - buf: A libc_dirent_t to fill in
 *  - offset: How many bytes into the directory inode to begin reading from?
 *            Assumes offset bytes into h points to an ext2_dir_t.
 *
 * Return Value: The number of bytes we read in h (add this to your fd's offset).
 * Returns 0 if there is no more data to read.
 */
usize filesys_readdir(void *h, libc_dirent_t *buf, usize offset);

// Returns true if handle is a regular file
bool filesys_is_file(void *h);

// Returns true if handle is a directory
bool filesys_is_dir(void *h);

/*
 * filesys_mkdir
 * Creates a new directory at path.
 * Assumes path is of the form "/path/to/directory" or "/path/to/directory/".
 *
 * Returns true if successful, false otherwise (eg. file already exists,
 * or couldn't create the path, or something).
 */
bool filesys_mkdir(char *path, void *cur_dir);

/*
 * filesys_create
 * Creates a new file at path.
 * Assumes path is of the form "/path/to/file"
 *
 * Returns true if successful, false otherwise (eg. file already exists,
 * or couldn't create the path, or something).
 */
void *filesys_create(char *filename_in, void *cur_dir, filekind_t kind);

/*
 * Essentially returns open("/")
 * What's the root inode / resource for this fs?
 */
void *filesys_root_resource();

/*
 * filesys_close
 * Close a resource.
 */
void filesys_close(void *resource);

// Get inode num of a handle
// Not super portable to non-inode based filesystems
int filesys_inodenum(void *resource);

/*
 * filesys_unlink
 * Removes a resource relative to another one.
 * Uses an absolute path if the first character is '/' or cur_dir is NULL.
 * Otherwise this returns the path-relative file from cur_dir at path.
 */
int filesys_unlink(char *filename_in, void *cur_dir);

inodenum_t filesys_open_inodenum_relative(char *path, void *cur_dir);

// Does this file exist?
bool filesys_exists(char *path, void *cur_dir);

END_C_HEADER

#endif // FRACTAL_FILEIO_H
