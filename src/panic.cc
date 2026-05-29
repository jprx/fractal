#include <out.h>
#include <types.h>
#include <stdarg.h>
#include <lib/mem.h>
#include <regs.h>
#include <oxide.h>
#include <board.h>
#include <io/gpu/virtio-gpu.h>

typedef void (*putc_fn_t)(char c);
typedef void (*puts_fn_t)(char *s);
extern "C" kret_t _vprintf_internal(char *format, putc_fn_t putc_cb, puts_fn_t puts_cb, va_list *args);
extern "C" usize _snprintf_internal(char *outs, char *format, va_list *args);

extern "C" void __disable_global_interrupts();

extern FractalGPU *global_gpu;

/*
 * panic
 * Unrecoverable error- call this if there
 * is no other possible option.
 */
extern "C" __attribute__((noreturn)) void _panic_internal(regs_t *regs, char *reason, ...) {
    va_list args;
    char oxide_panic_reason_str[2048];
    va_start(args, reason);
    __global_out_puts("panic: ");
    _snprintf_internal(
        oxide_panic_reason_str,
        reason,
        &args
    );
    printf("%s", oxide_panic_reason_str);
    __global_out_puts("\r\n");
    if (current_thread()) printf("task: %s\r\n", current_thread()->t_task->t_name);
    va_end(args);

    dump_regs(regs);
    __disable_global_interrupts();

#ifdef BOARD_HAS_REBOOT
    printf("trying to reboot..\n");
    extern void __board_reboot__();
    __board_reboot__();
#endif // BOARD_HAS_REBOOT

#ifdef USE_GRAPHICS
    u32 orig_screen_w, orig_screen_h;
    bool queried_screensize = false;
#endif // USE_GRAPHICS

    while(true) {
#ifdef USE_GRAPHICS

    if (NULL == global_gpu) while(true);

    // Support resizing the panic screen if the window is adjusted
    struct virtio_gpu_rect cur_screen_dim;
    oxide_panic(oxide_panic_reason_str);

    global_gpu->QueryScreensize(&cur_screen_dim);

    if (cur_screen_dim.width > SCREEN_MAX_W) cur_screen_dim.width = SCREEN_MAX_W;
    if (cur_screen_dim.height > SCREEN_MAX_H) cur_screen_dim.height = SCREEN_MAX_H;

    if (!queried_screensize) {
        orig_screen_w = cur_screen_dim.width;
        orig_screen_h = cur_screen_dim.height;
        queried_screensize = true;
    }

    if (orig_screen_w != cur_screen_dim.width) {
        SCREEN_W = cur_screen_dim.width;
    }

    if (orig_screen_h != cur_screen_dim.height) {
        SCREEN_H = cur_screen_dim.height;
    }

#endif // USE_GRAPHICS
    }
}
