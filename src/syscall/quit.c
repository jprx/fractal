#include <task.h>
#include <regs.h>
#include <out.h>
i64 do_sys_quit(u64 reason) {
    quit_task(current_task(), reason);
    panic("[quit] We should never get here");
    return 0;
}
i64 sys_quit(u64 reason) {
    return do_sys_quit(reason);
}
