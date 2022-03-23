/*
Author: Cjacker <cjacker@foxmail.com>

Description:
Map joystick events to mouse or key events.
*/


#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <string.h>
#include <ctype.h>
#include <pthread.h>
#include <signal.h>
#include <linux/joystick.h>
#include <sys/epoll.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <netdb.h>
#include <sys/stat.h>
#include <errno.h>
#include <unistd.h>

#include "cfg_parse.h"
#include "arg.h"
#include "keytable.h"
#include "uinput.h"
#include "daemon.h"

#define MAXEVENTS 64

int pid_fd;
static int running = 1;

/* global config */
struct cfg_struct *cfg;

static char *pid_filename = NULL;

int uinput_fd = -1;
int debug_mode = 0;

static int suspend = 0;

char *argv0;

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

void signal_handler(int sig)
{
    if (sig == SIGINT) {
        /* Unlock and close lockfile */
        if (pid_fd != -1) {
            lockf(pid_fd, F_ULOCK, 0);
            close(pid_fd);
        }
        /* Try to delete lockfile */
        if (pid_filename != NULL) {
            unlink(pid_filename);
        }
        running = 0;
        /* Reset signal handling to default behavior */
        signal(SIGINT, SIG_DFL);
    }
}

/* support key sequence, for example: "Control_L+g c" */
void fake_key_sequence(char *value)
{
    char *keyseq = malloc(strlen(value)+1);
    strcpy(keyseq, value);

    char *end_str;
    char *token = strtok_r(keyseq, " ", &end_str);
    struct timespec ts = { .tv_sec = 0, .tv_nsec = 10000  };
    while(token != NULL ) {
        fake_key_uinput(uinput_fd, token, 1);
        fake_key_uinput(uinput_fd, token, 0);
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
        fprintf(stderr, "Key: '%s', State: '%s'\n", key, state ? "pressed" : "released");
    }

    if(value == NULL) {
        fprintf(stderr, "Key '%s' not map\n", key);
        return;
    } else {
        if(debug_mode)
            fprintf(stderr, "Key '%s' map to '%s'\n", key, value);
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

    /* 'mouse_button <n>': value + 13 is 'n' */
    if(strncasecmp (value, "mouse_button ", 13) == 0) {
        value += 13;
        if(is_valid_number(value)) /* make sure 'value' is a 'number' */
            fake_mouse_button_uinput(uinput_fd, atoi(value), state);
    } else
         fake_key_uinput(uinput_fd, value, state);
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

/* Init control socket */
int init_ctl_socket(char *sock_path)
{
    int fd;
    int len;
    struct sockaddr_un local;

    if((fd = socket(AF_UNIX, SOCK_DGRAM, 0)) == -1)
    {
        perror("Error creating server socket");
        exit(EXIT_FAILURE);
    }

    local.sun_family = AF_UNIX;
    strcpy(local.sun_path, sock_path);
    unlink(local.sun_path);
    len = strlen(local.sun_path) + sizeof(local.sun_family);
    if(bind(fd, (struct sockaddr *)&local, len) == -1)
    {
        perror("binding");
        exit(EXIT_FAILURE);
    }

    return fd;
}

/* show help infomation */
void usage()
{
    printf("enjoy - map joystick events and convert to mouse/key event\n"
           "        By cjacker <cjacker@foxmail.com>\n\n"
           "Usage: enjoy [-D] [-h] [-k] [-c configfile] [-d device] [-p pidfile]\n\n"
           "Args:\n"
           " -D: debug mode.\n"
           " -c <configfile> : specify config file.\n"
           " -p <pidfile> : specify pid file.\n"
           " -k: print 'keyname' can be used in config file.\n"
           " -h: show this message.\n\n"
           "Config file:\n"
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
int main(int argc, char *argv[])
{
    int ctl_sock_fd;
    int joystick_fd;
    int epoll_fd;
    struct epoll_event ep_sevent;
    struct epoll_event ep_jevent;
    struct epoll_event *ep_events;

    int ret;

    const char *device = NULL;
    const char *config_filename = NULL;

    struct js_event event;
    struct axis_state axes[3] = {0};
    size_t axis;

    /* process arguments */
    ARGBEGIN {
    case 'D':
      debug_mode = 1;
      break;
    case 'p':
      pid_filename = strdup(EARGF(usage()));
      break;
    case 'c':
      config_filename = EARGF(usage());
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

    if(config_filename && access(config_filename, R_OK) == 0) {
        cfg_load(cfg, config_filename);
    } else {
        fprintf(stderr, "please specify correct config file.\n");
        exit(EXIT_FAILURE);
    }

    device = cfg_get(cfg, "device");
    if(!device) {
        fprintf(stderr, "wrong config file format, \n"
                        "please add 'device=<joystick dev path>' to config file '%s'.\n", config_filename);
        exit(EXIT_FAILURE);
    }

    if(device && access(device, R_OK) != 0) {
        fprintf(stderr, "wrong device: '%s'\n", device);
        exit(EXIT_FAILURE);
    }

    /* force config_filename equal to device filename. */
    if(strcmp(basename(config_filename), basename(device)) != 0) {
        fprintf(stderr, "wrong config filename: '%s', should be '%s' as device name\n", config_filename, basename(device));
        exit(EXIT_FAILURE);
    }

    if(!pid_filename) {
        pid_filename = malloc(256);
        sprintf(pid_filename, "/run/enjoy_%s.pid", basename(config_filename));
    }

    if(access(pid_filename, R_OK) == 0) {
        fprintf(stderr, "enjoy already running? pid file exists: %s\n", pid_filename);
        exit(EXIT_FAILURE);
    }

    if(!debug_mode)
        daemonize(pid_filename);

    /* Register signal handler */
    signal(SIGINT, signal_handler);

    joystick_fd = open(device, O_RDONLY);
    if (joystick_fd == -1){
        perror("Could not open joystick");
        exit(EXIT_FAILURE);
    }

    if(debug_mode) {
        fprintf(stderr,"joystick '%s' opened, have '%ld' buttons and '%ld' axis\n",
                       device,
                       get_button_count(joystick_fd),
                       get_axis_count(joystick_fd));
    }

    char *sock_path = malloc(256);
    sprintf(sock_path, "/tmp/enjoy_%s.socket", basename(config_filename));
    ctl_sock_fd = init_ctl_socket(sock_path);

    chmod(sock_path, 0666);

    uinput_fd = init_uinput();
    if (uinput_fd == -1){
        perror("Could not open uinput");
        exit(EXIT_FAILURE);
    }


    epoll_fd = epoll_create1 (0);
    if (epoll_fd == -1) {
        perror ("epoll_create");
        exit(EXIT_FAILURE);
    }


    ep_sevent.data.fd = ctl_sock_fd;
    ep_sevent.events = EPOLLIN;
    ret = epoll_ctl (epoll_fd, EPOLL_CTL_ADD, ctl_sock_fd, &ep_sevent);
    if (ret == -1) {
        perror ("epoll_ctl");
        abort ();
    }

    ep_jevent.data.fd = joystick_fd;
    ep_jevent.events = EPOLLIN;
    ret = epoll_ctl (epoll_fd, EPOLL_CTL_ADD, joystick_fd, &ep_jevent);
    if (ret == -1) {
        perror ("epoll_ctl");
        exit(EXIT_FAILURE);
    }

    /* Buffer where events are returned */
    ep_events = calloc (MAXEVENTS, sizeof(struct epoll_event));

    /* The event loop */
    while (running) {
        int n, i;
        n = epoll_wait (epoll_fd, ep_events, MAXEVENTS, -1);
        for (i = 0; i < n; i++){
            if ((ep_events[i].events & EPOLLERR) ||
                (ep_events[i].events & EPOLLHUP) ||
                (!(ep_events[i].events & EPOLLIN))) {
                /* An error has occured on this fd, or the socket is not
                   ready for reading (why were we notified then?) */
                close (ep_events[i].data.fd);
                continue;
            } else if (ctl_sock_fd == ep_events[i].data.fd) {
                char buff[16];
                struct sockaddr_un from;
                socklen_t fromlen = sizeof(from);
                recvfrom(ep_events[i].data.fd, buff, 16, 0, (struct sockaddr *)&from, &fromlen);
                if(!strcmp(buff, "suspend"))
                        suspend = 1;
                else if(!strcmp(buff, "toggle"))
                        suspend = !suspend;
                else
                        suspend = 0;
            } else if (joystick_fd == ep_events[i].data.fd) {
                read_event(ep_events[i].data.fd, &event);
                if(suspend)
                    continue;
                switch (event.type) {
                    case JS_EVENT_BUTTON:
                        fake_button_event(0, -1, 0, 0, event.number, event.value);
                        break;
                    case JS_EVENT_AXIS:
                        axis = get_axis_state(&event, axes);
                        if (axis < 3) {
                            if(debug_mode)
                                fprintf(stderr, "Axis %zu button %d at (%6d, %6d)\n",
                                        axis, event.number,  axes[axis].x, axes[axis].y);

                            /* buttons will generate AXIS event too, ignore it. */
                            if(abs(axes[axis].x) == 1 || abs(axes[axis].y) == 1)
                                break;

                            char axis_as_mouse_key[20];
                            sprintf(axis_as_mouse_key, "axis%zu_as_mouse", axis);
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
                                    pthread_create(&motion_thread_t, NULL, motion_thread_uinput, NULL);
                                    motion_thread_created = 1;
                                }
                            }
                        }
                        break;
                    default:
                        /* Ignore init events. */
                        break;
                 }
                 continue;
            }
        }
    }

    free(pid_filename);
    free(ep_events);
    cfg_free(cfg);
    close(joystick_fd);
    close(ctl_sock_fd);
    close_uinput(uinput_fd);

    return EXIT_SUCCESS;
}
