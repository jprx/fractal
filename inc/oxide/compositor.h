#ifndef OXIDE_COMPOSITOR_H
#define OXIDE_COMPOSITOR_H

#include <fractal.h>

class Window;

class Compositor {
public:
	bool dragging_a_window = false;
	bool resizing_a_window = false;
	bool use_blur = true;
	i32 drag_offset_x, drag_offset_y;
	i32 resize_offset_w, resize_offset_h;
	i32 hover_x, hover_y;
	bool mouse_moved = false;
	bool blur_needs_repaint = true; // Trigger repaint of foreground blurbuf
	bool will_repaint_blur = false;
	bool background_changed = true; // Happens 1 frame after background_will_change is true
	bool background_will_change = false; // Trigger repaint of background blurbuf
	void Draw();
	void Initialize();
	void SendKey(char c);
	void MouseMoved();
	void MouseClick(bool);
	void LaunchWindow(usize);
	void CloseWindow(Window *w);

	// Returns false if still playing, true when done
	bool BootAnimation();
};

#endif // OXIDE_COMPOSITOR_H
