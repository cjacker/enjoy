#ifndef UINPUT_H
#define UINPUT_H
void emit(int fd, int type, int code, int val);
int init_uinput();
void close_uinput(int fd);
#endif
