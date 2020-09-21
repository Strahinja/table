all: table

table: table.o
	$(CC) -lunistring -o $@ $<

clean: table.o table
	rm table.o table 2>/dev/null

.PHONY: clean all

