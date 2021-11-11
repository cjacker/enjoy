/*
Author: Cjacker <cjacker@foxmail.com>

Description:
Convert joystick events to mouse or key events.

Compile:
gcc -Os enjoy.c -o enjoy

Run:
./enjoy
*/

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <string.h>
#include <ctype.h>
#include <pthread.h>

#include <linux/joystick.h>
#include <X11/Xlib.h>
#include <X11/extensions/XTest.h>

int axis_up_press = 0;
int axis_down_press = 0;
int axis_left_press = 0;
int axis_right_press = 0;

int axis_x_direction = 0;
int axis_y_direction = 0;

#define MOTION_INTERVAL_INIT 8000
/* mouse motion intervals */
int motion_interval = MOTION_INTERVAL_INIT;

Bool motion_thread_created = False;

pthread_t motion_thread_t;

/* structure to store configurations */
typedef struct config {
    char *device;
    int axis_as_mouse;
    char *button_0;
    char *button_1;
    char *button_2;
    char *button_3;
    char *button_4;
    char *button_5;
    char *button_6;
    char *button_7;
    char *button_8;
    char *button_9;
    char *axis_up;
    char *axis_down;
    char *axis_left;
    char *axis_right;
} config;

/* default configuration */
config *create_default_config()
{
    config *p = malloc(sizeof(struct config));
    p->device = "/dev/input/js0";
    p->axis_as_mouse = 1;
    p->button_0 = "Super_L";
    p->button_1 = "mouse_button_3";
    p->button_2 = "mouse_button_1";
    p->button_3 = "Control_L";
    p->button_4 = NULL;
    p->button_5 = NULL;
    p->button_6 = NULL;
    p->button_7 = NULL;
    p->button_8 = "Super_L+End";
    p->button_9 = "Super_L+d";
    p->axis_up = "Up";
    p->axis_down = "Down";
    p->axis_left = "Left";
    p->axis_right = "Right";
    return p;
}

void *motion_thread(void * disp) {
    XTestGrabControl(disp, True);
    while(axis_x_direction != 0 || axis_y_direction != 0) {
        XTestFakeRelativeMotionEvent(disp, axis_x_direction, axis_y_direction, CurrentTime);
        XFlush(disp);
        /* motion acceleration */
        usleep((motion_interval > 1000) ? motion_interval -= 200 : motion_interval);
    }
    XTestGrabControl(disp, False);
}

