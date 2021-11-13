/*
Author: Cjacker <cjacker@foxmail.com>

Description:
Map joystick events to mouse or key events.

Compile:
make

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

#include "cfg_parse.h"
#include "arg.h"

#include "keytable.h"

#include "uinput.h"

#ifdef WITH_X
#include "x.h"
#endif

static int use_x = 0;

int uinput_fd = -1;

#ifdef WITH_X
Display *disp = NULL;
#endif

int debug_mode = 0;

char *argv0;

static int no_default_config = 0;
static char * config_file = NULL;

/* global config */
struct cfg_struct *cfg;

int axis_up_press = 0;
int axis_down_press = 0;
int axis_left_press = 0;
int axis_right_press = 0;

int axis_x_direction = 0;
int axis_y_direction = 0;

/* motion interval init value */
#define MOTION_INTERVAL_INIT 8000
int motion_interval = MOTION_INTERVAL_INIT;

int motion_thread_created = 0;
pthread_t motion_thread_t;

/* support key sequence, for example: "Control_L+g c" */
void fake_key_sequence(char *value)
{
    char *keyseq = malloc(strlen(value)+1);
    strcpy(keyseq, value);

    char *end_str;
    char *token = strtok_r(keyseq, " ", &end_str);
    struct timespec ts = { .tv_sec = 0, .tv_nsec = 10000  };
    while(token != NULL ) {
        if(!use_x) {
            fake_key_uinput(uinput_fd, token, 1);
            fake_key_uinput(uinput_fd, token, 0);
        } else {
#ifdef WITH_X
            fake_key_x(disp, token, 1);
            fake_key_x(disp, token, 0);
#endif
        }
        nanosleep(&ts, NULL);
        token = strtok_r(NULL, " ", &end_str);
    }
    free(keyseq);
}

int is_valid_number(char * string)
{
   for(int i = 0; i < strlen( string ); i ++)
   {
      //ASCII value of 0 = 48, 9 = 57. So if value is outside of numeric range then fail
      //Checking for negative sign "-" could be added: ASCII value 45.
      if (string[i] < 48 || string[i] > 57)
         return 0;
   }
   return 1;
}

/* axis: is axis event or not
 * axis_n: which axis
 * x: axis x value
 * y: axis y value
 * button_n: which button
 * state: press or release
 */
