#include <fractal.h>
#include <out.h>
#include <types.h>
#include <stdarg.h>
#include <lib/mem.h>

#define CHARS_PER_U64 16
#define TOP_CHAR_BITMASK 0xF000000000000000
#define TOP_CHAR_SHIFT 60ull
#define NEXT_CHAR_SHIFT 4ull

#define HIGHEST_PRINTABLE_NUMBER 9
#define TO_ASCII_NUMBER 0x30
#define TO_ASCII_LETTER ((0x41 - 0x0A))

/* TMPBUF is used to store intermediate before reversing them */
#define TMPBUF_LEN 64

#define DECIMAL_MODULUS 10

typedef void (*putc_fn_t)(char c);
typedef void (*puts_fn_t)(char *s);

kret_t _vprintf_internal(char *format, putc_fn_t putc_cb, puts_fn_t puts_cb, va_list *args);
usize _snprintf_internal(char *outs, char *format, va_list *args);

kret_t printf(char *format, ...) {
    kret_t rval;
    va_list args;
    va_start(args,format);
    rval = _vprintf_internal(
        format,
        __global_out_putc,
        __global_out_puts,
        &args
    );
    va_end(args);
    return rval;
}

// @TODO: Take output string length as an argument
usize snprintf(char *outs, char* format, ...) {
    usize rval;
    va_list args;
    va_start(args,format);
    rval = _snprintf_internal(
        outs,
        format,
        &args
    );
    va_end(args);
    return rval;
}

/*
 * _vprintf_internal
 * A virtual printf that calls two generic callback functions
 * in place of putc and puts.
 *
 * A consumer of this function can define their own putc and
 * puts functions and printf will work for their target.
 */
kret_t _vprintf_internal(char *format, putc_fn_t putc_cb, puts_fn_t puts_cb, va_list *args) {
    char tmpbuf[TMPBUF_LEN];
    bool is_special_code = false;

    for (char *i = format; *i != '\x00'; i++) {
        if (is_special_code) {
            if (*i == 'x' || *i == 'X') {
                // Strategy:
                // 1. Grab top 4 bits of number, print
                // 2. Shift number by 4 bits
                // 3. Repeat

                u64 val = va_arg(*args, u64);

                for (usize i = 0; i < CHARS_PER_U64; i++) {
                    u8 cur_byte = (val & TOP_CHAR_BITMASK) >> TOP_CHAR_SHIFT;
                    if (cur_byte <= HIGHEST_PRINTABLE_NUMBER) putc_cb(cur_byte + TO_ASCII_NUMBER);
                    else putc_cb(cur_byte + TO_ASCII_LETTER);
                    val <<= NEXT_CHAR_SHIFT;
                }
            }
            else if (*i == 's' || *i == 'S') {
                char *cursor = va_arg(*args, char*);
                puts_cb(cursor);
            }
            else if (*i == 'c' || *i == 'C') {
                i32 c = va_arg(*args, i32);
                putc_cb(c);
            } else if (*i == 'd') {
                // Strategy:
                // 1. If zero, print '0' and quit, else setup tmpbuf
                // 2. If negative, store '-' in tmpbuf and *=-1 val
                // 3. While value is nonzero, store value modulo 10 in *tmpbuf++
                // 4. Divide val by 10 and continue to 4 until value is zero
                // 5. Flip tmpbuf around and print it out
                i32 val = va_arg(*args, i32);

                if (0 == val) {
                    putc_cb('0');
                }
                else {
                    char *tmpbuf_ptr = tmpbuf;

                    if (val < 0) {
                        val *= -1;
                        putc_cb('-');
                    }

                    while (val != 0) {
                        i32 cur_val_modulo = val % DECIMAL_MODULUS;
                        if (cur_val_modulo < 0) cur_val_modulo *= -1;
                        *tmpbuf_ptr++ = cur_val_modulo + TO_ASCII_NUMBER;
                        val /= DECIMAL_MODULUS;
                    }

                    for (char *cursor = tmpbuf_ptr-1; cursor != tmpbuf - 1; cursor--) {
                        putc_cb(*cursor);
                    }
                }
            } else if (*i == 'l' && *++i == 'd') {
                // Same as above but for 64 bit int
                i64 val = va_arg(*args, i64);

                if (0 == val) {
                    putc_cb(0 + TO_ASCII_NUMBER);
                }
                else {
                    char *tmpbuf_ptr = tmpbuf;

                    if (val < 0) {
                        val *= -1;
                        putc_cb('-');
                    }

                    while (val != 0) {
                        i64 cur_val_modulo = val % DECIMAL_MODULUS;
                        if (cur_val_modulo < 0) cur_val_modulo *= -1;
                        *tmpbuf_ptr++ = cur_val_modulo + TO_ASCII_NUMBER;
                        val /= DECIMAL_MODULUS;
                    }

                    for (char *cursor = tmpbuf_ptr-1; cursor != tmpbuf - 1; cursor--) {
                        putc_cb(*cursor);
                    }
                }
            } else {
                // We don't know this character, so move onto next arg
                putc_cb(*i);
                va_arg(*args, usize);
            }

            is_special_code = false;
            continue;
        }

        if (*i == '%') {
            is_special_code = true;
            continue;
        }

        putc_cb(*i);
    }

    return ALL_GOOD;
}

