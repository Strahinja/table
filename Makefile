BINDIR=/usr/local/bin
MANDIR=/usr/local/share/man/man1
CFLAGS=
#CFLAGS=-g

all: table table.1.gz

.o: .c
	$(CC) $(CFLAGS) -o $@ -c $<

%.gz: %
	gzip -k $<

table: table.o
	$(CC) $(CFLAGS) -lunistring -o $@ $<

install: all
	mkdir -p $(BINDIR) $(MANDIR)
	cp table $(BINDIR)
	cp table.1.gz $(MANDIR)

uninstall:
	rm $(BINDIR)/table $(MANDIR)/table.1.gz 2>/dev/null

clean: table.o table table.1.gz
	rm table.o table table.1.gz 2>/dev/null

.PHONY: clean all install uninstall

