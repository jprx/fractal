#include <stdint.h>
#include <string.h>
#include <errno.h>
#include <termios.h>
#include <stdio.h>
#include <poll.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <sys/termios.h>

#define DEFAULT_USER_HEAP    0x555555000000
extern int errno;
typedef enum {
    SYS_HANDOFF    = 0,
    SYS_SPAWN      = 1,
    SYS_QUIT       = 2,
    SYS_OPEN       = 3,
    SYS_READ       = 4,
    SYS_WRITE      = 5,
    SYS_CLOSE      = 6,
    SYS_STAT       = 7,
    SYS_LSEEK      = 8,
    SYS_MKDIR      = 9,
    SYS_CHDIR      = 10,
    SYS_FORK       = 11,
    SYS_EXECVE     = 12,
    SYS_WAIT       = 13,
    SYS_GETDENTS   = 14,
    SYS_PIPE       = 15,
    SYS_DUP        = 16,
    SYS_DUP2       = 17,
    SYS_FCNTL      = 18,
    SYS_UNAME      = 19,
    SYS_FSTATAT    = 20,
    SYS_OPENAT     = 21,
    SYS_FCHDIR     = 22,
    SYS_ACCESS     = 23,
    SYS_SYSCONF    = 24,
    SYS_UNLINK     = 25,
    SYS_WAITPID    = 26,
    SYS_GETPID     = 27,
    SYS_KEXEC      = 28,
    SYS_FRACTAL    = 29, /* The "Magic" */
    SYS_SELECT     = 30,
    SYS_POLL       = 31,
    SYS_TIOS_GET   = 32,
    SYS_TIOS_SET   = 33,
    SYS_RMDIR      = 34,
    SYS_SBRK       = 35,
    SYS_FT_CREAT   = 36,
    SYS_FT_JOIN    = 37,
    SYS_FT_EXIT    = 38,
    SYS_FT_SWTCH   = 39,
    SYS_MPROTECT   = 40,
    SYS_FMAP       = 41,
    SYS_XLATE      = 42,
    SYS_FUNMAP     = 43,
    SYS_GIGAMAP    = 44,

    NUM_SYSCALLS, /* How many system calls are defined? */
} syscall_nr_t;

typedef enum {
    GIGAOPT_ALLOC    = 1, // create a new gigamap
    GIGAOPT_CARVE    = 2, // carve out a page of the gigamap
    GIGAOPT_UGETBASE = 3, // returns a VA that maps to the default gigamap PA (for user half)
    GIGAOPT_RESET    = 4, // reset the whole gigamap
    GIGAOPT_UNMAP    = 5, // free the whole gigamap
    GIGAOPT_KGETBASE = 6, // returns a VA that maps to the default gigamap PA (for kernel half)
} gigaopt_t;

int64_t do_syscall(uint64_t num, ...);

int _close(int file) {
  return do_syscall(SYS_CLOSE, file);
}
int _open(const char *name, int flags, int mode) {
  int rval = do_syscall(SYS_OPEN, name, flags, mode);
  if (rval < 0) {
    errno = 2;
  }
  return rval;
}
void _exit(uint64_t reason) {
  do_syscall(SYS_QUIT, reason);
}
int _write(int file, char *ptr, int len) {
  return do_syscall(SYS_WRITE, file, ptr, len);
}
int _read(int file, char *ptr, int len) {
  return do_syscall(SYS_READ, file, ptr, len);
}
char *__env[1] = { 0 };
char **environ = __env;

int _execve(char *name, char **argv, char **env) {
  int rval = do_syscall(SYS_EXECVE, name, argv, env);
  // If we got here, execve failed
  errno = ENOENT;
  return rval;
}

int _fork(void) {
  return do_syscall(SYS_FORK);
}

int _fstat(int file, struct stat *st) {
  int rval = do_syscall(SYS_STAT, file, st);
  if (rval < 0) {
    errno = ENOENT;
  }
  return rval;
}
int _getpid(void) {
  return do_syscall(SYS_GETPID);
}
int _isatty(int file) {
  return 1;
}

int _kill(int pid, int sig) {
  errno = EINVAL;
  return -1;
}

int _link(char *old, char *new) {
  errno = EMLINK;
  return -1;
}
int _lseek(int file, int ptr, int dir) {
  return do_syscall(SYS_LSEEK, file, ptr, dir);
}
uint64_t _sbrk(int incr) {
  return do_syscall(SYS_SBRK, incr);
}
int _times(struct tms *buf) {
  return -1;
}

int _unlink(char *name) {
  return do_syscall(SYS_UNLINK, name);
}

int _wait(int *status) {
  return do_syscall(SYS_WAIT, status, 0);
}

// Things required for binutils:

