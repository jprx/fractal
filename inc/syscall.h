#ifndef SYSCALL_H
#define SYSCALL_H

#include <fractal.h>
#include <fthread.h>
#include <task.h>
#include <errno.h>

BEGIN_C_HEADER

/*
 * Syscall
 * Handles a system call. Call this from an interrupt context where
 * userspace has just requested something from us.
 *
 * The arguments to this function are defined by the Fractal ABI;
 * callers should take care to ensure the arguments are passed correctly.
 *
 * This method writes its return value directly into the current task's
 * saved register state, so no return value is provided here. For system
 * calls that block, a kernel thread or other system call will write the
 * return value into the appropriate register struct when complete.
 *
 * This may cause a call to naptime() and the current thread to block!
 */
void Syscall(u64 num, u64 arg0, u64 arg1, u64 arg2, u64 arg3, u64 arg4);

/********************************
 * Various structs and defines  *
 * for the system calls         *
 ********************************/

/*
 * SpawnMode is a u64 bit vector:
 *             3   2   1   0
 * +-------------------------+
 * | Unused  | P | A | I | C |
 * +-------------------------+
 *
 * C: Cooperation Mode. 0 for preemptive, 1 for cooperative.
 * I: Immediate Mode.   0 for wait for child to finish, 1 to run alongside.
 * A: Abandoned Mode.   0 means cur_thread is the parent, 1 means this task has no parent.
 * P: Privilege Level.  0 == USER_THREAD, 1 == KERNEL_OUTER_THREAD. Can't spawn inner tasks.
 */
typedef u64 SpawnMode;
#define SPAWN_MODE_PREEMPTIVE  ((0b0000))
#define SPAWN_MODE_COOPERATIVE ((0b0001))
#define SPAWN_MODE_WAIT_FINISH ((0b0000))
#define SPAWN_MODE_IMMEDIATE   ((0b0010))
#define SPAWN_MODE_HAS_PARENT  ((0b0000))
#define SPAWN_MODE_ABANDONED   ((0b0100))
#define SPAWN_MODE_USER_TASK   ((0b0000))
#define SPAWN_MODE_KERN_TASK   ((0b1000))

typedef enum {
    GIGAOPT_ALLOC    = 1, // create a new gigamap
    GIGAOPT_CARVE    = 2, // carve out a page of the gigamap
    GIGAOPT_UGETBASE = 3, // returns a VA that maps to the default gigamap PA (for user half)
    GIGAOPT_RESET    = 4, // reset the whole gigamap
    GIGAOPT_UNMAP    = 5, // free the whole gigamap
    GIGAOPT_KGETBASE = 6, // returns a VA that maps to the default gigamap PA (for kernel half)
} gigaopt_t;

/* from newlib/libc/include/sys/stat.h:61 */
#define STAT_MODE_DIR    0040000
#define STAT_MODE_REG    0100000
#define STAT_MODE_CHAR   0020000
#define STAT_MODE_BLK    0060000
#define STAT_MODE_FIFO   0010000
#define	S_IRWXU 	(S_IRUSR | S_IWUSR | S_IXUSR)
#define		S_IRUSR	0000400	/* read permission, owner */
#define		S_IWUSR	0000200	/* write permission, owner */
#define		S_IXUSR 0000100/* execute/search permission, owner */
#define	S_IRWXG		(S_IRGRP | S_IWGRP | S_IXGRP)
#define		S_IRGRP	0000040	/* read permission, group */
#define		S_IWGRP	0000020	/* write permission, grougroup */
#define		S_IXGRP 0000010/* execute/search permission, group */
#define	S_IRWXO		(S_IROTH | S_IWOTH | S_IXOTH)
#define		S_IROTH	0000004	/* read permission, other */
#define		S_IWOTH	0000002	/* write permission, other */
#define		S_IXOTH 0000001/* execute/search permission, other */
#define ACCESSPERMS (S_IRWXU | S_IRWXG | S_IRWXO) /* 0777 */

/* from newlib sys/_types.h and manually inspecting offsets in GDB */
typedef unsigned short dev_t;
typedef unsigned short ino_t;
typedef unsigned short mode_t;
typedef unsigned short nlink_t;
typedef unsigned short gid_t;
typedef unsigned short uid_t;
typedef u64 off_t;
typedef long blksize_t;
typedef long blkcnt_t;

