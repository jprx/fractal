#ifndef ASCII_H
#define ASCII_H

#include <fractal.h>

BEGIN_C_HEADER

#define ASCII_A 'A'
#define ASCII_Z 'Z'

#define ASCII_LOWER_A 'a'
#define ASCII_LOWER_Z 'z'

#define TO_LOWERCASE 0x20
#define TO_UPPERCASE 0x20

static inline bool is_upper_alphabet_char(char c) { return (c >= ASCII_A && c <= ASCII_Z); }
static inline bool is_lower_alphabet_char(char c) { return (c >= ASCII_LOWER_A && c <= ASCII_LOWER_Z); }

END_C_HEADER

#endif // ASCII_H
