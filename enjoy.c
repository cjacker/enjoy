/*
  Author: Cjacker

  Description:
  Read joystick events and convert it to mouse or key event.

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

pthread_t mouse_move_thread_t;

typedef struct config {
  char *device;
  int axis_as_mouse;
  char *button_a;
  char *button_b;
  char *button_x;
  char *button_y;
  char *button_select;
  char *button_start;
  char *axis_up;
  char *axis_down;
  char *axis_left;
  char *axis_right;
} config;

/*default configuration*/
config *create_default_config()
{
  config *p = malloc(sizeof(struct config));
  p->device = "/dev/input/js0";
  p->axis_as_mouse = 1;
  p->button_a = "mouse_click_1";
  p->button_b = "mouse_click_3";
  p->button_x = "Control_L";
  p->button_y = "Shift_L";
  p->button_select = "Super_L+End";
  p->button_start = "Super_L+d";
  p->axis_up = "Up";
  p->axis_down = "Down";
  p->axis_left = "Left";
  p->axis_right = "Right";
  return p;
}

//printf configuration
void print_config(config *conf)
{
  printf("joystick device: %s\n", conf->device);
  printf("button_a map to: %s\n", conf->button_a);
  printf("button_b map to: %s\n", conf->button_b);
  printf("button_x map to: %s\n", conf->button_x);
  printf("button_y map to: %s\n", conf->button_y);
  printf("button_select map to: %s\n", conf->button_select);
  printf("button_start map to: %s\n", conf->button_start);
  printf("axis as mouse: %d\n", conf->axis_as_mouse);
  printf("axis_up map to: %s\n", conf->axis_up);
  printf("axis_down map to: %s\n", conf->axis_down);
  printf("axis_left map to: %s\n", conf->axis_left);
  printf("axis_right map to: %s\n", conf->axis_right);
}

/*remove all spaces from string*/
void remove_spaces(char *str)
{
  int count = 0;
  for (int i = 0; str[i]; i++)
    if (str[i] != ' ')
      str[count++] = str[i];
  str[count] = '\0';
}

Bool is_mouse_move_event(char *str)
{
  if(strstr(str, "mouse_move_"))
    return True;
  return False;
}

Bool is_mouse_click_event(char *str)
{
  if(strstr(str, "mouse_click_"))
    return True;
  return False;
}


void *mouse_move_thread(void * disp) {
  XEvent event;
  while(axis_x_direction != 0 || axis_y_direction != 0) {
    /* Get the current pointer position */
    XQueryPointer (disp, RootWindow (disp, 0), &event.xbutton.root,
         &event.xbutton.window, &event.xbutton.x_root,
         &event.xbutton.y_root, &event.xbutton.x, &event.xbutton.y,
         &event.xbutton.state);
    XTestFakeMotionEvent (disp, 0, event.xbutton.x + axis_x_direction, event.xbutton.y + axis_y_direction, CurrentTime);
    //seems not needed?
    //XFlush(disp);
    usleep(6000);
  }
}

//state: 'True' for press, 'False' for release
void fake_key(Display *disp, KeyCode keycode, Bool state)
{
  if(keycode == 0)
    return;
  XTestFakeKeyEvent (disp, keycode, state, 0);
  XFlush(disp);
}


/*convert string to keycode*/
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

void fake_mouse_click(Display *disp, char *string, Bool state)
{
  if(is_mouse_click_event(string) && (strdup(string) + 12) != NULL) {
    /* Fake the mouse button Press and Release events */
    XTestFakeButtonEvent (disp, atoi(strdup(string) + 12), state,  CurrentTime);
    XFlush(disp);
  }
}

void fake_event(Display *disp, char *string, Bool state)
{
  if(is_mouse_click_event(string))
    fake_mouse_click(disp, string, state);
  else
    fake_key_sequence(disp, string, state);
}
/**
 * Reads a joystick event from the joystick device.
 *
 * Returns 0 on success. Otherwise -1 is returned.
 */
int read_event(int fd, struct js_event *event)
{
  ssize_t bytes;

  bytes = read(fd, event, sizeof(*event));

  if (bytes == sizeof(*event))
    return 0;

  /* Error, could not read full event. */
  return -1;
}

/**
 * Returns the number of axes on the controller or 0 if an error occurs.
 */
size_t get_axis_count(int fd)
{
  __u8 axes;

  if (ioctl(fd, JSIOCGAXES, &axes) == -1)
    return 0;

  return axes;
}

/**
 * Returns the number of buttons on the controller or 0 if an error occurs.
 */
size_t get_button_count(int fd)
{
  __u8 buttons;
  if (ioctl(fd, JSIOCGBUTTONS, &buttons) == -1)
    return 0;

  return buttons;
}

/**
 * Current state of an axis.
 */
struct axis_state {
  short x, y;
};

/**
 * Keeps track of the current axis state.
 *
 * NOTE: This function assumes that axes are numbered starting from 0, and that
 * the X axis is an even number, and the Y axis is an odd number. However, this
 * is usually a safe assumption.
 *
 * Returns the axis that the event indicated.
 */