/*
 * _snprintf_internal
 * @TODO: Combine this with _vprintf_internal
 */
usize _snprintf_internal(char *outs, char *format, va_list *args) {
    char *orig_outs = outs;
    char tmpbuf[TMPBUF_LEN];
    bool is_special_code = false;

    for (char *i = format; *i != '\x00'; i++) {
        if (is_special_code) {
            if (*i == 'x' || *i == 'X') {
                // Strategy:
                // 1. Grab top 4 bits of number, print
                // 2. Shift number by 4 bits
                // 3. Repeat

                u64 val = va_arg(*args, u64);

                for (usize i = 0; i < CHARS_PER_U64; i++) {
                    u8 cur_byte = (val & TOP_CHAR_BITMASK) >> TOP_CHAR_SHIFT;
                    if (cur_byte <= HIGHEST_PRINTABLE_NUMBER) *(outs++) = (cur_byte + TO_ASCII_NUMBER);
                    else *(outs++) = (cur_byte + TO_ASCII_LETTER);
                    val <<= NEXT_CHAR_SHIFT;
                }
            }
            else if (*i == 's' || *i == 'S') {
                char *cursor = va_arg(*args, char*);
                while (*cursor) {
                    *(outs++) = *cursor;
                    cursor++;
                }
            }
            else if (*i == 'c' || *i == 'C') {
                i32 c = va_arg(*args, i32);
                *(outs++) = (c);
            } else if (*i == 'd') {
                // Strategy:
                // 1. If zero, print '0' and quit, else setup tmpbuf
                // 2. If negative, store '-' in tmpbuf and *=-1 val
                // 3. While value is nonzero, store value modulo 10 in *tmpbuf++
                // 4. Divide val by 10 and continue to 4 until value is zero
                // 5. Flip tmpbuf around and print it out
                i32 val = va_arg(*args, i32);

                if (0 == val) {
                    *(outs++) = ('0');
                }
                else {
                    char *tmpbuf_ptr = tmpbuf;

                    if (val < 0) {
                        val *= -1;
                        *(outs++) = ('-');
                    }

                    while (val != 0) {
                        i32 cur_val_modulo = val % DECIMAL_MODULUS;
                        if (cur_val_modulo < 0) cur_val_modulo *= -1;
                        *tmpbuf_ptr++ = cur_val_modulo + TO_ASCII_NUMBER;
                        val /= DECIMAL_MODULUS;
                    }

                    for (char *cursor = tmpbuf_ptr-1; cursor != tmpbuf - 1; cursor--) {
                        *(outs++) = (*cursor);
                    }
                }
            } else if (*i == 'l' && *++i == 'd') {
                // Same as above but for 64 bit int
                i64 val = va_arg(*args, i64);

                if (0 == val) {
                    *(outs++) = (0 + TO_ASCII_NUMBER);
                }
                else {
                    char *tmpbuf_ptr = tmpbuf;

                    if (val < 0) {
                        val *= -1;
                        *(outs++) = ('-');
                    }

                    while (val != 0) {
                        i64 cur_val_modulo = val % DECIMAL_MODULUS;
                        if (cur_val_modulo < 0) cur_val_modulo *= -1;
                        *tmpbuf_ptr++ = cur_val_modulo + TO_ASCII_NUMBER;
                        val /= DECIMAL_MODULUS;
                    }

                    for (char *cursor = tmpbuf_ptr-1; cursor != tmpbuf - 1; cursor--) {
                        *(outs++) = (*cursor);
                    }
                }
            } else {
                // We don't know this character, so move onto next arg
                *(outs++) = (*i);
                va_arg(*args, usize);
            }

            is_special_code = false;
            continue;
        }

        if (*i == '%') {
            is_special_code = true;
            continue;
        }

        *(outs++) = (*i);
    }
    // NULL terminator:
    *(outs++) = '\x00';

    return (usize)(outs - orig_outs);
}
