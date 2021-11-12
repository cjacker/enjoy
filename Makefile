all:
	gcc -g -Os -o enjoy enjoy.c cfg_parse.c uinput.c -lX11 -lXtst -pthread
	clang -g -Os -o enjoy enjoy.c cfg_parse.c uinput.c -lX11 -lXtst -pthread
clean:
	rm enjoy