void fake_button_event(int axis, int axis_n, int x, int y, int button_n, int state)
{
    //query button_n value from cfg;
    char key[20];
    if(axis) {
        /* up and down */
        if(x == 0) {
            if (y == -32767) {
                axis_up_press = state;
                sprintf(key, "axis%d_button%d_up", axis_n, button_n);
            }
            if (y == 32767) {
                axis_down_press = state;
                sprintf(key, "axis%d_button%d_down", axis_n, button_n);            
            }
        } 
        if(y == 0) {
            if (x == -32767) {
                axis_left_press = state;
                sprintf(key, "axis%d_button%d_left", axis_n, button_n);
            }
            if (x == 32767) {
                axis_right_press = state;
                sprintf(key, "axis%d_button%d_right",axis_n, button_n);            
            }
        } 
    } else {
        sprintf(key, "button_%d", button_n); 
    }
   
    char *value = (char *)cfg_get(cfg, key);

    if(debug_mode) {
        fprintf(stderr, "'%s' %s\n", key, state ? "pressed" : "released"); 
    }

    if(value == NULL) {
        fprintf(stderr, "'%s' not map\n", key);
        return;
    } else {
        if(debug_mode)
            fprintf(stderr, "'%s' map to '%s'\n", key, value);
    }

    /* keyseq */
    if(strncasecmp (value, "keyseq ", 7) == 0) {
        /* run key sequence only on key press */
        if(state == 1) {
            value += 7; /* key sequance seperate by ' ' */
            fake_key_sequence(value);
        }
        return;
    }
  
    /* exec */    
    if(strncasecmp (value, "exec ", 5) == 0) {
        /* run cmd only on key press */
        if(state == 1) {
            value += 5; /* command with args */
            char cmd[strlen(value)+1];
            sprintf(cmd, "%s &", value);
            int ret = system(cmd);
        } 
        return;    
    }

    /* 'mouse_button <n>': value + 13 is 'n' */
    if(strncasecmp (value, "mouse_button ", 13) == 0) {
        value += 13;
        if(is_valid_number(value)) /* make sure 'value' is a 'number' */
            if(!use_x)
                fake_mouse_button_uinput(uinput_fd, atoi(value), state);
#ifdef WITH_X
            else
                fake_mouse_button_x(disp, atoi(value), state);
#endif
    } else
        if(!use_x)
            fake_key_uinput(uinput_fd, value, state);
#ifdef WITH_X
        else
            fake_key_x(disp, value, state);
#endif
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

/* show help infomation */
void usage()
{
    printf("enjoy - Read joystick events and convert to mouse/key event\n"
           "        By cjacker <cjacker@foxmail.com>\n\n"
           "Usage: enjoy [-D] [-n] [-h] [-c configfile]\n\n"
           "Args:\n"
           " -D: debug mode, report joystick event name can be used in config file.\n"
#ifdef WITH_X
           " -x: use Xtest instead of uinput to simulate events, slow.\n"
#endif
           " -n: no default configurations.\n"
           " -c <configfile>: use '~/.config/<configfile>' as config file.\n"
           " -k: print 'keyname' can be used in config file.\n"
           " -h: show this message.\n\n"
           "Config file format:\n"
           " enjoy's config file use 'key=value' format,\n"
           " the key is the name of joystick event (use debug mode to find out),\n"
           " the value of key has below formats:\n"
           "    1. Keyname or combined keyname with '+' as delimiter, for example: up, control_l+shift_l+q.\n"
           "       - Use '-k' to find keynames enjoy supported.\n"
           "    2. Key sequence prefix with 'keyseq ', continue with a sequence of keyname seperate by ' '.\n"
           "    3. Mouse button prefix with 'mouse_button ', continue with button number.\n"
           "       - 1/left, 2/middle, 3/right, 4/scrollup, 5/scrolldown\n"
           "    4. Launch App prefix with 'exec ', continue with command and args\n"
           "    5. Set 'axis<n>_as_mouse' to 1 will treate axis as mouse motion\n\n"
           "For more information, please refer to README.md\n");
    exit(0);
}

void load_user_cfg(struct cfg_struct *cfg, char *config_file_name)
{
    char config_file[64];

    if(!config_file_name)
        sprintf(config_file, "%s/.config/enjoyrc", getenv("HOME"));
    else
        sprintf(config_file, "%s/.config/%s", getenv("HOME"), config_file_name);

    if(access(config_file, R_OK) == 0) {
        cfg_load(cfg, config_file);
    } else {
        if(config_file_name)
            fprintf(stderr, "%s/.config/%s not exists\n",  getenv("HOME"), config_file_name);
    }
}

void init_default_cfg(struct cfg_struct *cfg)
{
   cfg_set(cfg, "device", "/dev/input/js0");
   cfg_set(cfg, "axis0_as_mouse", "1");
   cfg_set(cfg, "button_0", "super_l");
   cfg_set(cfg, "button_1", "mouse_button 3");
   cfg_set(cfg, "button_2", "mouse_button 1");
   cfg_set(cfg, "button_3", "control_l");
   cfg_set(cfg, "button_8", "super_l+end");
   cfg_set(cfg, "button_9", "super_l+d");
   cfg_set(cfg, "axis0_button1_up", "up");
   cfg_set(cfg, "axis0_button1_down", "down");
   cfg_set(cfg, "axis0_button0_left", "left");
   cfg_set(cfg, "axis0_button0_right", "right");
}

int main(int argc, char *argv[])
{
    const char *device;
    int js;
    struct js_event event;
    struct axis_state axes[3] = {0};
    size_t axis;


    /* process arguments */
    ARGBEGIN {
    case 'D':
      debug_mode = 1;
      break;
#ifdef WITH_X
    case 'x':
      use_x = 1;
      break;
#endif
    case 'n':
      no_default_config = 1;
      break;
    case 'c':
      config_file = EARGF(usage());
      break;
    case 'h':
      usage();
      exit(0);
    case 'k':
      print_keytable();
      exit(0);
    default:
      usage();
      exit(0);
    }
    ARGEND;

    /* init config file; */
    cfg = cfg_init();


    /* init default configuration */
    if(!no_default_config)
        init_default_cfg(cfg);

    /* load user configuration */
    load_user_cfg(cfg, config_file);

    device = cfg_get(cfg, "device");

    js = open(device, O_RDONLY);

    if (js == -1){
        perror("Could not open joystick");
        exit(1);
    }

    uinput_fd = init_uinput();

#ifdef WITH_X
    disp = init_x();
#endif
    /* This loop will exit if the controller is unplugged. */
    while (read_event(js, &event) == 0)
    {
        switch (event.type)
        {
            case JS_EVENT_BUTTON:
                fake_button_event(0, -1, 0, 0, event.number, event.value);
                break;
            case JS_EVENT_AXIS:
                axis = get_axis_state(&event, axes);
                if (axis < 3) {
                    if(debug_mode)
                        fprintf(stderr, "Axis %zu button %d at (%6d, %6d)\n", axis, event.number,  axes[axis].x, axes[axis].y);
                     
                    /* buttons will generate AXIS event too, ignore it. */
                    if(abs(axes[axis].x) == 1 || abs(axes[axis].y) == 1)
                        break;

                    char axis_as_mouse_key[20];
                    sprintf(axis_as_mouse_key, "axis%ld_as_mouse", axis);
                    /* null to 0. */
                    if(!(atoi(cfg_get(cfg, axis_as_mouse_key) ? cfg_get(cfg, axis_as_mouse_key) : "0"))) {
                        if(axes[axis].x != 0 || axes[axis].y != 0)
                            fake_button_event(1, axis, axes[axis].x, axes[axis].y, event.number, 1);
                        if(axes[axis].x == 0 && axes[axis].y == 0) /* release */
                        {
                            if(axis_up_press)
                                fake_button_event(1, axis, 0, -32767, event.number, 0);
                            if(axis_down_press)
                                fake_button_event(1, axis, 0, 32767, event.number, 0);
                            if(axis_left_press) 
                                fake_button_event(1, axis, -32767, 0, event.number, 0);
                            if(axis_right_press)
                                fake_button_event(1, axis, 32767, 0, event.number, 0);
                        }
                    } else {
                        axis_x_direction = axes[axis].x==0 ? 0 : -(axes[axis].x < 0) | 1; /*convert it to -1, 0, 1 */
                        axis_y_direction = axes[axis].y==0 ? 0 : -(axes[axis].y < 0) | 1; /*convert it to -1, 0, 1 */
                        if(axes[axis].x == 0 && axes[axis].y == 0) { 
                            pthread_join(motion_thread_t, NULL);
                            motion_thread_created = 0;
                            /* restore move intervals.*/
                            motion_interval = MOTION_INTERVAL_INIT;
                        } else if(!motion_thread_created) {
                            if(!use_x)
                                pthread_create(&motion_thread_t, NULL, motion_thread_uinput, NULL);
#ifdef WITH_X
                            else
                                pthread_create(&motion_thread_t, NULL, motion_thread_x, (void *)disp);
#endif
                            motion_thread_created = 1;
                        }
                    }
                }
                break;
            default:
                /* Ignore init events. */
                break;
        }
    }

    cfg_free(cfg);
    close(js);
#ifdef WITH_X
    if(disp)
        close_x(disp); 
#endif
    if(uinput_fd != -1)
        close_uinput(uinput_fd);
    return 0;
}
