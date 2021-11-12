# enjoy
## map joystick events to mouse/key events

Recently, I got a [DevTerm](https://www.clockworkpi.com/devterm) and `enjoy` is specially written for this device to map joystick events to mouse click/scroll/motion or key events, or use joystick buttons to launch applications. It should work with other joysticks but not tested.

![DevTerm](https://github.com/cjacker/enjoy/raw/main/DevTerm.png)

## Build

`enjoy` almost have no dependencies but use libx11 and libxtst to simulate mouse/key events:

```
sudo apt-get install libx11-dev libxtst-dev
make
./enjoy
```

## Usage

`enjoy` support:

* simulate key use 'xlib keyname'. it support combined keys with `+`, for example : `Super_L+Shift_L+q`.

* simulate key sequence, use `keyseq ` prefix and continue with xlib keynames, for example : `keyseq Control_L+g c`.

* simulate mouse button, use `mouse_button ` prefix, continue with a `button number`. 
    * `1` = left button, `2` = middle button, `3` = right button, `4` = scroll up, `5` = scroll down.

* launch application with `exec ` prefix, for example : `exec st -D`.

* simulate mouse motion by set `axis_as_mouse` to `1`. 


Since I am a heavy user of i3wm, the default configuration set to:

```ini
# joystick device
device=/dev/input/js0
# button x of DevTerm
button_0=Super_L
# button a of DevTerm
button_1=mouse_button 3
# button b of DevTerm
button_2=mouse_button 1
# button y of DevTerm
button_3=Control_L
# button select of DevTerm
button_8=Super_L+End
# button start of DevTerm
button_9=Super_L+d
# set it to 0 to use axis to simulate key/mouse event.
axis_as_mouse=1
axis_up=Up
axis_down=Down
axis_left=Left
axis_right=Right

```

You can create your own config file as `~/.config/enjoyrc`.

And, `enjoy`.

## TODO

* Support verbose mode to found which joystick event generated and not bound.

* Support multiple axis on one joystick, maybe with `axis[n]_button[n]_up/down/left/right` keys.

* Support multiple joysticks by multiple config files, can launch multiple instances.
