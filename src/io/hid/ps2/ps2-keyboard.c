#include <fractal.h>
#include <io/hid/ps2-hid.h>
#include <io/hid/input.h>
#include <ascii.h>

#define NUM_PS2_CODES 256

#define PS2_LSHIFT ((0x12))
#define PS2_RSHIFT ((0x59))

// CHAR_TO_SHIFT(c,s)
// c: the character (ASCII) of the letter pressed, if shift not pressed
// s: the character (ASCII) to change it to, if shift is pressed
#define CHAR_TO_SHIFT(c,s) case c: return s; break;

// Scan Code Set 2
// See: https://wiki.osdev.org/PS/2_Keyboard
// See: "Application Note V-107 PS/2 PC Keyboard Scan Sets Translation Table" (http://www.vetra.com/scancodes.html)
static char ps2_to_ascii[NUM_PS2_CODES] = {
    [0x0E] = '`',
    [0x16] = '1',
    [0x1E] = '2',
    [0x26] = '3',
    [0x25] = '4',
    [0x2E] = '5',
    [0x36] = '6',
    [0x3D] = '7',
    [0x3E] = '8',
    [0x46] = '9',
    [0x45] = '0',
    [0x4E] = '-',
    [0x55] = '=',
    [0x66] = '\b',
    [0x0D] = '\t',
    [0x15] = 'q',
    [0x1D] = 'w',
    [0x24] = 'e',
    [0x2D] = 'r',
    [0x2C] = 't',
    [0x35] = 'y',
    [0x3C] = 'u',
    [0x43] = 'i',
    [0x44] = 'o',
    [0x4D] = 'p',
    [0x54] = '[',
    [0x5B] = ']',
    [0x1C] = 'a',
    [0x1B] = 's',
    [0x23] = 'd',
    [0x2B] = 'f',
    [0x34] = 'g',
    [0x33] = 'h',
    [0x3B] = 'j',
    [0x42] = 'k',
    [0x4B] = 'l',
    [0x4C] = ';',
    [0x52] = '\'',
    [0x5A] = '\n',
    [0x1A] = 'z',
    [0x22] = 'x',
    [0x21] = 'c',
    [0x2A] = 'v',
    [0x32] = 'b',
    [0x31] = 'n',
    [0x3A] = 'm',
    [0x41] = ',',
    [0x49] = '.',
    [0x4A] = '/',
    [0x29] = ' ',
    [0x5D] = '\\',
    [0x6B] = KEY_LEFT,
    [0x74] = KEY_RIGHT
};

#define PS2_RELEASE_CODE ((0xF0))

// Set to true when we see PS2_RELEASE_CODE,
// which means the next code is actually a key being released
static bool is_letting_go = false;

static bool lshift = false;
static bool rshift = false;

char ps2_decode_keypress(u8 code) {
    char decoded;

    if (PS2_RELEASE_CODE == code) {
        is_letting_go = true;
        return '\x00';
    }

    decoded = ps2_to_ascii[code];

    // Handle key releases
    if (code == PS2_LSHIFT) lshift = !is_letting_go;
    if (code == PS2_RSHIFT) rshift = !is_letting_go;

    // From here on out we ignore release packets,
    // so if we were letting go, exit early
    if (is_letting_go) {
        is_letting_go = false;
        return '\x00';
    }

    if (is_lower_alphabet_char(decoded)) {
        if (lshift || rshift) { decoded -= TO_UPPERCASE; }
        return decoded;
    }

    // Handle weird chars / shift chars
    if (lshift || rshift) {
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
