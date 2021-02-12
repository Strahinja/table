redo-ifchange $2.c
${TABLE_CC:-gcc} -g -Wall -std=c99 -o $3 -c $2.c

