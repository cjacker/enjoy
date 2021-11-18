#ifndef KEYTABLE_H
#define KEYTABLE_H

#include <linux/uinput.h>
#include <stdio.h>


typedef struct {
    char *name;
    char *xkeyname;
    int uinpcode;
} keymap;

keymap* get_keymap_by_name(char *name);
void print_keytable();

#endif
