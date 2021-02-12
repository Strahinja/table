redo-ifchange table.o table.c defs.h
${TABLE_CC:-gcc} -g -Wall -std=c99 -o $3 table.o -lunistring