/* from man 2 stat on Linux */
/* and newlib stuff */
struct timespec {
    i64 tv_sec;
    i64 tv_nsec;
};

struct stat {
    dev_t     st_dev;         /* ID of device containing file */
    ino_t     st_ino;         /* Inode number */
    mode_t    st_mode;        /* File type and mode */
    nlink_t   st_nlink;       /* Number of hard links */
    uid_t     st_uid;         /* User ID of owner */
    gid_t     st_gid;         /* Group ID of owner */
    dev_t     st_rdev;        /* Device ID (if special file) */
    off_t     st_size;        /* Total size, in bytes */
    struct timespec st_atim;
    struct timespec st_mtim;
    struct timespec st_ctim;
    blksize_t st_blksize;     /* Block size for filesystem I/O */
    blkcnt_t  st_blocks;      /* Number of 512B blocks allocated */
};

#define LSEEK_WHENCE_SET  0
#define LSEEK_WHENCE_CUR  1
#define LSEEK_WHENCE_END  2
#define LSEEK_WHENCE_DATA 3
#define LSEEK_WHENCE_HOLE 4

#define FORK_PARENT_RETURNVAL 0

/* From newlib/libc/include/sys/wait.h */
#define WNOHANG 1
#define WUNTRACED 2
#define WIFEXITED(w)	(((w) & 0xff) == 0)
#define WIFSIGNALED(w)	(((w) & 0x7f) > 0 && (((w) & 0x7f) < 0x7f))
#define WIFSTOPPED(w)	(((w) & 0xff) == 0x7f)
#define WEXITSTATUS(w)	(((w) >> 8) & 0xff)
#define WTERMSIG(w)	((w) & 0x7f)
#define WSTOPSIG	WEXITSTATUS

/* From newlib/libc/include/sys/_default_fcntl.h */
#define	_FREAD		0x0001	/* read enabled */
#define	_FWRITE		0x0002	/* write enabled */
#define	_FAPPEND	0x0008	/* append (writes guaranteed at the end) */
#define	_FMARK		0x0010	/* internal; mark during gc() */
#define	_FDEFER		0x0020	/* internal; defer for next gc pass */
#define	_FASYNC		0x0040	/* signal pgrp when data ready */
#define	_FSHLOCK	0x0080	/* BSD flock() shared lock present */
#define	_FEXLOCK	0x0100	/* BSD flock() exclusive lock present */
#define	_FCREAT		0x0200	/* open with file create */
#define	_FTRUNC		0x0400	/* open with truncation */
#define	_FEXCL		0x0800	/* error on open if file exists */
#define	_FNBIO		0x1000	/* non blocking I/O (sys5 style) */
#define	_FSYNC		0x2000	/* do all writes synchronously */
#define	_FNONBLOCK	0x4000	/* non blocking I/O (POSIX style) */
#define	_FNDELAY	_FNONBLOCK	/* non blocking I/O (4.2 style) */
#define	_FNOCTTY	0x8000	/* don't assign a ctty on this open */
/*
 * Flag values for open(2) and fcntl(2)
 * The kernel adds 1 to the open modes to turn it into some
 * combination of FREAD and FWRITE.
 */
#define	O_RDONLY	0		/* +1 == FREAD */
#define	O_WRONLY	1		/* +1 == FWRITE */
#define	O_RDWR		2		/* +1 == FREAD|FWRITE */
#define	O_APPEND	_FAPPEND
#define	O_CREAT		_FCREAT
#define	O_TRUNC		_FTRUNC
#define	O_EXCL		_FEXCL
#define O_SYNC		_FSYNC
/*	O_NDELAY	_FNDELAY 	set in include/fcntl.h */
/*	O_NDELAY	_FNBIO 		set in include/fcntl.h */
#define	O_NONBLOCK	_FNONBLOCK
#define	O_NOCTTY	_FNOCTTY

