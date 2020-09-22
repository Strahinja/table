BINDIR=/usr/local/bin

all: table

table: table.o
	$(CC) -lunistring -o $@ $<

install: all
	cp table $(BINDIR)

clean: table.o table
	rm table.o table 2>/dev/null

.PHONY: clean all install

