#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <linux/uinput.h>

#include "keytable.h"

keymap keytable[] = {
{"escape",      "Escape",       KEY_ESC},
{"f1",          "F1",           KEY_F1},
{"f2",          "F2",           KEY_F2},
{"f3",          "F3",           KEY_F3},
{"f4",          "F4",           KEY_F4},
{"f5",          "F5",           KEY_F5},
{"f6",          "F6",           KEY_F6},
{"f7",          "F7",           KEY_F7},
{"f8",          "F8",           KEY_F8},
{"f9",          "F9",           KEY_F9},
{"f10",         "F10",          KEY_F10},
{"f11",         "F11",          KEY_F11},
{"f12",         "F12",          KEY_F12},
{"home",        "Home",         KEY_HOME},
{"end",         "End",          KEY_END},
{"up",          "Up",           KEY_UP},
{"down",        "Down",         KEY_DOWN},
{"left",        "Left",         KEY_LEFT},
{"right",       "Right",        KEY_RIGHT},
{"insert",      "Insert",       KEY_INSERT},
{"delete",      "Delete",       KEY_DELETE},
{"pageup",      "Prior",        KEY_PAGEUP},
{"pagedown",    "Next",         KEY_PAGEDOWN},
{"1",           "1",            KEY_1},
{"2",           "2",            KEY_2},
{"3",           "3",            KEY_3},
{"4",           "4",            KEY_4},
{"5",           "5",            KEY_5},
{"6",           "6",            KEY_6},
{"7",           "7",            KEY_7},
{"8",           "8",            KEY_8},
{"9",           "9",            KEY_9},
{"0",           "0",            KEY_0},
{"-",           "minus",        KEY_MINUS}, 
{"=",           "equal",        KEY_EQUAL}, 
{"`",           "grave",        KEY_GRAVE}, 
{"[",           "bracketleft",  KEY_LEFTBRACE},
{"]",           "bracketright", KEY_RIGHTBRACE},
{";",           "semicolon",    KEY_SEMICOLON},
{"'",           "apostrophe",   KEY_APOSTROPHE},
{"\\",          "backslash",    KEY_BACKSLASH},
{",",           "comma",        KEY_COMMA},
{".",           "period",       KEY_DOT},
{"/",           "slash",        KEY_SLASH},
{"space",       "space",        KEY_SPACE},
{"capslock",    "Caps_Lock",    KEY_CAPSLOCK},
{"control_l",   "Control_L",    KEY_LEFTCTRL},
{"control_r",   "Control_R",    KEY_RIGHTCTRL},
{"shift_l",     "Shift_L",      KEY_LEFTSHIFT},
{"shift_r",     "Shift_R",      KEY_RIGHTSHIFT},
{"alt_l",       "Alt_L",        KEY_LEFTALT},
{"alt_r",       "Alt_R",        KEY_RIGHTALT},
{"meta_l",      "Meta_L",       KEY_LEFTMETA},
{"meta_r",      "Meta_R",       KEY_RIGHTMETA},
{"super_l",     "Super_L",      KEY_LEFTMETA},
{"super_r",     "Super_R",      KEY_RIGHTMETA},
{"backspace",   "BackSpace",    KEY_SPACE}, 
{"tab",         "Tab",          KEY_TAB},
{"return",      "Return",       KEY_ENTER},
{"numlock",     "Num_Lock",     KEY_NUMLOCK},
{"scrolllock",  "Scroll_Lock",  KEY_SCROLLLOCK},
{"menu",        "Menu",         KEY_MENU},
{"print",       "Print",        KEY_PRINT},
{"sysrq",       "Sys_Req",      KEY_SYSRQ},
{"break",       "Break",        KEY_BREAK},
{"a",           "a",            KEY_A},
{"b",           "b",            KEY_B},
{"c",           "c",            KEY_C},
{"d",           "d",            KEY_D},
{"e",           "e",            KEY_E},
{"f",           "f",            KEY_F},
{"g",           "g",            KEY_G},
{"h",           "h",            KEY_H},
{"i",           "i",            KEY_I},
{"j",           "j",            KEY_J},
{"k",           "k",            KEY_K},
{"l",           "l",            KEY_L},
{"m",           "m",            KEY_M},
{"n",           "n",            KEY_N},
{"o",           "o",            KEY_O},
{"p",           "p",            KEY_P},
{"q",           "q",            KEY_Q},
{"r",           "r",            KEY_R},
{"s",           "s",            KEY_S},
{"t",           "t",            KEY_T},
{"u",           "u",            KEY_U},
{"v",           "v",            KEY_V},
{"w",           "w",            KEY_W},
{"x",           "x",            KEY_X},
{"y",           "y",            KEY_Y},
{"z",           "z",            KEY_Z},
{"brightnessdown",  "XF86MonBrightnessDown",    KEY_BRIGHTNESSDOWN},
{"brightnessup",    "XF86MonBrightnessUp",      KEY_BRIGHTNESSUP},
{"eject",           "XF86Eject",                KEY_EJECTCD},
{"mute",            "XF86AudioMute",            KEY_MUTE},
{"micmute",         "XF86AudioMicMute",         KEY_MICMUTE},
{"volumedown",      "XF86AudioLowerVolume",     KEY_VOLUMEDOWN},
{"volumeup",        "XF86AudioRaiseVolume",     KEY_VOLUMEUP},
{"bluetooth",       "XF86Bluetooth",            KEY_BLUETOOTH},
{"keyboard",        "",                         KEY_KEYBOARD},
{"favorites",       "XF86Favorites",            KEY_BOOKMARKS},
{"display",         "XF86Display",              KEY_SWITCHVIDEOMODE},
{"wlan",            "XF86WLAN",                 KEY_WLAN},
{NULL,      NULL,       0},
};

keymap* get_keymap_by_name(char *name)
{

    keymap *km = malloc(sizeof(keymap));
    km->name = NULL;
    km->xkeyname = NULL;
    km->uinpcode = 0;

    int i = 0;
    while(keytable[i].name != NULL) {
        if(!strcmp(keytable[i].name, name)) {
            //fprintf(stderr, "get_keymap_by_name: %s, %s, %d\n", keytable[i].name, keytable[i].xkeyname, keytable[i].uinpcode);
            km->name=strdup(keytable[i].name);
            km->xkeyname=strdup(keytable[i].xkeyname);
            km->uinpcode = keytable[i].uinpcode;
            return km;
        }
        i++;
    }
    return km;
}

void print_keytable()
{
	printf("%-15s|%-22s|%-15s\n", "enjoy keyname", "x keyname", "uinput code");
    printf("--------------------------------------------------\n");
    int i = 0;
    while(keytable[i].name != NULL) {
		printf("%-15s|%-22s|%-15d\n", keytable[i].name, keytable[i].xkeyname, keytable[i].uinpcode);
        i++;
    }
}
