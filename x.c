#include <X11/Xlib.h>
#include <X11/extensions/XTest.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <string.h>
#include <ctype.h>

#include "x.h"
#include "keytable.h"

extern int debug_mode;

extern int axis_x_direction;
extern int axis_y_direction;
extern int motion_interval;

void *motion_thread_x(void * disp) {
    if(debug_mode)
        fprintf(stderr, "Xtst motion thread\n");
    XTestGrabControl(disp, True);
    while(axis_x_direction != 0 || axis_y_direction != 0) {
        XTestFakeRelativeMotionEvent(disp, axis_x_direction, axis_y_direction, CurrentTime);
        XFlush(disp);
        /* motion acceleration */
        usleep((motion_interval > 1000) ? motion_interval -= 200 : motion_interval);
    }
    XTestGrabControl(disp, False);
    return NULL;
}

/* convert string to keycode */
KeyCode str2key(Display *disp, char *keystr)
{
    int key = XStringToKeysym(keystr);

    if(key == NoSymbol) {
        fprintf(stderr, "Error key format: %s\n", keystr);
        return 0;
    }
    if(disp == NULL)
        return 0;
    return XKeysymToKeycode (disp, key);
}

/* support combined keys, for examples: Super_L+Shift_L+q */
void fake_key_x(Display *disp, char *value, Bool state)
{
    char *keys = malloc(strlen(value)+1);
    strcpy(keys, value);
    char *end_token;
    char *token = strtok_r(keys, "+", &end_token);
    // loop through the string to extract all other tokens
    while( token != NULL ) {
        keymap *km = get_keymap_by_name(token);
        if(km->name != NULL) {
            if(debug_mode)
                fprintf(stderr, "x: %s, %s, %d, %d\n", km->name, km->xkeyname, km->uinpcode, state);
            KeyCode kc = str2key(disp, km->xkeyname);
            if(kc == 0) {
                fprintf(stderr, "Wrong keyname: %s\n", token);
                return;
            }
            XTestFakeKeyEvent (disp, kc, state, 0);
            XFlush(disp);
        }
        free(km->name);
        free(km->xkeyname);
        free(km);
        token = strtok_r(NULL, "+", &end_token);
    }
    free(keys);
}

void fake_mouse_button_x(Display *disp, int button, Bool state)
{
     XTestFakeButtonEvent (disp, button, state,  CurrentTime);
     XFlush(disp);
}