/* From newlib/libc/include/sys/_default_fcntl.h */
/* fcntl(2) requests */
#define	F_DUPFD		0	/* Duplicate fildes */
#define	F_GETFD		1	/* Get fildes flags (close on exec) */
#define	F_SETFD		2	/* Set fildes flags (close on exec) */
#define	F_GETFL		3	/* Get file flags */
#define	F_SETFL		4	/* Set file flags */
#define	F_GETOWN 	5	/* Get owner - for ASYNC */
#define	F_SETOWN 	6	/* Set owner - for ASYNC */
#define	F_GETLK  	7	/* Get record-locking information */
#define	F_SETLK  	8	/* Set or Clear a record-lock (Non-Blocking) */
#define	F_SETLKW 	9	/* Set or Clear a record-lock (Blocking) */
#define	F_RGETLK 	10	/* Test a remote lock to see if it is blocked */
#define	F_RSETLK 	11	/* Set or unlock a remote lock */
#define	F_CNVT 		12	/* Convert a fhandle to an open fd */
#define	F_RSETLKW 	13	/* Set or Clear remote record-lock(Blocking) */
#define	F_DUPFD_CLOEXEC	14	/* As F_DUPFD, but set close-on-exec flag */

/* fcntl(2) flags (l_type field of flock structure) */
#define	F_RDLCK		1	/* read lock */
#define	F_WRLCK		2	/* write lock */
#define	F_UNLCK		3	/* remove lock(s) */
#define	F_UNLKSYS	4	/* remove remote locks for a given system */

/* Special descriptor value to denote the cwd in calls to openat(2) etc. */
#define AT_FDCWD -2

/* from newlib winsup/cygwin/include/sys/utsname.h */
#define _UTSNAME_LENGTH 65

// Max amount of IO we can do at a time
#define SYS_READWRITE_LEN_MAX 4096

struct utsname
{
  char sysname[_UTSNAME_LENGTH];
  char nodename[_UTSNAME_LENGTH];
  char release[_UTSNAME_LENGTH];
  char version[_UTSNAME_LENGTH];
  char machine[_UTSNAME_LENGTH];
#if __GNU_VISIBLE
  char domainname[_UTSNAME_LENGTH];
#else
  char __domainname[_UTSNAME_LENGTH];
#endif
};

/* Structs and typedefs for select, modified from newlib/newlib/libc/include/sys/select.h */
typedef u64 __fd_mask;
#define FD_SETSIZE	64
#define _NFDBITS	((int)sizeof(__fd_mask) * 8)

#ifndef	_howmany
#define	_howmany(x,y)	(((x) + ((y) - 1)) / (y))
#endif

typedef struct fd_set {
  __fd_mask __fds_bits[_howmany(FD_SETSIZE, _NFDBITS)];
} fd_set;

#define __fdset_mask(n)	((__fd_mask)1 << ((n) % _NFDBITS))
#define FD_CLR(n, p)	((p)->__fds_bits[(n)/_NFDBITS] &= ~__fdset_mask(n))
#define FD_COPY(f, t)	(void)(*(t) = *(f))
#define FD_ISSET(n, p)	(((p)->__fds_bits[(n)/_NFDBITS] & __fdset_mask(n)) != 0)
#define FD_SET(n, p)	((p)->__fds_bits[(n)/_NFDBITS] |= __fdset_mask(n))
#define FD_ZERO(p) do {					\
        fd_set *_p;					\
        __size_t _n;					\
							\
        _p = (p);					\
        _n = _howmany(FD_SETSIZE, _NFDBITS);		\
        while (_n > 0)					\
                _p->__fds_bits[--_n] = 0;		\
} while (0)

#undef _howmany

// Flags for mprotect
#define PROT_NONE  0x00
#define PROT_READ  0x01
#define PROT_WRITE 0x02
#define PROT_EXEC  0x04
#define PROT_ALL ((PROT_READ | PROT_WRITE | PROT_EXEC))
#define PROT_USER    0x10
#define PROT_KERNEL  0x20

// Attributes for fthread_create
#define FTHREAD_USER_THREAD   ((PROT_USER))
#define FTHREAD_KERNEL_THREAD ((PROT_KERNEL))
#define FTHREAD_PREEMPTIVE  ((0x40))
#define FTHREAD_COOPERATIVE ((0x80))

/* Argument options to the sys_fractal system call */
typedef enum {
  FRACTAL_SET_SCHEDULER_AFFINITY = 1,
  FRACTAL_SET_THREAD_KIND = 2,
  FRACTAL_GET_TASK_KIND = 3,
  FRACTAL_GET_KERNEL_BASE = 4,
  FRACTAL_SET_PREEMPTION_MODE = 5,
  FRACTAL_SET_SCHEDULER_LOGGING = 6,
  FRACTAL_SET_MEMORY_TRACING = 7,
} sysfractal_arg_t;

