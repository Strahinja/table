BINDIR=/usr/local/bin
MANDIR=/usr/local/share/man/man1
CFLAGS=
#CFLAGS=-g

all: table table.1.gz doc

.o: .c
	$(CC) $(CFLAGS) -o $@ -c $<

%.gz: %
	gzip -kf $<

%.ps: %.1
	groff -mandoc -t -T ps $< > $@

%.pdf: %.1
	groff -mandoc -t -T pdf $< > $@

table: table.o
	$(CC) $(CFLAGS) -lunistring -o $@ $<

doc: table.pdf

install: all
	mkdir -p $(BINDIR) $(MANDIR)
	cp table $(BINDIR)
	cp table.1.gz $(MANDIR)

uninstall:
	rm $(BINDIR)/table $(MANDIR)/table.1.gz 2>/dev/null

clean: table.o table table.1.gz table.pdf
	rm $^ 2>/dev/null

.PHONY: clean all doc install uninstall