size_t get_axis_state(struct js_event *event, struct axis_state axes[3])
{
  size_t axis = event->number / 2;

  if (axis < 3)
    {
      if (event->number % 2 == 0)
	axes[axis].x = event->value;
      else
	axes[axis].y = event->value;
    }

  return axis;
}


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

void help()
{
  printf("enjoy - read joystick events and convert to mouse/key event\n\n");
  printf("the default config is:\n");
  printf("  device=/dev/input/js0\n");
  printf("  button_a=mouse_click_1\n");
  printf("  button_b=mouse_click_3\n");
  printf("  button_x=Control_L\n");
  printf("  button_y=Shift_L\n");
  printf("  button_select=Super_L+End\n");
  printf("  button_start=Super_L+d\n");
  printf("  axis_as_mouse=1\n");
  printf("  axis_up=Up\n");
  printf("  axis_down=Down\n");
  printf("  axis_left=Left\n");
  printf("  axis_right=Right\n\n");
  printf("you can write your own config as '~/.enjoyrc'\n\n");
  printf("Note:\n");
  printf("  * if 'axis_as_mouse' set to 1, axis_xx values will be ignored and used as mouse\n");
  printf("  * you can set up combined keys, such as 'Super_L+Shift_L+q'\n");
  printf("  * if you want to set it as mouse click event, use 'mouse_click_[1,2,3]'\n");
  printf("    - 1:left button, 2:middle button, 3:right button.\n");
}


int main(int argc, char *argv[])
{
  if(argc > 1 && !strcmp(argv[1], "-h")){
    help();
    exit(0);
  }
 
  const char *device;

  int js;
  struct js_event event;
  struct axis_state axes[3] = {0};
  size_t axis;

  KeyCode keycode = 0;
  
  //for mouse move thread.
  XInitThreads();
  Display *disp = XOpenDisplay (NULL);
  config *conf = create_default_config();

  char config_file[64];
  sprintf(config_file,"%s/.enjoyrc", getenv("HOME"));

  if(access(config_file, F_OK) == 0) {
    FILE *fp;
    char *line = NULL;
    size_t len = 0;
    ssize_t read;
    char *key, *value;

    fp = fopen(config_file, "r");
    if (fp == NULL) {
        printf ("can not read config file \n");
    }
    while ((read = getline(&line, &len, fp)) != -1) {
        if (parse_line(line, &key, &value))
            continue;
        if (strcmp(key, "device") == 0)
            conf->device=strdup(value);
        else if (strcmp(key, "button_a") == 0)
            conf->button_a=strdup(value);
        else if (strcmp(key, "button_b") == 0)
            conf->button_b=strdup(value);
        else if (strcmp(key, "button_x") == 0)
            conf->button_x=strdup(value);
        else if (strcmp(key, "button_y") == 0)
            conf->button_y=strdup(value);
        else if (strcmp(key, "button_select") == 0)
            conf->button_select=strdup(value);
        else if (strcmp(key, "button_start") == 0)
            conf->button_start=strdup(value);
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
 
  //print_config(conf);
 
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
	    case 0: fake_event(disp, conf->button_x, event.value); break;
	    case 1: fake_event(disp, conf->button_a, event.value); break;
	    case 2: fake_event(disp, conf->button_b, event.value); break;
	    case 3: fake_event(disp, conf->button_y, event.value); break;
	    case 8: fake_event(disp, conf->button_select, event.value); break;
	    case 9: fake_event(disp, conf->button_start, event.value); break;
	    default: break;
	    }
	case JS_EVENT_AXIS:
	  axis = get_axis_state(&event, axes);
	  if (axis < 3) {
	    if(!conf->axis_as_mouse) {
	      if(axes[axis].x == 0 && axes[axis].y == -32767) //up
		{
		  axis_up_press = 1;
		  fake_event(disp, conf->axis_up, 1);
		}
	      if(axes[axis].x == 0 && axes[axis].y == 32767) //down
		{
		  axis_down_press = 1;
		  fake_event(disp, conf->axis_down, 1);
		}
	      if(axes[axis].x == -32767 && axes[axis].y == 0) //left
		{
		  axis_left_press = 1;
		  fake_event(disp, conf->axis_left, 1);
		}
	      if(axes[axis].x == 32767 && axes[axis].y == 0) //left
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
			 		
	    }else{	
	      axis_x_direction = axes[axis].x/32767;
              axis_y_direction = axes[axis].y/32767;
	      //printf("Axis %zu at (%6d, %6d)\n", axis, axes[axis].x, axes[axis].y);
              if(axes[axis].x == 0 && axes[axis].y == 0) 
                pthread_join(mouse_move_thread_t, NULL);
	      else
  	        pthread_create(&mouse_move_thread_t, NULL, mouse_move_thread, (void *)disp);
	    }
	  }
	  break;
	default:
	  /* Ignore init events. */
	  break;
        }
        
      fflush(stdout);
    }

  free(conf);
  XCloseDisplay(disp); 
  close(js);
  return 0;
}
