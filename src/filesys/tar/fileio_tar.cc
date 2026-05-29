#include <fractal.h>
#include <filesys/tar.h>
#include <filesys/fileio.h>

#ifdef USE_TARFS

extern virt_t global_initrd;

// NOTE: Remember to wrap every header->name reference
// in a call to fixup_tar_path!!
// Input: A TAR style path (eg. of the form "./dir1/dir2/file.txt")
// Output: Stripping the leading "." (to get /dir1/dir2/file.txt)
static inline char *fixup_tar_path(char *p) {
    if (!p) return p;
    if (*p != '.') return p;
    return p+1;
}

// Returns pointer to just the base filename of this file
extern "C" char *filesys_basename(char *path) {
    char *cursor = (char *)((usize)path + strlen(path));
    while (cursor != path) {
        if (*cursor == '/') return cursor+1;
        cursor--;
    }

    return path;
}

// basename for a directory
extern "C" char *filesys_dir_basename(char *path) {
    bool encountered_backslash = false;
    char *cursor = (char *)((usize)path + strlen(path));
    while (cursor != path) {
        if (*cursor == '/') {
            if (encountered_backslash) return cursor+1;
            encountered_backslash = true;
        }
        cursor--;
    }

    return path;
}

extern "C" bool filesys_is_file(void *h) {
    if (!h) return false;
    // Assume anything not a dir is a file
    return ((struct tar_posix_header *)h)->typeflag != DIRTYPE;
}

void *filesys_open_absolute(char *f) {
    TARIterator it = TARIterator((struct tar_posix_header *)global_initrd);

    while ((*it != NULL)) {
        tar_t *header = *it;
        char *fixedup_name = fixup_tar_path(header->name);
        // Use strequal instead of memcmp here in case the provided string is ""
        if (strequal(f, fixedup_name)) {
            return (void *)header;
        }
        ++it;
    }

    return NULL;
}

usize filesys_filelen(void *resource) {
    if (!resource) return 0;
    return read_tar_size(((tar_t*)resource)->size);
}

// Returns true if path is a child of directory dir
bool path_is_child_of_dir(char *dir, char *path) {
    char *path_basename = filesys_basename(path);
    usize path_upperlen = (path_basename - path);
    return memcmp(dir, path, path_upperlen);
}

bool dir_is_child_of_dir(char *dir, char *other) {
    char *path_basename = filesys_dir_basename(other);
    usize path_upperlen = (path_basename - other);
    return memcmp(dir, other, path_upperlen);
}

extern "C" kret_t list_files(char *dir_in, char **paths, usize n_paths) {
    char *dir = fixup_tar_path(dir_in);
    TARIterator it = TARIterator((struct tar_posix_header *)global_initrd);
    usize path_idx = 0;
    usize dirlen = strlen(dir);

    while ((*it != NULL) && path_idx < n_paths) {
        struct tar_posix_header *header = *it;
        char *fixedup_path = fixup_tar_path(header->name);
        if (strequal(fixedup_path, dir)) {
            // If exact match, skip
            ++it;
            continue;
        }
        if (memcmp(dir, fixedup_path, dirlen)) {
            if (filesys_is_file(header)) {
                if (path_is_child_of_dir(dir, fixedup_path)) {
                    strncpy(paths[path_idx], fixedup_path, MAX_FILENAME_LEN);
                }
            }
            path_idx++;
        }
        ++it;
    }

    return ALL_GOOD;
}

extern "C" char *list_files_as_one_string(char *dir_in, char *outbuf, usize nbytes) {
    char *cursor = outbuf;
    char *dir = fixup_tar_path(dir_in);
    TARIterator it = TARIterator((struct tar_posix_header *)global_initrd);
    usize dirlen = strlen(dir);

    while ((*it != NULL) && (cursor - outbuf) < nbytes) {
        struct tar_posix_header *header = *it;
        char *fixedup_path = fixup_tar_path(header->name);
        // printf("scanning %s\r\n", fixedup_path);
        if (strequal(fixedup_path, dir)) {
            // If exact match, skip
            ++it;
            continue;
        }
        if (memcmp(dir, fixedup_path, dirlen)) {
            if ((!filesys_is_file(header) && dir_is_child_of_dir(dir, fixedup_path)) || (filesys_is_file(header) && path_is_child_of_dir(dir, fixedup_path))) {
                char *path_cursor = fixedup_path;
                while((cursor - outbuf) < nbytes && *path_cursor != 0) {
                    *cursor++ = *path_cursor++;
                }
                if (*path_cursor == 0 && (cursor - outbuf) < nbytes) {
                    *cursor++ = '\n';
                }
                if ((cursor - outbuf) == nbytes) return cursor;
            }
        }
        ++it;
    }

    return cursor;
}

extern "C" kret_t list_files_recursive(char *dir_in, char **paths, usize n_paths) {
    char *dir = fixup_tar_path(dir_in);
    TARIterator it = TARIterator((struct tar_posix_header *)global_initrd);
    usize path_idx = 0;
    usize dirlen = strlen(dir);

    while ((*it != NULL) && path_idx < n_paths) {
        struct tar_posix_header *header = *it;
        char *fixedup_path = fixup_tar_path(header->name);
        if (strequal(fixedup_path, dir)) {
            // If exact match, skip
            ++it;
            continue;
        }
        if (memcmp(dir, fixedup_path, dirlen)) {
            strncpy(paths[path_idx], fixedup_path, MAX_FILENAME_LEN);
            path_idx++;
        }
        ++it;
    }

    return ALL_GOOD;
}

extern "C" usize filesys_read_file(void *h, u8 *buf, usize n_bytes, usize offset) {
    if (!h) return THING_DOESNT_EXIST;
    if (!buf) return THING_DOESNT_EXIST;
    usize file_size = read_tar_size(((tar_t *)h)->size);
    if (offset >= file_size) return 0;
    if ((n_bytes + offset) > file_size) n_bytes = file_size - offset;
    memcpy(buf, (void *)(((usize)h) + TAR_SECTOR_LEN + offset), n_bytes);
    return n_bytes;
}

extern "C" usize filesys_read_dir(void *h, u8 *buf, usize n_bytes, usize offset) {
    if (!h) return THING_DOESNT_EXIST;
    if (!buf) return THING_DOESNT_EXIST;
    if (filesys_is_file((tar_t*)h)) return 0; // Only read dirs
    if (offset != 0) printf("Warning: filesys_read_dir doesn't support nonzero offsets\r\n");
    char *result = list_files_as_one_string(fixup_tar_path(((tar_t*)h)->name), (char *)buf, n_bytes);
    return result - (char *)buf;
}

usize filesys_read(void *h, u8 *buf, usize n_bytes, usize offset) {
    if (!h) return THING_DOESNT_EXIST;
    if (!buf) return THING_DOESNT_EXIST;
    if (filesys_is_file((tar_t*)h)) return filesys_read_file(h, buf, n_bytes, offset);
    return filesys_read_dir(h, buf, n_bytes, offset);
}

#endif // USE_TARFS