typedef enum {
  FRACTAL_SCHEDULER_PREEMPTIVE = 0,
  FRACTAL_SCHEDULER_COOPERATIVE = 1,
} scheduler_mode_t;

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

/*************************************
 * User-Callable Syscall Definitions *
 *************************************/

/*
 * handoff()
 * Let the CPU take a break- pass control back to the scheduler.
 */
u64 sys_handoff();

/*
 * spawn(char *name, SpawnMode)
 * Launch a new task.
 */

// Assumption: caller already called copy_from_user
// This is because sys_spawn is very useful for kernel tasks, so I want it to be easy
// to call from kernel contexts (so you can just call this with a string)
i64 sys_spawn(virt_t binpath_user, virt_t user_args, SpawnMode mode);

/*
 * quit(i64 reason)
 * Destroy a task, unlinking it from the scheduler list,
 * and if applicable, passing the return reason back to its parent.
 */
i64 sys_quit(i64 reason);

/*
 * open(char *path)
 * Open a resource
 */
i64 sys_open(virt_t path, u64 flags);

/*
 * read(int fd, virt_t buf, usize sz)
 * Read sz bytes into buf from file descriptor fd
 */
i64 sys_read(u32 fd, virt_t buf, usize sz);

/*
 * write(int fd, virt_t buf, usize sz)
 * Write sz bytes from buf into file descriptor fd
 */
i64 sys_write(u32 fd, virt_t buf, usize sz);

/*
 * close(int fd)
 * Close a resource
 */
i64 sys_close(u32 fd);

/*
 * stat(int fd, struct stat *buf)
 * Learn information about a given file descriptor's underlying file
 * on the file system.
 */
i64 sys_stat(u32 fd, virt_t userbuf);

/*
 * lseek
 * Move fd's offset field.
 */
i64 sys_lseek(u32 fd, i32 offset, int whence);

/*
 * mkdir
 * Create a directory.
 */
i64 sys_mkdir(virt_t path);

/*
 * chdir
 * Change to a new working directory.
 */
i64 sys_chdir(virt_t new_wd);

/*
 * fork
 * Create an exact carbon copy of the current task.
 * Returns 0 if we are the original, and the new PID
 * if we are the child.
 */
i64 sys_fork();

/*
 * execve
 * An interface for spawning a Fractal task the POSIX way.
 *
 * Replaces the current process with the program located at path.
 * All file descriptors, the parent, and PID are copied into the new task.
 */
i64 sys_execve(virt_t path, virt_t argv, virt_t envp);

/*
 * wait
 * A Fractal interface for the POSIX wait/wait3/wait4 syscalls.
 *
 * Puts the calling task to sleep while awaiting the completion
 * of one of its child tasks.
 *
 * This is essentially the regular wait syscall, wait3 and wait4
 * can just use this as we don't acknowledge (yet) the other
 * arguments like rusage, and pid that those variants
 * provide. Use sys_waitpid if pid matters.
 *
 * Writes the exit code of the child task into user_status_loc.
 *
 * Returns the PID of the task that stopped, or -1 on error.
 * Libc should set errno to the POSIX-compliant errno reason.
 */
i64 sys_wait(virt_t user_status_loc, u64 flags);

/*
 * getdents
 * Read from an opened directory.
 */
i64 sys_getdents(u32 fd, virt_t user_buf, usize nbytes);

/*
 * pipe
 * Opens an unnamed pipe.
 */
i64 sys_pipe(virt_t user_fds);

/*
 * dup
 * Copy an fd into the next available fd.
 */
i64 sys_dup(u32 fd_from);

/*
 * dup2
 * Move one fd into another, closing the destination
 * number if a file exists there.
 */
i64 sys_dup2(u32 fd_from, u32 fd_to);

/*
 * fcntl
 * Supports only the minimal required subset of fcntl commands.
 */
i64 sys_fcntl(u32 fd, u64 cmd, u64 arg);

/*
 * uname
 * Get info about the system / kernel.
 */
i64 sys_uname(virt_t unamebuf);

/*
 * sys_fstatat
 * fstatat system call- stat but relative to the path at fd.
 */
