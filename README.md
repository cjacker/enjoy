# enjoy
## convert joystick events to mouse/key events

Recently, I got a [DevTerm](https://www.clockworkpi.com/devterm) and `enjoy` is specially written for this device to convert joystick events to mouse click/scroll/motion or key events, or use joystick buttons to launch applications. It should work with other joysticks but not tested.

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

```ini
#joystick device
device=/dev/input/js0
# button x
button_0=Super_L
# button a
button_1=mouse_button_3
# button b
button_2=mouse_button_1
# button y 
button_3=Control_L
# button select
button_8=Super_L+End
# button start
button_9=Super_L+d
# axis simulate mouse mition
# set it to 0 if you want to simulate key/mouse click event.
axis_as_mouse=1
axis_up=Up
axis_down=Down
axis_left=Left
axis_right=Right

```

Set `axis_as_mouse` to `1` will use axis to simulate mouse motion event and ignore `axis_up/down/left/right` settings.

You can create your own config file as `~/.config/enjoyrc` to map buttons and axis to mouse click/scroll/motion or key event, or launch application.

It support map combined keys with '+', for example:

```
button_1=Super_L+Shift_L+q
```

If you need to map to mouse click/scroll event, please use:
```
mouse_button_<n>
n=1: left button
n=2: middle button
n=3: right button
n=4: scroll up
n=5: scroll down
```

If you need to launch a application, please use:
```
button_1 = exec application [args]
```

If you need to map axis to key or mouse click event, please set:
```
axis_as_mouse=0
```
and map `axis_up/down/left/right` to which key or mouse button you need.

Finally, `enjoy`.

