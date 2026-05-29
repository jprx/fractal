#include <fractal.h>
#include <io/hid/ps2-hid.h>
#include <io/mouse.h>
#include <oxide.h>
#include <io/gpu.h>

extern FractalGPU *global_gpu;

// These should roughly match the VESA framebuffer resolution
// Because mouse_x and mouse_y are relative to the dimensions of the screen
// (If they are scaled the same in both directions, X will feel faster than Y)
#define MOUSE_SCALE_FACTOR_X 27
#define MOUSE_SCALE_FACTOR_Y 36

#define PS2_MOUSE_CMD_PACKET_BUTTON_LEFT   BIT(0)
#define PS2_MOUSE_CMD_PACKET_BUTTON_RIGHT  BIT(1)
#define PS2_MOUSE_CMD_PACKET_BUTTON_MIDDLE BIT(2)

// See: http://users.utcluj.ro/~baruch/sie/labor/PS2/PS-2_Mouse_Interface.htm
// See: https://wiki.osdev.org/PS/2_Mouse

// Which state is the PS/2 mouse in?
// Each packet moves forward 1 modulo number of states
typedef enum ps2_mouse_state_t {
    OVERFLOW_BYTE = 0,
    X_DISPLACEMENT = 1,
    Y_DISPLACEMENT = 2,
    NUM_STATES_PS2_MOUSE
} mouse_state_t;

static bool left_click_down = false;

static mouse_state_t cur_state;

static usize last_mouse_interrupt_frame_idx = 0;

extern "C" void ps2_decode_mouse(i8 pkt) {
    bool was_left_click_down = left_click_down;

    // Automatically reset internal state counter after a few frames without interrupt
    if (last_mouse_interrupt_frame_idx + PS2_CONTROLLER_CONFIG_INTERRUPTS_ON < frame_idx) {
        cur_state = OVERFLOW_BYTE;
    }

    switch(cur_state) {
        case OVERFLOW_BYTE:
        left_click_down = (0 != ((u8)pkt & PS2_MOUSE_CMD_PACKET_BUTTON_LEFT));
        break;

        case X_DISPLACEMENT:
        mouse_x += MOUSE_SCALE_FACTOR_X * pkt;
        break;

        case Y_DISPLACEMENT:
        mouse_y -= MOUSE_SCALE_FACTOR_Y * pkt;
        break;
    }

    if (was_left_click_down != left_click_down) {
        mouse_click(left_click_down);
    }

    if (mouse_x < MOUSE_X_MIN) mouse_x = MOUSE_X_MIN;
    if (mouse_y < MOUSE_Y_MIN) mouse_y = MOUSE_Y_MIN;
    if (mouse_x > MOUSE_X_MAX) mouse_x = MOUSE_X_MAX;
    if (mouse_y > MOUSE_Y_MAX) mouse_y = MOUSE_Y_MAX;

    if (NUM_STATES_PS2_MOUSE - 1 == cur_state) {
        global_gpu->SetMouse(mouse_x, mouse_y);
    }

    cur_state = (mouse_state_t)(((usize)cur_state + 1) % NUM_STATES_PS2_MOUSE);

    last_mouse_interrupt_frame_idx = frame_idx;
}
