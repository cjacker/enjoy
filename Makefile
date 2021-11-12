PREFIX = /usr

all:
	gcc -Os -o enjoy enjoy.c cfg_parse.c uinput.c keytable.c -pthread
	gcc -Os -o enjoy-with-x enjoy.c cfg_parse.c uinput.c keytable.c x.c -lX11 -lXtst -pthread -DWITH_X

install:
	mkdir -p $(DESTDIR)/etc/udev/rules.d/
	mkdir -p $(DESTDIR)$(PREFIX)/bin/
	install -D -m0644 99-uinput.rules $(DESTDIR)/etc/udev/rules.d/
	install -D -m0755 enjoy $(DESTDIR)$(PREFIX)/bin

clean:
	rm -rf enjoy enjoy-with-x
