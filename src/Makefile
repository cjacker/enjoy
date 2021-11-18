PREFIX = /usr

all:
	gcc -Os -o enjoy enjoy.c cfg_parse.c uinput.c keytable.c daemon.c -pthread
	gcc -Os -o enjoyctl enjoyctl.c
clean:
	rm -rf enjoy enjoyctl
