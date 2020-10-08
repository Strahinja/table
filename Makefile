BINDIR=/usr/local/bin
CFLAGS=
#CFLAGS=-g

all: table

.o: .c
	$(CC) $(CFLAGS) -o $@ -c $<

table: table.o
	$(CC) $(CFLAGS) -lunistring -o $@ $<

install: all
	cp table $(BINDIR)

clean: table.o table
	rm table.o table 2>/dev/null

.PHONY: clean all install

