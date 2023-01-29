DESTDIR=/usr/local

all: bpspatch behead ipspatch

behead:
	gcc -o behead behead.c

bpspatch:
	gcc -o bpspatch bpspatch.c futils.c

ipspatch:
	gcc -o ipspatch ipspatch.c futils.c

install: all
	mkdir -p "$(DESTDIR)/bin"
	install -m 755 behead bpspatch ipspatch "$(DESTDIR)/bin"

uninstall:
	rm "$(DESTDIR)/bin/behead"
	rm "$(DESTDIR)/bin/bpspatch"
	rm "$(DESTDIR)/bin/ipspatch"

clean:
	rm behead
	rm bpspatch
	rm ipspatch

