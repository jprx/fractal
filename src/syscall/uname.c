#include <fractal.h>
#include <syscall.h>
#include <copy_task.h>
#include <version.h>

i64 do_sys_uname(task_t *t, struct utsname *unbuf) {
    if (!t) return -1;
    if (!unbuf) return -2;

    strncpy(unbuf->sysname, KERNEL_NAME, _UTSNAME_LENGTH);
    strncpy(unbuf->nodename, KERNEL_VARIANT, _UTSNAME_LENGTH);
    strncpy(unbuf->release, fractal_version_num_str, _UTSNAME_LENGTH);
    strncpy(unbuf->version, fractal_version_str, _UTSNAME_LENGTH);

#ifdef BOARD_APPLE
    #include <board/arm/apple/soc.h>
    strncpy(unbuf->machine, BOARD_NAME, _UTSNAME_LENGTH);
#else // BOARD_APPLE
    strncpy(unbuf->machine, ARCH_STRING, _UTSNAME_LENGTH);
#endif // ! BOARD_APPLE

    return 0;
}

i64 sys_uname(virt_t unamebuf) {
    i64 rval;
    struct utsname n;
    rval = do_sys_uname(current_task(), &n);
    if (0 != rval) return rval;

    if (ALL_GOOD != copy_to_task(current_task(), unamebuf, &n, sizeof(n))) {
        return -1;
    }

    return 0;
}
