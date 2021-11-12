#ifndef ENJOY_X_H
#define ENJOY_X_H
#include <X11/Xlib.h>
Display *init_x();
void close_x(Display *disp);
void *motion_thread_x(void * disp);
void fake_key_x(Display *disp, char *value, Bool state);
void fake_mouse_button_x(Display *disp, int button, Bool state);
#endif