i64 sys_fstatat(u32 fd, virt_t user_path, virt_t user_statbuf, u32 flags);

/*
 * sys_openat
 * open but relative to fd instead of cwd.
 */
i64 sys_openat(u32 fd, virt_t user_path, u64 flags);

/*
 * fchdir
 * Change working directory to the dir specified by fd.
 */
i64 sys_fchdir(u32 fd);

/*
 * access
 * Returns 0 if the file can be accessed (eg. it exists),
 * -1 otherwise.
 */
i64 sys_access(virt_t user_path, u64 mode);

/*
 * sysconf
 * Return some system configuration variable referred to by `name'.
 */
i64 sys_sysconf(u64 name);

/*
 * unlink
 * Delete a file at `user_path', if it exists.
 */
i64 sys_unlink(virt_t user_path);

/*
 * waitpid
 * Wait, but for a specific PID.
 */
i64 sys_waitpid(pid_t p, virt_t user_status_loc, u64 options);

/*
 * getpid
 * Returns the task's PID.
 */
i64 sys_getpid();

/*
 * kexec
 * Replace the running kernel with a new one.
 */
i64 sys_kexec(virt_t new_kernel_path);

/*
 * fractal
 * Configure the scheduler for this task.
 */
i64 sys_fractal(u64 cmd, u64 arg);

/*
 * select
 * Multiplex file descriptors.
 * Prefixing all user pointers with u_ to recall they're user pointers.
 */
i64 sys_select(i32 nfds, virt_t u_readfds, virt_t u_writefds, virt_t u_errfds, virt_t u_timeout);

/*
 * poll
 * Multiplex file descriptors. Very similar to select
 * (I haven't decided which of the two we will use yet).
 */
i64 sys_poll(virt_t u_fds, u32 nfds, i32 timeout);

/*
 * tcgetattr
 * Termios support.
 */
i64 sys_tcgetattr(i32 fd, virt_t u_termios_p);

/*
 * tcsetattr
 * Termios support.
 */
i64 sys_tcsetattr(i32 fd, i32 optional_actions, virt_t u_termios_p);

/*
 * rmdir
 * Removes a directory.
 */
i64 sys_rmdir(virt_t u_filename);

/*
 * sbrk
 * Increase (or allocate, if needed) a user-level heap.
 */
i64 sys_sbrk(i32 incr);

/*
 * fthread_create
 * Create a new thread within the current task.
 *
 * Our "fthread_t" is just a tid_t ptr (aka u64*).
 */
i64 sys_fthread_create(virt_t u_thread, u64 attr, virt_t u_start, virt_t u_arg);

/*
 * fthread_join
 * Wait for a specific thread to quit via fthread_exit.
 */
i64 sys_fthread_join(tid_t tid, virt_t u_val_ptr);

/*
 * fthread_switch
 * Switch from the current thread to a different thread in the same task
 * without performing a TLB flush, if that thread can be scheduled.
 */
i64 sys_fthread_switch(tid_t tid);

/*
 * fthread_exit
 * Quit from the current thread.
 */
i64 sys_fthread_exit(virt_t u_val_ptr);

/*
 * mprotect
 * Change the protection level of a given page of memory.
 */
i64 sys_mprotect(virt_t u_addr, size_t len, u64 prot_flags);

/*
 * fmap
 * mmap, but for fractal- mostly just an easy way to get a huge page.
 * n_copies: how many duplicated images to allocate in the VA space.
 * log_stride: log of the stride between duplicated pages (eg. 0 means 1 page, 1 means 2 pages, etc).
 */
i64 sys_fmap(virt_t u_newpgptr, usize n_copies, usize log_stride);

/*
 * xlate
 * Given a virtual address, translate it to a physical address.
 * va: a u64 pointing to some virtual address
 * u_pa: a pointer to where we should write the corresponding physical address
 */
i64 sys_xlate(u64 va, virt_t u_pa);

/*
 * funmap
 * Free a page from fmap.
 */
i64 sys_funmap(virt_t u_pgptr);

/*
 * gmap
 * General-purpose dispatcher for any gigamap related operations.
 * opt: the operation being performed
 * arg: (optional) the argument for that operation
 */
i64 sys_gmap(gigaopt_t opt, u64 arg);

