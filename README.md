# enjoy
## convert joystick events to mouse/key events

Recently, I got a device named [DevTerm](https://www.clockworkpi.com/devterm) and `enjoy` is specially written for this device to convert joystick events to mouse/key events.

> 最近大家都'捐'，我也想把这2小时写完的代码捐给**开放原子基金会**。。。

> (If you do not understand, it's joke, just ignore it)

The hardcode configuration is:

```
device=/dev/input/js0
button_a=mouse_click_1
button_b=mouse_click_3
button_x=Control_L
button_y=Shift_L
button_select=Super_L+End
button_start=Super_L+d
axis_as_mouse=1
axis_up=Up
axis_down=Down
axis_left=Left
axis_right=Right
```

By default, `axis_as_mouse` set to `1`, that means it will ignore all `axis_up/down/left/right` settings and use axis as a mouse.

You can create your own configuration file as `~/.enjoyrc` to map buttons and axis to mouse or key event.

It support map combined keys, for example:

```
button_a=Super_L+Shift_L+q
```

If you want to map to mouse click event, please use:
```
mouse_click_<n>
n=1: left button
n=2: middle button
n=3: right button
```

If you want to map axis to keys or mouse button, please set:
```
axis_as_mouse=0
```

Finally, `enjoy`.

