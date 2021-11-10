# enjoy
## convert joystick events to mouse/key events

Recently, I got a [DevTerm](https://www.clockworkpi.com/devterm) and `enjoy` is specially written for this device to convert joystick events to mouse click/scroll/motion and key events.

![DevTerm](https://github.com/cjacker/enjoy/raw/main/DevTerm.png)

Since I am a heavy user of i3wm, the default configuration set to:

```
device=/dev/input/js0
button_a=mouse_click_3
button_b=mouse_click_1
button_x=Super_L
button_y=Control_L
button_select=Super_L+End
button_start=Super_L+d
axis_as_mouse=1
axis_up=Up
axis_down=Down
axis_left=Left
axis_right=Right
```

By default, `axis_as_mouse` set to `1`, that means it will ignore all `axis_up/down/left/right` settings and use axis to send mouse motion event.

You can create your own config file as `~/.enjoyrc` to map buttons and axis to mouse motion/click or key event.

It support map combined keys, for example:

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

And map `axis_up/down/left/right` to which key or mouse button you need.

Finally, `enjoy`.