int _stat(const char *__restrict path, struct stat *__restrict sbuf ) {
  int rval = 0;
  int fd = _open(path,0,0);
  if (fd < 0) return fd;
  rval = _fstat(fd, sbuf);
  _close(fd);
  return rval;
}

int	lstat (const char *__restrict __path, struct stat *__restrict __buf ) {
  return _stat(__path, __buf);
}

long sysconf(int name) {
  return do_syscall(SYS_SYSCONF, name);
}

// dash doesn't use this to learn the current directory-
// instead it does some path traversal magic to learn where it is.
// For now this method is not implemented, but when we need it,
// we can traverse from the current working directory via ".."
// until we reach "/" and build the path from doing that.
// (as the kernel doesn't save the current path as a string, but an inode ptr).
char *getwd(char *buf) {
  strcpy(buf, "/UNKNOWN");
  return buf;
}

int _gettimeofday(struct timeval *tp, void *tzp) {
  tp->tv_sec = 0;
  tp->tv_usec = 0;
  return 0;
}

int _fcntl(int fd, int cmd, int arg) {
  return do_syscall(SYS_FCNTL, fd, cmd, arg);
}

mode_t umask(mode_t cmask) {
  return 0;
}

int chmod(const char *path, mode_t mode) {
  return -1;
}

int access(const char *path, int mode) {
  return do_syscall(SYS_ACCESS, path, mode);
}

int mkdir(const char *path, mode_t mode) {
  return do_syscall(SYS_MKDIR, path, mode);
}

int chdir(const char *path) {
  return do_syscall(SYS_CHDIR, path);
}

int dup(int fildes) {
  do_syscall(SYS_DUP, fildes);
}

int utime(const char *path, const struct utimbuf *times) {
  return -1;
}

int fchmod(int fildes, mode_t mode) {
  return -1;
}

int rmdir(const char *path) {
  return do_syscall(SYS_RMDIR, path);
}

int sleep(unsigned int seconds) {
  return -1;
}

int pipe(int fildes[2]) {
  return do_syscall(SYS_PIPE, fildes);
}

int dup2(int fildes, int fildes2) {
  return do_syscall(SYS_DUP2, fildes, fildes2);
}

// Stuff I needed for dash:
int getuid() { return -1; }
int geteuid() { return -1; }
int getgid() { return -1; }
int getegid() { return -1; }
int tcsetpgrp() { return -1; }
int sigprocmask() { return -1; }
int setpgid() { return -1; }
int sigsuspend() { return -1; }
int getpgrp() { return -1; }
int killpg() { return -1; }
int tcgetpgrp() { return -1; }
int vfork() { return _fork(); }
int sigaction() { return -1; }
int getgroups() { return -1; }
int getdents(uint32_t fd, uint8_t *dent, uint64_t sz) { return do_syscall(SYS_GETDENTS, fd, dent, sz); }
int getppid() { return -1; }
int wait3(int *status, int options, void *rusage) { return do_syscall(SYS_WAIT, status, options); }

// Stuff for coreutils
void *setmntent(const char *filename, const char *type) { return NULL; }
struct mntent *getmntent(void *fp) { return NULL; }
int addmntent(void *fp, const struct mntent *mnt) { return 0; }
int endmntent(void *fp) { return 0; }
char *hasmntopt(const struct mntent *mnt, const char *opt) { return NULL; }
int fsync(int fdes) {
  // Fractal never syncs to disk because all writes
  // are immediately sent the ramdisk. So, fsync is always successful.
  return 0;
}
void *getservbyname() {return NULL;}
int getrlimit() { return -1; }
int waitpid(int64_t pid, int64_t *status_loc, int64_t options) {
  return do_syscall(SYS_WAITPID, pid, status_loc, options);
}
int alarm() { return -1; }
int setreuid() { return -1; }
int setregid() { return -1; }
int getgrgid() { return -1; }
int getgrnam() { return -1; }
int gethostbyname() { return -1; }
int ftruncate() { return -1; }
int setgrent() { return -1; }
int getgrent() { return -1; }
int endgrent() { return -1; }
int fchown() { return -1; }
int lchown() { return -1; }
int chown() { return -1; }
int select(int nfds, fd_set *readfds, fd_set *writefds, fd_set *errfds, struct timeval *timeout) {
  return do_syscall(SYS_SELECT, nfds, readfds, writefds, errfds, timeout);
}
typedef uint64_t fthread_t;
int fthread_create(fthread_t *f, uint64_t attr, void *(*start_fn)(void *), void *arg) {
  return do_syscall(SYS_FT_CREAT, f, attr, start_fn, arg);
}
int fthread_join(fthread_t f, void **val_ptr) {
  return do_syscall(SYS_FT_JOIN, f, val_ptr);
}
int fthread_switch(fthread_t f) {
  return do_syscall(SYS_FT_SWTCH, f);
}
int fthread_exit(void *val_ptr) {
  return do_syscall(SYS_FT_EXIT, val_ptr);
}
int pthread_create() { return -1; }
int pthread_exit() { return -1; }
int pthread_join() { return -1; }
int pthread_mutex_init() { return -1; }
int pthread_mutex_lock() { return -1; }
int pthread_cond_signal() { return -1; }
int pthread_mutex_unlock() { return -1; }
int pthread_cond_wait() { return -1; }
int pthread_cond_init() { return -1; }
int pthread_mutex_destroy() { return -1; }
int pthread_cond_destroy() { return -1; }
int pthread_attr_init() { return -1; }
int uname(struct utsname *unbuf) { return do_syscall(SYS_UNAME, unbuf); }
int setrlimit() { return -1; }

