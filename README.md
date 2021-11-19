# enjoy
## A daemon to map joystick events to mouse/key events

Recently, I got a [DevTerm](https://www.clockworkpi.com/devterm) and `enjoy` is specially written for this device to map joystick events to mouse or key events. It should work with other joysticks but not tested.

![DevTerm](https://github.com/cjacker/enjoy/raw/main/DevTerm.png)

## Features

`enjoy` support: 

* support both X and wayland, even console. since `enjoy` use kernel uinput to simulate key/mouse events.

* simulate key event  use 'keyname'. support single or combined keys with `+` delimiter, for example : `super_l+shift_l+q`.
    * for 'keyname' can be used in config file, please use `enjoy -k` for more information.

* simulate key sequence use `keyseq ` prefix and continue with 'keyname' sequence, for example : `keyseq control_l+g c`.

* simulate mouse button, use `mouse_button ` prefix, continue with a 'button number'. 
    * `1` = left button
    * `2` = middle button
    * `3` = right button
    * `4` = scroll up
    * `5` = scroll down.

* simulate mouse motion by set `axis<n>_as_mouse` to `1` in config file. 

* use `enjoyctl` to pause and resume the event mapping as you need.

## Build and Install

`enjoy` have no external dependencies:

```
make
sudo make install
sudo systemctl daemon-reload
sudo systemctl start enjoy@js0
sudo systemctl enable enjoy@js0
```

## Usage: 

enjoy [-D] [-h] [-k] [-c configfile] [-p pidfile]

Args:

`-D`: debug mode, do not daemonize. useful to find which joystick key not mapped.

`-k`: show 'keyname' can be used in config file.

`-c <configfile>`: specify config file.

`-p <pidfile>`: specify pid file.

`-h`: show this message.

For more than one joysticks, You need to create multiple config files under `/etc/enjoy/<device name>`, for example:

If you have two joysticks 'js0' and 'js1', you should create  `/etc/enjoy/js0` for 'js0' and `/etc/enjoy/js1` for 'js1',

and run:

```shell
sudo systemctl start enjoy@js0 
sudo systemctl start enjoy@js1
sudo systemctl enable enjoy@js0 
sudo systemctl enable enjoy@js1
```

if you want to suspend and resume the events mapping, for example: js0,

for suspend:

```
enjoyctl js0 suspend
```

for resume:

```
enjoyctl js0 resume
```


## Default configuration
Since I am a heavy user of i3wm, the default configuration for DevTerm set to:

```ini
# default config file for Devterm.

# joystick device
device=/dev/input/js0

# x button of Devterm
# map to 'cmd' key (win key)
button_0=super_l

# a button of Devterm
# map to mouse right button
button_1=mouse_button 3

# b button of Devterm
# map to mouse left button
button_2=mouse_button 1

# y button of Devterm
# map to control key
button_3=ctrl_l

# select button of Devterm
# map to combined keys
button_8=super_l+end

# start button of Devterm
# map to combined keys
button_9=super_l+d

# use axis as mouse motion.
axis0_as_mouse=1

# map axis to up/down/left/right
# need set axis0_as_mouse to 0,
# otherwise below settings will be ignored.
axis0_button1_up=up
axis0_button1_down=down
axis0_button0_left=left
axis0_button0_right=right

```

And, `enjoy`.

## TODO

* ~~Conver it to a daemon and run only one instance, it should not be a user specific process.~~

* ~~Add a control interface, maybe via unix domain socket to suspend and resume, consider to use epoll to handle multiple fd events.~~

* ~~Finish uinput support and make X optional. after that, enjoy should work well with wayland.~~

* ~~Support verbose mode to found which joystick event generated and not bound.~~

* ~~Support multiple axis on one joystick, maybe with `axis[n]_button[n]_up/down/left/right` keys.~~

* ~~Support multiple joysticks by multiple config files, can launch multiple instances.~~
