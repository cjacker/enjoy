#ifndef UINPUT_H
#define UINPUT_H
int init_uinput();
void close_uinput(int fd);
void emit(int fd, int type, int code, int val);
void fake_mouse_button_uinput(int fd, int button_number, int state);
void fake_key_uinput(int fd, char *keyname, int state);
#endif