// This is set by crt0:
char *__progname = NULL;
char __progname_default[256] = "fractal-unknown-app";
char *getprogname() {
  if (NULL == __progname) {
    return __progname_default;
  }
  return __progname;
}

// This doesn't work unless crt0 is broken
// Most GNU programs have their own setprogname that doesn't talk to this anyways
// getprogname should just do the "right thing" and ignore setprogname in most cases.
void setprogname(const char *progname) {
  if (strlen(progname) > sizeof(__progname_default)) return;
  strcpy(__progname_default, progname);
}
int fpathconf() { return -1; }
int pathconf() { return -1; }
int statfs() { return -1; }
int fstatfs() { return -1; }
int ioctl(int fildes, unsigned long req, ...) { return -1; }
int cfsetospeed(struct termios *i, speed_t s) { return -1; }
int cfsetispeed(struct termios *i, speed_t s) { return -1; }

// Stuff for readline (and bash I suspect)
int tcgetattr(int fd, struct termios *termios_p) {
  return do_syscall(SYS_TIOS_GET, fd, termios_p);
}
int tcsetattr(int fd, int optional_actions, const struct termios *termios_p) {
  return do_syscall(SYS_TIOS_SET, fd, optional_actions, termios_p);
}

int getpagesize() {
  return 0x1000;
}

// Stuff for findutils:
int poll(struct pollfd fds[], nfds_t nfds, int timeout) {
  return do_syscall(SYS_POLL, fds, nfds, timeout);
}
int setuid(uid_t uid) {return 0;}
int setgid(gid_t gid) {return 0;}
int seteuid() {return 0;}
int setegid() {return 0;}
int fstatat(int fd, const char *path, struct stat *buf, int flag) {
  return do_syscall(SYS_FSTATAT, fd, path, buf, flag);
}
int openat(int fd, const char *path, int flag) {
  return do_syscall(SYS_OPENAT, fd, path, flag);
}
int fchdir(int fd) {
  return do_syscall(SYS_FCHDIR, fd);
}

// Fractal Magic
uint64_t sys_fractal(uint64_t cmd, uint64_t arg) {
  return do_syscall(SYS_FRACTAL, cmd, arg);
}

uint64_t handoff() {
  return do_syscall(SYS_HANDOFF);
}

int mprotect(void *v, size_t s, uint64_t p) {
  return do_syscall(SYS_MPROTECT, v, s, p);
}

int fmap_strided(void *newpgptr, size_t n_copies, size_t log_stride) {
  return do_syscall(SYS_FMAP, newpgptr, n_copies, log_stride);
}

int fmap(void *newpgptr) {
  return fmap_strided(newpgptr, 1, 0);
}

int funmap(void *pgptr) {
  return do_syscall(SYS_FUNMAP, pgptr);
}

int xlate_v2p(uint64_t va, uint64_t *pa_ptr) {
  return do_syscall(SYS_XLATE, va, pa_ptr);
}

int gmap_alloc(void **newpgptr) {
  return do_syscall(SYS_GIGAMAP, GIGAOPT_ALLOC, newpgptr);
}

int gmap_carve(void *carveout) {
  return do_syscall(SYS_GIGAMAP, GIGAOPT_CARVE, carveout);
}

int gmap_get_ubase(void **baseptr) {
  return do_syscall(SYS_GIGAMAP, GIGAOPT_UGETBASE, baseptr);
}

int gmap_get_kbase(void **baseptr) {
  return do_syscall(SYS_GIGAMAP, GIGAOPT_KGETBASE, baseptr);
}

int gmap_reset() {
  return do_syscall(SYS_GIGAMAP, GIGAOPT_RESET);
}

int gmap_unmap() {
  return do_syscall(SYS_GIGAMAP, GIGAOPT_UNMAP);
}

