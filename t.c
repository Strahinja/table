/*
 *    Command line utility to format and display a CSV file.
 *    Copyright (C) 2020  Страхиња Радић
 *
 *    This program is free software: you can redistribute it and/or modify
 *    it under the terms of the GNU General Public License as published by
 *    the Free Software Foundation, either version 3 of the License, or
 *    (at your option) any later version.
 *
 *    This program is distributed in the hope that it will be useful,
 *    but WITHOUT ANY WARRANTY; without even the implied warranty of
 *    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *    GNU General Public License for more details.
 *
 *    You should have received a copy of the GNU General Public License
 *    along with this program.  If not, see <https://www.gnu.org/licenses/>.
 *
 */


#include <stdarg.h>
#include <stdio.h>
#include <string.h>
#include "defs.h"

int
version()
{
    printf("%s v%s\n", PROGRAMNAME, VERSION);
    return 0;
}

int
usage()
{
    printf("Usage: %s [-v|--version]\n", PROGRAMNAME);
    return 0;
}

int
error(int code, char *fmt, ...)
{
    char            buf[BUFSIZE];
    va_list         args;
    va_start(args, fmt);
    vsnprintf(buf, sizeof(buf), fmt, args);
    va_end(args);
    fprintf(stderr, "%s: %s", PROGRAMNAME, buf);
    return code;
}

int
main(int argc, char **argv)
{
    char           *arg;
    int             cmd = CMD_NONE;
    while (arg = *argv++) {
	if (*arg++ == '-') {
	    char            c = *arg++;
	    if (c == '-') {
		if (!strcmp(arg, "version")) {
		    cmd = CMD_VERSION;
		} else {
		    error(1, "Invalid argument: --%s\n", arg);
		    return usage();
		}
	    } else {
		switch (c) {
		case 'v':
		    cmd = CMD_VERSION;
		    break;
		default:
		    error(1, "Invalid argument: -%c\n", c);
		    return usage();
		}
	    }
	}
	arg++;
    }

    if (cmd == CMD_VERSION) {
	return version();
    }

    char            line[BUFSIZE];

    do {
	fgets(line, sizeof(line), stdin);
	if (!feof(stdin)) {
	    char           *eol = strchr(line, '\n');
	    if (eol)
		*eol = 0;
	    fprintf(stdout, "%s\n", line);
	}
    }
    while (!feof(stdin));

    /*
     * error(2, "Invalid arguments\n");
     */
    return 0;
}