/***************************************
 * Kernel-Callable Syscall Definitions *
 ***************************************/

// Kernel callable versions of the syscalls
// These methods are called by their sys_* counterparts
// They assume all buffers have already been copied into the kernel,
// so they are safe to call from kernel contexts.
// (eg. when a task is killed, use this to print "Segmentation Fault" to its stdout)
// These all take a task argument, but most of the time we assume the task
// is the current task. In some cases it may make sense to call one of these
// on behalf of a different task, though (like for do_sys_write, for example).
i64 do_sys_spawn(task_t *t, char *binpath, char **argv, SpawnMode mode);
i64 do_sys_quit(task_t *t, u64 reason);
i64 do_sys_read(task_t *t, u32 fd, u8 *buf, usize sz);
i64 do_sys_write(task_t *t, u32 fd, u8 *buf, usize sz);
i64 do_sys_close(task_t *t, u32 fd);
i64 do_sys_stat(task_t *t, u32 fd, struct stat *buf);
i64 do_sys_lseek(task_t *t, u32 fd, i32 offset, int whence);
i64 do_sys_mkdir(task_t *t, char *path);
i64 do_sys_chdir(task_t *t, char *new_wd);
i64 do_sys_open(task_t *t, char *path_in, u64 flags);
i64 do_sys_fork(task_t *t);
i64 do_sys_execve(task_t *t, char *path, char *argv[], char *envp[]);
i64 do_sys_wait(task_t *t, i64 *exitcode, u64 flags);
i64 do_sys_getdents(task_t *t, u32 fd, u8 *buf, usize sz);
i64 do_sys_pipe(task_t *t, i32 filedes[2]);
i64 do_sys_dup(task_t *t, u32 fd_from);
i64 do_sys_dup2(task_t *t, u32 fd_from, u32 fd_to);
i64 do_sys_fcntl(task_t *t, u32 fd, u64 cmd, u64 arg);
i64 do_sys_uname(task_t *t, struct utsname *unbuf);
i64 do_sys_fstatat(task_t *t, u32 fd, char *path, struct stat *buf, u32 flags);
i64 do_sys_openat(task_t *t, u32 fd, char *path_in, u64 flags);
i64 do_sys_fchdir(task_t *t, u32 fd);
i64 do_sys_access(task_t *t, char *path, u64 mode);
i64 do_sys_sysconf(task_t *t, u64 name);
i64 do_sys_unlink(task_t *t, char *path);
i64 do_sys_waitpid(task_t *t, pid_t p, i64 *exitcode, u64 flags);
i64 do_sys_getpid(task_t *t);
i64 do_sys_kexec(task_t *t, virt_t new_kernel_path);
i64 do_sys_fractal(task_t *t, u64 cmd, u64 arg);
i64 do_sys_select(task_t *t, i32 nfds, virt_t u_readfds, virt_t u_writefds, virt_t u_errfds, virt_t u_timeout);
i64 do_sys_poll(task_t *t, virt_t u_fds, u32 nfds, i32 timeout);
i64 do_sys_tcgetattr(task_t *t, i32 fd, virt_t u_termios_p);
i64 do_sys_tcsetattr(task_t *t, i32 fd, i32 optional_actions, virt_t u_termios_p);
i64 do_sys_rmdir(task_t *t, char *path);
i64 do_sys_sbrk(task_t *t, i32 incr);
// We break the pattern of do_sys_X for fthreads
i64 do_sys_mprotect(task_t *t, virt_t u_addr, size_t len, u64 prot_flags);
i64 do_sys_fmap(task_t *t, virt_t u_newpgptr, usize n_copies, usize log_stride);
i64 do_sys_xlate(task_t *t, u64 va, virt_t u_pa);
i64 do_sys_funmap(task_t *t, virt_t u_pgptr);
i64 do_sys_gmap(task_t *t, gigaopt_t opt, u64 arg);

/****************************
 * Shared-Utility Functions *
 ****************************/

END_C_HEADER

#ifdef ARCH_X86
#include <arch/x86/syscall.h>
#endif // ARCH_X86

#ifdef ARCH_ARM
#include <arch/arm/syscall.h>
#endif // ARCH_ARM

#ifdef ARCH_RISCV
#include <arch/rv/syscall.h>
#endif // ARCH_RISCV

#endif // SYSCALL_H
