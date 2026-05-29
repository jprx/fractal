#ifndef DECODE_KEYS_H
#define DECODE_KEYS_H

#include <fractal.h>
#include <io/hid/input.h>

BEGIN_C_HEADER

// Only index into the keymap table if the code is less than this
#define VIRTIO_KEYMAP_SIZE 256

// Custom struct for tracking all state for an HID keyboard
// Eg. what keys are pressed etc.
typedef struct {
    bool shift_pressed;
    bool ctrl_pressed;
} fractal_keyboard_state_t;

struct virtio_input_event;
char decode_virtio_keypress(struct virtio_input_event *ev, fractal_keyboard_state_t *keystate);

extern const char virtio_keymap[VIRTIO_KEYMAP_SIZE];

END_C_HEADER

#endif // DECODE_KEYS_H
