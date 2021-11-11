# enjoy
## convert joystick events to mouse/key events

Recently, I got a [DevTerm](https://www.clockworkpi.com/devterm) and `enjoy` is specially written for this device to convert joystick events to mouse click/scroll/motion and key events. It should work with other joysticks but not tested.

![DevTerm](https://github.com/cjacker/enjoy/raw/main/DevTerm.png)

## Build

`enjoy` almost have no dependencies but use libx11 and libxtst to simulate mouse/key events:
```
sudo apt-get install libx11-dev libxtst-dev
make
./enjoy
```

## Usage

Since I am a heavy user of i3wm, the default configuration set to:

```
device=/dev/input/js0
button_0=Super_L
button_1=mouse_button_3
button_2=mouse_button_1
button_3=Control_L
button_4=
button_5=
button_6=
button_7=
button_8=Super_L+End
button_9=Super_L+d
axis_as_mouse=1
axis_up=Up
axis_down=Down
axis_left=Left
axis_right=Right
```

By default, `axis_as_mouse` set to `1`, that means it will ignore all `axis_up/down/left/right` settings and use axis to send mouse motion event.

You can create your own config file as `~/.enjoyrc` to map buttons and axis to mouse click/scroll/motion or key event.

It support map combined keys with '+', for example:

```
button_a=Super_L+Shift_L+q
```

If you need to map to mouse click/scroll event, please use:
```
mouse_click_<n>
n=1: left button
n=2: middle button
n=3: right button
n=4: scroll up
n=5: scroll down
```

If you need to map axis to key or mouse click event, please set:
```
axis_as_mouse=0
```

and map `axis_up/down/left/right` to which key or mouse button you need.

Finally, `enjoy`.

