PREFIX = /usr

all:
	gcc -Os -o enjoy enjoy.c cfg_parse.c uinput.c keytable.c daemon.c -pthread

install: all
	mkdir -p $(DESTDIR)/usr/lib/systemd/systemd
	install -D -m0644 enjoy@.service $(DESTDIR)/usr/lib/systemd/systemd/
	mkdir -p $(DESTDIR)/etc/enjoy
	install -D -m0644 js0 $(DESTDIR)/etc/enjoy/	
	mkdir -p $(DESTDIR)$(PREFIX)/bin/
	install -D -m0755 enjoy $(DESTDIR)$(PREFIX)/bin

clean:
	rm -rf enjoy
