#ifndef MOUSE_H
#define MOUSE_H

#include <fractal.h>

BEGIN_C_HEADER

// Called from an interrupt context
// whenever a driver detects a mouse click:
void mouse_click(bool pressed);

// Called from an interrupt context
// whenever a driver detects the mouse moved:
void mouse_moved();

// Current mouse coords
extern i32 mouse_x, mouse_y;
extern bool mouse_currently_pressed;

// Absolute mouse coords at 1920x1080:
#define MOUSE_X_MIN 0
#define MOUSE_X_MAX 32715
#define MOUSE_Y_MIN 0
#define MOUSE_Y_MAX 32584

#define MOUSE_TO_SCREEN_X(m) (((m * SCREEN_W) / MOUSE_X_MAX))
#define MOUSE_TO_SCREEN_Y(m) (((m * SCREEN_H) / MOUSE_Y_MAX))

END_C_HEADER

#endif // MOUSE_H
