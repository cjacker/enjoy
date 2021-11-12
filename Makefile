all:
	gcc -Os -o enjoy enjoy.c cfg_parse.c uinput.c -lX11 -lXtst -pthread

install:
	install -m0644 99-uinput.rules /etc/udev/rules.d/
	install -m0755 enjoy /usr/bin

clean:
	rm enjoy
