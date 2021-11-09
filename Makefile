all:
	gcc -Os -o enjoy enjoy.c -lX11 -lXtst -pthread
