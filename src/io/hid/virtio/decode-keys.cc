#include <fractal.h>
#include <io/hid/decode-keys.h>
#include <io/hid/input.h>
#include <io/hid/virtio-hid.h>
#include <ascii.h>

// CHAR_TO_SHIFT(c,s)
// c: the character (ASCII) of the letter pressed, if shift not pressed
// s: the character (ASCII) to change it to, if shift is pressed
#define CHAR_TO_SHIFT(c,s) case c: return s; break;

char decode_virtio_keypress(struct virtio_input_event *ev, fractal_keyboard_state_t *keystate) {
    if (!ev) return '\x00';
    if (!keystate) return '\x00';
    if (ev->code >= VIRTIO_KEYMAP_SIZE) return '\x00';

    char decoded = virtio_keymap[ev->code];

    if (ev->code == KEY_LEFTSHIFT || ev->code == KEY_RIGHTSHIFT) {
        keystate->shift_pressed = !keystate->shift_pressed;
        return decoded;
    }

    if (is_upper_alphabet_char(decoded)) {
        if (!keystate->shift_pressed) { decoded += TO_LOWERCASE; }
        return decoded;
    }

    // Handle weird chars / shift chars
    if (keystate->shift_pressed) {
        switch(decoded) {
            CHAR_TO_SHIFT('1','!')
            CHAR_TO_SHIFT('2','@')
            CHAR_TO_SHIFT('3','#')
            CHAR_TO_SHIFT('4','$')
            CHAR_TO_SHIFT('5','%')
            CHAR_TO_SHIFT('6','^')
            CHAR_TO_SHIFT('7','&')
            CHAR_TO_SHIFT('8','*')
            CHAR_TO_SHIFT('9','(')
            CHAR_TO_SHIFT('0',')')
            CHAR_TO_SHIFT('-','_')
            CHAR_TO_SHIFT('=','+')
            CHAR_TO_SHIFT('[','{')
            CHAR_TO_SHIFT(']','}')
            CHAR_TO_SHIFT(',','<')
            CHAR_TO_SHIFT('.','>')
            CHAR_TO_SHIFT('/','?')
            CHAR_TO_SHIFT('`','~')
            CHAR_TO_SHIFT(';',':')
            CHAR_TO_SHIFT('\'','"')
            CHAR_TO_SHIFT('\\','|')
        }
    }

    return decoded;
}
