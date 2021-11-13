# enjoy
## map joystick events to mouse/key events

Recently, I got a [DevTerm](https://www.clockworkpi.com/devterm) and `enjoy` is specially written for this device to map joystick events to mouse or key events. It should work with other joysticks but not tested.

![DevTerm](https://github.com/cjacker/enjoy/raw/main/DevTerm.png)

## Features

`enjoy` support: 

* support both X and wayland, even console. since `enjoy` use kernel uinput to simulate key/mouse events.

* simulate key event  use 'keyname'. support single or combined keys with `+` delimiter, for example : `super_l+shift_l+q`.

* simulate key sequence use `keyseq ` prefix and continue with 'keyname', for example : `keyseq control_l+g c`.

* simulate mouse button, use `mouse_button ` prefix, continue with a 'button number'. 
    * `1` = left button
    * `2` = middle button
    * `3` = right button
    * `4` = scroll up
    * `5` = scroll down.

* launch application with `exec ` prefix, for example : `exec st -D`.

* simulate mouse motion by set `axis_as_mouse` to `1`. 

## Build and Install

`enjoy` have no dependencies:

```
make
sudo make install
# need reboot or reload udev rules
# to set correct permission of /dev/uinput.
udevadm control --reload-rules
enjoy
```

If you want to build with Xtst support(maybe slow performance than uinput), use:

```
make withx
enjoy-with-x -x
```

## Usage

`-D` : enable debug mode. very helpful to find out which joystick "key" should be mapped.

`-n` : ignore default configuration.

`-k` : print out 'enjoy keyname' can be used in config file.

`-c <config file name>` : load another config file instead of `~/.config/enjoyrc`. 

`-h` : show help message.

For more than one joysticks, You may need to create multiple config files under `~/.config` and launch multiple `enjoy` instance.

If you build enjoy with Xtest support:

`-x` : to use 'Xtest' instead of 'uinput' to simulate mouse/key events.

## Default configuration
Since I am a heavy user of i3wm, the default configuration for DevTerm set to:

```ini
# joystick device
device=/dev/input/js0
# button x of DevTerm
button_0=super_l
# button a of DevTerm
button_1=mouse_button 3
# button b of DevTerm
button_2=mouse_button 1
# button y of DevTerm
button_3=control_l
# button select of DevTerm
button_8=super_l+end
# button start of DevTerm
button_9=super_l+d
# set it to 0 to use axis to simulate key/mouse event.
axis0_as_mouse=1
axis0_button0_up=up
axis0_bttton0_down=down
axis0_button1_left=left
axis0_button1_right=right

```

You can create your own config file such as `~/.config/enjoyrc` or `~/.config/<your config file>`.

And, `enjoy`.

## TODO

* ~~Finish uinput support and make X optional. after that, enjoy should work well with wayland.~~

* ~~Support verbose mode to found which joystick event generated and not bound.~~

* ~~Support multiple axis on one joystick, maybe with `axis[n]_button[n]_up/down/left/right` keys.~~

* ~~Support multiple joysticks by multiple config files, can launch multiple instances.~~
