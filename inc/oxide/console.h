#ifndef CONSOLE_H
#define CONSOLE_H

#include <fractal.h>
#include <oxide/window.h>
#include <task.h>
#include <page_alloc.h>
#include <tty.h>

class Console : public Window {
public:
    Console();
    virtual ~Console() override;
    char *inner_buf;
    TTYBuffer tty;
    usize cursor;
    // Cursor can go back up until backspace_backstop
    usize backspace_backstop;
    task_t *assoc_task;
    virtual kret_t DrawInnerContent(i32 hover_x, i32 hover_y, bool) override;

    // Compositor sent us a key press:
    virtual kret_t Keypress(char c) override;

    // The associated task sent us data:
    virtual void ReceiveData(char *buf, usize sz);

    // Add data to be displayed without any side effects
    virtual void AppendChar(char c);

    // Associated task is about to quit:
    virtual void AssocTaskQuitting(task_t *t, u64 code);

    void OnClose();
};

#endif // CONSOLE_H
