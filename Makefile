PREFIX = /usr

all:
	make all -C src

install: 
	mkdir -p $(DESTDIR)/usr/lib/systemd/system
	install -D -m0644 enjoy@.service $(DESTDIR)/usr/lib/systemd/system/
	mkdir -p $(DESTDIR)/etc/enjoy
	install -D -m0644 js0 $(DESTDIR)/etc/enjoy/	
	mkdir -p $(DESTDIR)$(PREFIX)/bin/
	install -D -m0755 src/enjoy $(DESTDIR)$(PREFIX)/bin
	install -D -m0755 src/enjoyctl $(DESTDIR)$(PREFIX)/bin

clean:
	make clean -C src
