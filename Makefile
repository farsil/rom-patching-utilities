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
	rm -f "$(DESTDIR)/bin/behead"
	rm -f "$(DESTDIR)/bin/bpspatch"
	rm -f "$(DESTDIR)/bin/ipspatch"

clean:
	rm -f behead
	rm -f bpspatch
	rm -f ipspatch

