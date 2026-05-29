#include <fractal.h>
#include <syscall.h>
#include <spawn.h>
#include <oxide.h>
#include <oxide/window.h>
#include <oxide/console.h>
#include <task.h>
#include <stdio.h>
#include <filesys/fileio.h>
#include <oxide/font.h>

#define ATTACH_SHELL

static font_t *console_font = &font_mono;

#define FIXED_WIDTH_FONT_CHAR SPACE_REPLACE_CHAR

Console::Console() {
    bounds.x = WINDOW_INIT_X;
    bounds.y = WINDOW_INIT_Y;
    bounds.w = 820;
    bounds.h = 480;
    min_bounds.w = 300;
    min_bounds.h = 180;
    strncpy(window_name, "Console", WINDOW_NAME_MAX);
    active_darken_numerator = 1;
    active_darken_denominator = 3;
    inner_buf = (char *)AllocPage(SMALL_PAGE);
    if (!inner_buf) panic("Failed to allocate memory for this console");
    memset(inner_buf, '\x00', SMALL_PAGE_SIZE);
    cursor = 0;
    backspace_backstop = 0;

    assoc_task = NULL;
#ifdef ATTACH_SHELL
    char *args[TASK_MAX_NUM_ARGS];
    args[0] = INIT_TASK_NAME;
    args[1] = "-l";
    args[2] = NULL;
    assoc_task = spawn_anon_task(INIT_TASK, args, SPAWN_MODE_PREEMPTIVE | SPAWN_MODE_IMMEDIATE | SPAWN_MODE_USER_TASK);
    if (!assoc_task) panic("Failed to launch associated task for a console");
    assign_owner_to_task(assoc_task, this);
    init_console_stdio(assoc_task, this);
    schedule_task(assoc_task);
#endif // ATTACH_SHELL
}

Console::~Console() {
    FreePage((usize)inner_buf);
}

void Console::OnClose() {
    resource_is_quitting(this);
}

kret_t Console::DrawInnerContent(i32 hover_x, i32 hover_y, bool is_top) {
    usize max_rows = ((bounds.h - WINDOW_INNER_PADDING) / ((5 * console_font->char_max_height) / 7)) - 3;
    usize cur_x = bounds.x + WINDOW_INNER_PADDING;
    usize cur_y = bounds.y + WINDOW_INNER_PADDING + window_font->char_max_height;
    usize last_x = (bounds.x + bounds.w - WINDOW_INNER_PADDING);
    char *newline_scanner = inner_buf;
    usize n_rows = 0;

    // 1. Determine how many rows we need to draw
    while (*newline_scanner) {
        cur_x += console_font->char_width_lut[FIXED_WIDTH_FONT_CHAR];
        if (cur_x + console_font->char_width_lut[FIXED_WIDTH_FONT_CHAR] > last_x || *newline_scanner == '\n') {
            cur_x = bounds.x + WINDOW_INNER_PADDING;
            cur_y += (5 * console_font->char_max_height) / 7;
            n_rows++;
        }
        newline_scanner++;
    }

    // 2. Find the row to start drawing on
    char *start_cursor = NULL;
    if (n_rows > max_rows) {
        usize row_to_start = n_rows - max_rows;
        char *findrow_scanner = inner_buf;
        cur_x = bounds.x + WINDOW_INNER_PADDING;
        cur_y = bounds.y + WINDOW_INNER_PADDING + console_font->char_max_height;
        usize rows_encountered = 0;
        while (*findrow_scanner) {
            cur_x += console_font->char_width_lut[FIXED_WIDTH_FONT_CHAR];
            if (cur_x + console_font->char_width_lut[FIXED_WIDTH_FONT_CHAR] > last_x || *findrow_scanner == '\n') {
                if (rows_encountered == row_to_start) {
                    start_cursor = findrow_scanner;
                    break;
                }
                cur_x = bounds.x + WINDOW_INNER_PADDING;
                cur_y += (5 * console_font->char_max_height) / 7;
                rows_encountered++;
            }
            findrow_scanner++;
        }

    } else {
        start_cursor = inner_buf;
    }

    // 3. Begin drawing
    char *c = start_cursor;
    cur_x = bounds.x + WINDOW_INNER_PADDING;
    cur_y = bounds.y + WINDOW_INNER_PADDING + console_font->char_max_height;
    while (*c) {
        cur_x += console_font->char_width_lut[FIXED_WIDTH_FONT_CHAR];
        if (cur_x + console_font->char_width_lut[FIXED_WIDTH_FONT_CHAR] > last_x || *c == '\n') {
            cur_x = bounds.x + WINDOW_INNER_PADDING;
            cur_y += (5 * console_font->char_max_height) / 7;
        }
        oxide_draw_char(console_font, *c, cur_x, cur_y, 0xFFFFFF);
        c++;
    }
    cur_x += console_font->char_width_lut[FIXED_WIDTH_FONT_CHAR];
    oxide_draw_char(console_font, '_', cur_x, cur_y, 0xFFFFFF);
    return ALL_GOOD;
}

void Console::AppendChar(char c) {
    if (0 == c) return;
    if (cursor >= SMALL_PAGE_SIZE - 1) {
        // This is lazy and slow, @TODO: refactor this to be a ring buffer
        memcpy(inner_buf, &inner_buf[SMALL_PAGE_SIZE / 2], SMALL_PAGE_SIZE / 2);
        cursor = SMALL_PAGE_SIZE / 2 - 1;
    }
    inner_buf[cursor++] = c;
    inner_buf[cursor] = '\x00';
}

kret_t Console::Keypress(char c) {
    if (c == '\b') {
        if (cursor <= backspace_backstop) {
            return ALL_GOOD;
        }
    }

    tty.SendData(c);

    if (c == '\b') {
        inner_buf[--cursor] = '\x00';
        return ALL_GOOD;
    }
    AppendChar(c);

    return ALL_GOOD;
}

void Console::ReceiveData(char *buf, usize sz) {
    for (usize i = 0; i < sz; i++) {
        AppendChar(buf[i]);
        backspace_backstop = cursor;
    }
}

// Notification when a task whose console field matches us quits
// Only do anything if this is the assoc_task, the root anon task we started
void Console::AssocTaskQuitting(task_t *t, u64 reason) {
    char quitbuf[128];
    if (assoc_task != t) return;
    snprintf(quitbuf, "Task Exited: %d\r\n", reason);
    ReceiveData(quitbuf, strlen(quitbuf));
    assoc_task = NULL;
}
