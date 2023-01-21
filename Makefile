DESTDIR=/usr/local

all: bpspatch behead

bpspatch:
	gcc -o bpspatch bpspatch.c

behead:
	gcc -o behead behead.c

install: all
	mkdir -p "$(DESTDIR)/bin"
	install -m 755 bpspatch behead "$(DESTDIR)/bin" 

uninstall:
	rm "$(DESTDIR)/bin/behead"
	rm "$(DESTDIR)/bin/bpspatch"

clean:
	rm behead
	rm bpspatch

