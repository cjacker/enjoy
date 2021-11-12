#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <string.h>
#include <unistd.h>

#include <linux/uinput.h>

#include "uinput.h"

void emit(int fd, int type, int code, int val)
{
   struct input_event ie;

   ie.type = type;
   ie.code = code;
   ie.value = val;
   /* timestamp values below are ignored */
   ie.time.tv_sec = 0;
   ie.time.tv_usec = 0;

   int ret = write(fd, &ie, sizeof(ie));
   return;
}


void fake_mouse_button_uinput(int fd, int button_number, int state)
{
    switch(button_number) {
        case 1:
            emit(fd, EV_KEY, BTN_LEFT, state);
            emit(fd, EV_SYN, SYN_REPORT, 0);
            break;
        case 2:
            emit(fd, EV_KEY, BTN_MIDDLE, state);
            emit(fd, EV_SYN, SYN_REPORT, 0);
            break;
        case 3:
            emit(fd, EV_KEY, BTN_RIGHT, state);
            emit(fd, EV_SYN, SYN_REPORT, 0);
            break;
        case 4:
            emit(fd, EV_KEY, BTN_FORWARD, state);
            emit(fd, EV_SYN, SYN_REPORT, 0);
            break;
        case 5:
            emit(fd, EV_KEY, BTN_BACK, state);
            emit(fd, EV_SYN, SYN_REPORT, 0);
            break;
        default:
            break;
    }
}


int init_uinput()
{
    struct uinput_setup usetup;
    int fd = open("/dev/uinput", O_WRONLY | O_NONBLOCK);
    if(fd < 0) {
        perror("open /dev/uinput failed:");
        exit(1);
    }

    ioctl(fd, UI_SET_EVBIT, EV_KEY); 
    ioctl(fd, UI_SET_EVBIT, EV_REL);
    ioctl(fd, UI_SET_RELBIT, REL_X);
    ioctl(fd, UI_SET_RELBIT, REL_Y);
    int i;
    /* enable all keycodes support */
    for (i=0; i < 256; i++) { 
        ioctl(fd, UI_SET_KEYBIT, i); 
    } 
  
    /* enable all mouse buttons */ 
    ioctl(fd, UI_SET_KEYBIT, BTN_MOUSE); 
    ioctl(fd, UI_SET_KEYBIT, BTN_TOUCH); 
    ioctl(fd, UI_SET_KEYBIT, BTN_MOUSE); 
    ioctl(fd, UI_SET_KEYBIT, BTN_LEFT); 
    ioctl(fd, UI_SET_KEYBIT, BTN_MIDDLE); 
    ioctl(fd, UI_SET_KEYBIT, BTN_RIGHT); 
    ioctl(fd, UI_SET_KEYBIT, BTN_FORWARD); 
    ioctl(fd, UI_SET_KEYBIT, BTN_BACK); 
    
    memset(&usetup, 0, sizeof(usetup));
    usetup.id.bustype = BUS_USB;
    usetup.id.vendor = 0x1234; /* sample vendor */
    usetup.id.product = 0x5678; /* sample product */
    strcpy(usetup.name, "Enjoy device");
    
    ioctl(fd, UI_DEV_SETUP, &usetup);
    ioctl(fd, UI_DEV_CREATE);
    return fd;
}

void close_uinput(int fd)
{
   ioctl(fd, UI_DEV_DESTROY);
   close(fd);
}