/* state: 'True' for press, 'False' for release */
void fake_key(Display *disp, KeyCode keycode, Bool state)
{
    if(keycode == 0)
        return;
    XTestFakeKeyEvent (disp, keycode, state, 0);
    XFlush(disp);
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

/* process combined key */
void fake_key_sequence(Display *disp, char *string, Bool state)
{
    char * token = strtok(strdup(string), "+");
    // loop through the string to extract all other tokens
    while( token != NULL ) {
        //printf( " %s\n", token ); //printing each token
        fake_key(disp, str2key(disp, token), state);
        token = strtok(NULL, "+");
    }
} 

void fake_mouse_button(Display *disp, int button, Bool state)
{
     /* Fake the mouse button Press and Release events */
     XTestFakeButtonEvent (disp, button, state,  CurrentTime);
     XFlush(disp);
}

/* mouse_button_<n>: string + 13 is 'n' */
void fake_event(Display *disp, char *string, Bool state)
{
    if(string == NULL || strlen(string) == 0)
        return;

    if(strncasecmp (string, "exec ", 5) != 0) {
        return;    
    }

    if(strstr(string, "mouse_button_")) {
        string += 13;
        if(strstr("0123456789", string)) /* make sure it is a 'number' */
            fake_mouse_button(disp, atoi(string), state);
    } else
        fake_key_sequence(disp, string, state);
}

/* read joystick event, 0 success, -1 failed */
int read_event(int fd, struct js_event *event)
{
    ssize_t bytes;
    bytes = read(fd, event, sizeof(*event));
    if (bytes == sizeof(*event))
        return 0;
    /* Error, could not read full event. */
    return -1;
}

/* the number of axes on the controller or 0 if an error occurs. */
size_t get_axis_count(int fd)
{
    __u8 axes;
    if (ioctl(fd, JSIOCGAXES, &axes) == -1)
        return 0;
    return axes;
}

/* the number of buttons on the controller or 0 if an error occurs. */
size_t get_button_count(int fd)
{
    __u8 buttons;
    if (ioctl(fd, JSIOCGBUTTONS, &buttons) == -1)
        return 0;
    return buttons;
}

/* Current state of an axis. */
struct axis_state {
    short x, y;
};

/* Keeps track of the current axis state.

NOTE: This function assumes that axes are numbered starting from 0, and that
the X axis is an even number, and the Y axis is an odd number. However, this
is usually a safe assumption.

Returns the axis that the event indicated. */
size_t get_axis_state(struct js_event *event, struct axis_state axes[3])
{
    size_t axis = event->number / 2;

    if (axis < 3) {
        if (event->number % 2 == 0)
            axes[axis].x = event->value;
        else
            axes[axis].y = event->value;
    }

    return axis;
}


/* helper func for parse config file */
char *trim(char *str)
{
    char *start = str;
    char *end = str + strlen(str);

    while(*start && isspace(*start))
        start++;

    while(end > start && isspace(*(end - 1)))
        end--;

    *end = '\0';
    return start;
}

/* helper func for parse config file */
int parse_line(char *line, char **key, char **value)
{
    char *ptr = strchr(line, '=');
    if (ptr == NULL)
        return -1;

    *ptr++ = '\0';
    *key = trim(line);
    *value = trim(ptr);

    return 0;
}

/* print out config */
void print_config(config *conf)
{
    printf("device=%s\n", conf->device);
    printf("button_0=%s\n", conf->button_0?conf->button_0:"");
    printf("button_1=%s\n", conf->button_1?conf->button_1:"");
    printf("button_2=%s\n", conf->button_2?conf->button_2:"");
    printf("button_3=%s\n", conf->button_3?conf->button_3:"");
    printf("button_4=%s\n", conf->button_4?conf->button_4:"");
    printf("button_5=%s\n", conf->button_5?conf->button_5:"");
    printf("button_6=%s\n", conf->button_6?conf->button_6:"");
    printf("button_7=%s\n", conf->button_7?conf->button_7:"");
    printf("button_8=%s\n", conf->button_8?conf->button_8:"");
    printf("button_9=%s\n", conf->button_9?conf->button_9:"");
    printf("axis_as_mouse=%d\n", conf->axis_as_mouse);
    printf("axis_up=%s\n", conf->axis_up);
    printf("axis_down=%s\n", conf->axis_down);
    printf("axis_left=%s\n", conf->axis_left);
    printf("axis_right=%s\n", conf->axis_right);
}


/* show help infomation */
void help(config *default_config)
{
    printf("enjoy - Read joystick events and convert to mouse/key event\n");
    printf("        By cjacker <cjacker@foxmail.com>\n\n");
    printf("the default config set to:\n\n");
    print_config(default_config);
    printf("\n");
    printf("For devterm: x is button_0, a is button_1, b is button_2, y is button_3,\n");
    printf("             select is button_8, start is button_9\n");
    printf("\nyou can create your own config as '~/.enjoyrc'\n\n");
    printf("Note:\n");
    printf("  * Set 'axis_as_mouse' to 1 to ignore axis_up/down/left/right settings and use axis as mouse.\n");
    printf("  * Set combined keys with '+', such as 'Super_L+Shift_L+q'.\n");
    printf("  * Set mouse click/scroll event with 'mouse_button_<n>', where 'n' is:\n");
    printf("    - 1 : left button\n");
    printf("    - 2 : middle button\n");
    printf("    - 3 : right button\n");
    printf("    - 4 : scroll up\n");
    printf("    - 5 : scroll down\n");
}

void load_user_config(config *conf)
{
    char config_file[64];
    sprintf(config_file, "%s/.enjoyrc", getenv("HOME"));

    if(access(config_file, R_OK) == 0) {
        FILE *fp;
        char *line = NULL;
        size_t len = 0;
        ssize_t read;
        char *key, *value;

        fp = fopen(config_file, "r");
        /* ok, still have default configurations */
        if (fp == NULL) {
            printf ("can not read config file \n");
            return;
        }

        while ((read = getline(&line, &len, fp)) != -1) {
            /* ignore line start with # */
            if (strncmp(line, "#", strlen(line)) == 0)
                continue;
            if (parse_line(line, &key, &value))
                continue;
            if (strcmp(key, "device") == 0)
                conf->device=strdup(value);
            else if (strcmp(key, "button_0") == 0)
                conf->button_0=strdup(value);
            else if (strcmp(key, "button_1") == 0)
                conf->button_1=strdup(value);
            else if (strcmp(key, "button_2") == 0)
                conf->button_2=strdup(value);
            else if (strcmp(key, "button_3") == 0)
                conf->button_3=strdup(value);
            else if (strcmp(key, "button_4") == 0)
                conf->button_4=strdup(value);
            else if (strcmp(key, "button_5") == 0)
                conf->button_5=strdup(value);
            else if (strcmp(key, "button_6") == 0)
                conf->button_6=strdup(value);
            else if (strcmp(key, "button_7") == 0)
                conf->button_7=strdup(value);
            else if (strcmp(key, "button_8") == 0)
                conf->button_8=strdup(value);
            else if (strcmp(key, "button_9") == 0)
                conf->button_9=strdup(value);
            else if (strcmp(key, "axis_up") == 0)
                conf->axis_up=strdup(value);
            else if (strcmp(key, "axis_down") == 0)
                conf->axis_down=strdup(value);
            else if (strcmp(key, "axis_left") == 0)
                conf->axis_left=strdup(value);
            else if (strcmp(key, "axis_right") == 0)
                conf->axis_right=strdup(value);
            else if (strcmp(key, "axis_as_mouse") == 0)
                conf->axis_as_mouse=atoi(strdup(value));
        }
        free(line);
        fclose(fp);
    }
}

int main(int argc, char *argv[])
{
    const char *device;
    int js;
    struct js_event event;
    struct axis_state axes[3] = {0};
    size_t axis;

    KeyCode keycode = 0;

    /* for mouse move thread. */
    XInitThreads();

    Display *disp = XOpenDisplay (NULL);
    config *conf = create_default_config();

    if(argc > 1 && !strcmp(argv[1], "-h")){
        help(conf);
        exit(0);
    }

    /* load user configuration */
    load_user_config(conf);
    /* print_config(conf); */

    device = conf->device;
    js = open(device, O_RDONLY);

    if (js == -1)
        perror("Could not open joystick");

    /* This loop will exit if the controller is unplugged. */
    while (read_event(js, &event) == 0)
    {
        switch (event.type)
        {
            case JS_EVENT_BUTTON:
                switch(event.number)
                {
                    case 0: fake_event(disp, conf->button_0, event.value); break;
                    case 1: fake_event(disp, conf->button_1, event.value); break;
                    case 2: fake_event(disp, conf->button_2, event.value); break;
                    case 3: fake_event(disp, conf->button_3, event.value); break;
                    case 4: fake_event(disp, conf->button_4, event.value); break;
                    case 5: fake_event(disp, conf->button_5, event.value); break;
                    case 6: fake_event(disp, conf->button_6, event.value); break;
                    case 7: fake_event(disp, conf->button_7, event.value); break;
                    case 8: fake_event(disp, conf->button_8, event.value); break;
                    case 9: fake_event(disp, conf->button_9, event.value); break;
                    default: break;
                }
            case JS_EVENT_AXIS:
                axis = get_axis_state(&event, axes);
                //buttons will generate AXIS event too, ignore it.
                if(axes[axis].x == 1 || axes[axis].y == 1)
                    break; 
                if (axis < 3) {
                    if(!conf->axis_as_mouse) {
                        if(axes[axis].x == 0 && axes[axis].y == -32767) /* up */
                        {
                            axis_up_press = 1;
                            fake_event(disp, conf->axis_up, 1);
                        }
                        if(axes[axis].x == 0 && axes[axis].y == 32767) /* down */
                        {
                            axis_down_press = 1;
                            fake_event(disp, conf->axis_down, 1);
                        }
                        if(axes[axis].x == -32767 && axes[axis].y == 0) /* left */
                        {
                            axis_left_press = 1;
                            fake_event(disp, conf->axis_left, 1);
                        }
                        if(axes[axis].x == 32767 && axes[axis].y == 0) /* right */
                        {
                            axis_right_press = 1;
                            fake_event(disp, conf->axis_right, 1);
                        }
                        if(axes[axis].x == 0 && axes[axis].y == 0) //release
                        {
                            if(axis_up_press) {
                                fake_event(disp, conf->axis_up, 0);
                                axis_up_press = 0;
                            }
                            if(axis_down_press) {
                                fake_event(disp, conf->axis_down, 0);
                                axis_down_press = 0;
                            }
                            if(axis_left_press) {
                                fake_event(disp, conf->axis_left, 0);
                                axis_left_press = 0;
                            }
                            if(axis_right_press) {
                                fake_event(disp, conf->axis_right, 0);
                                axis_right_press = 0;
                            }
                        }
                    } else {
                        axis_x_direction = axes[axis].x/32767;
                        axis_y_direction = axes[axis].y/32767;
                        /* printf("Axis %zu at (%6d, %6d)\n", axis, axes[axis].x, axes[axis].y); */
                        if(axes[axis].x == 0 && axes[axis].y == 0) { 
                            pthread_join(motion_thread_t, NULL);
                            motion_thread_created = False;
                            /* restore move intervals.*/
                            motion_interval = MOTION_INTERVAL_INIT;
                        } else if(!motion_thread_created) {
                            pthread_create(&motion_thread_t, NULL, motion_thread, (void *)disp);
                            motion_thread_created = True;
                        }
                    }
                }
                break;
            default:
                /* Ignore init events. */
                break;
        }
        //fflush(stdout);
    }

    free(conf);
    XCloseDisplay(disp); 
    close(js);
    return 0;
}
