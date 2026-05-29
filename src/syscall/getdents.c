#include <fractal.h>
#include <filesys/fileio.h>
#include <task.h>
#include <stdio.h>
#include <copy_task.h>
#include <syscall.h>

i64 do_sys_getdents(task_t *t, u32 fd, u8 *buf, usize sz) {
    usize bytes_read = 0;
    if (fd > NUM_FDS) return -1;
    if (sz > SYS_READWRITE_LEN_MAX) sz = SYS_READWRITE_LEN_MAX;
    if (sz < sizeof(libc_dirent_t)) return -2;
    if (!buf) return -3;
    if (t->t_fds[fd].type != FD_FILE) return -4;
    if (!filesys_is_dir(t->t_fds[fd].resource)) return -5;

    ((libc_dirent_t *)buf)->d_reclen = 0;
    bytes_read = filesys_readdir(t->t_fds[fd].resource, (libc_dirent_t *)buf, t->t_fds[fd].offset);
    if (0 == bytes_read) return 0;
    t->t_fds[fd].offset += bytes_read;
    ((libc_dirent_t *)buf)->d_name[((libc_dirent_t *)buf)->d_namelen] = '\x00';
    return ((libc_dirent_t *)buf)->d_reclen;
}

i64 sys_getdents(u32 fd, virt_t user_buf, usize sz) {
    i64 rval = 0;
    libc_dirent_t cur_entry;
    if (sz > SYS_READWRITE_LEN_MAX) sz = SYS_READWRITE_LEN_MAX;
    if (sz < sizeof(libc_dirent_t)) return -2;

    rval = do_sys_getdents(current_task(), fd, (u8 *)&cur_entry, sizeof(cur_entry));
    if (rval < 0) return rval;

    // Sometimes ext2 puts random characters immediately after the filename
    // (eg. filenames are sometimes not null terminated, at least in my experience)
#ifdef EXT2_WARN_NON_NULL_STRINGS
    if (cur_entry.d_name[cur_entry.d_namelen] != '\x00') {
        printf("Warning: Non-NULL terminated string (%s)\n", cur_entry.d_name);
    }
#endif // EXT2_WARN_NON_NULL_STRINGS

    usize bytes_to_copy_out = rval;
    if (bytes_to_copy_out != 0) {
        if (ALL_GOOD != copy_to_task(current_task(), user_buf, (u8 *)&cur_entry, bytes_to_copy_out)) {
            return -1;
        }
    }

#ifdef DEBUG_GETDENTS
    printf("-> %s (%d) (%d)\n", cur_entry.d_name, cur_entry.d_namelen, rval);
#endif // DEBUG_GETDENTS

    return rval;
}
