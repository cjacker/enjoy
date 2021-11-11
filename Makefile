all:
	gcc -g -Os -o enjoy enjoy.c cfg_parse.c -lX11 -lXtst -pthread
clean:
	rm enjoy
