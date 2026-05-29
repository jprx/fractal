#include <fractal.h>
#include <io/mouse.h>
#include <oxide/compositor.h>

i32 mouse_x = 0;
i32 mouse_y = 0;
bool mouse_currently_pressed = false;

extern Compositor global_compositor;

extern "C" void mouse_click(bool pressed) {
    mouse_currently_pressed = pressed;

    // Notify all services waiting for mouse interrupts
    // Be careful as we are currently in an interrupt context!
    // (Don't do too much work)
    global_compositor.MouseClick(pressed);
}

extern "C" void mouse_moved() {
    global_compositor.MouseMoved();
}
