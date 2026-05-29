#ifndef WINDOW_H
#define WINDOW_H

#include <fractal.h>
#include <oxide.h>

#define WINDOW_NAME_MAX 64

// We assume the border images are WINDOW_BORDER_IMAGE_WIDTH x WINDOW_BORDER_IMAGE_WIDTH squares
#define WINDOW_BORDER_IMAGE_WIDTH 32

// How many pixels wide is the radius inside of the border image bins?
#define WINDOW_BORDER_RADUS 12

#define DARK_MODE_WINDOW_DEFAULT_DARKEN_NUMERATOR 1
#define DARK_MODE_WINDOW_DEFAULT_DARKEN_DENOMINATOR 1
#define DARK_MODE_WINDOW_DARKEN_FACTOR_INACTIVE 2

#define LIGHT_MODE_WINDOW_DARKEN_NUMERATOR 2
#define LIGHT_MODE_WINDOW_DARKEN_DENOMINATOR 1
#define LIGHT_MODE_WINDOW_DARKEN_FACTOR_INACTIVE 1

#define WINDOW_TITLE_PADDING 4
#define WINDOW_INNER_PADDING 16
#define WINDOW_BORDER_COLOR 0xAAAAAA

#define XBUTTON_W 16
#define XBUTTON_H 16
extern u32 xbutton[XBUTTON_H * XBUTTON_W];
extern u32 xbutton_hover[XBUTTON_H * XBUTTON_W];

#define WINDOW_INIT_X 10
#define WINDOW_INIT_Y 40

extern font_t *window_font;

class Window {
public:
    rect_t bounds;
    rect_t min_bounds; // Minimum window size, resize will limit width and height to this
    rect_t xbutton_bounds;
    bool is_closing = false; // Set by the window when the compositor should close it next frame
    char window_name[WINDOW_NAME_MAX];

    // When the window is active, we multiply the background blur color by
    // the fraction (active_darken_numerator / active_darken_denominator)
    // All inactive windows use the inactive darken factor, defined above
    i32 active_darken_numerator, active_darken_denominator;

    Window() : is_closing(false),
               active_darken_numerator(DARK_MODE_WINDOW_DEFAULT_DARKEN_NUMERATOR),
               active_darken_denominator(DARK_MODE_WINDOW_DEFAULT_DARKEN_DENOMINATOR) {}
    virtual ~Window() {}

    // High-level window draw call
    // This should paint the entire window, background and inner content, to the framebuffer
    // The default implementation of this uses PaintBox to draw a background & also draws
    // a close button using xbutton_bounds.
    virtual kret_t Paint(i32 hover_x, i32 hover_y, bool is_main_window);

    // Draw rounded rectangle for the bounds, following window shading rules
    // Windows that override Paint can still use this to draw the window background
    virtual kret_t PaintBox(bool is_main_window);

    // Windows that have actual content should override this:
    virtual kret_t DrawInnerContent(i32 hover_x, i32 hover_y, bool is_top);

    virtual kret_t Keypress(char c);

    // Outer window will service MouseClick, and call OnClick for
    // windows that override it. Client classes should override OnClick
    // but not MouseClick (unless they don't want to support the x button
    // and any other future window features).
    virtual kret_t MouseClick(i32 click_x, i32 click_y, bool pressed);

    virtual kret_t OnClick(i32 click_x, i32 click_y, bool pressed);

    // Called every time the compositor wants to resize this window
    virtual kret_t Resize(i32 new_w, i32 new_h);

    // Set this to true if you want the compositor to close you next frame
    virtual bool IsClosing();

    // Called when window is closing
    virtual void OnClose();
};

class SystemWindow : public Window {
public:
    SystemWindow();
    virtual kret_t DrawInnerContent(i32 hover_x, i32 hover_y, bool) override;
};

class Settings : public Window {
public:
    rect_t toggle_button, lowres, midres, hires;
    Settings();
    virtual kret_t DrawInnerContent(i32 hover_x, i32 hover_y, bool) override;
    virtual kret_t OnClick(i32 x, i32 y, bool pressed) override;
};

class Dock : public Window {
public:
    virtual kret_t Paint(i32 hover_x, i32 hover_y, bool) override;
    virtual kret_t MouseClick(i32, i32, bool) override;
};

class TopBar : public Window {
public:
    virtual kret_t Paint(i32 hover_x, i32 hover_y, bool) override;
};

class Menu : public Window {
public:
    Menu();
    virtual kret_t OnClick(i32 click_x, i32 click_y, bool pressed) override;
    virtual kret_t DrawInnerContent(i32 hover_x, i32 hover_y, bool) override;
};

#define DOCK_HEIGHT (100 + 2 * WINDOW_INNER_PADDING)

#endif // WINDOW_H
