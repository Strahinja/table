/*
 *    Command line utility to format and display CSV.
 *    Copyright (C) 2020  Страхиња Радић
 *
 *    This program is free software: you can redistribute it and/or modify it
 *    under the terms of the GNU General Public License as published by the Free
 *    Software Foundation, either version 3 of the License, or (at your option)
 *    any later version.
 *
 *    This program is distributed in the hope that it will be useful, but
 *    WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY
 *    or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License
 *    for more details.
 *
 *    You should have received a copy of the GNU General Public License along
 *    with this program.  If not, see <https://www.gnu.org/licenses/>.
 *
 */

#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistr.h>
#include <unistdio.h>
#include <uniwidth.h>
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
    printf("Usage: %s [options]\n", PROGRAMNAME);
    printf("    Where [options] is one or more of the following:\n");
    printf("        -h or --help       Prints this usage information screen\n");
    printf("        -v or --version    Prints program version and exits\n");
    return 0;
}

int
error(int code, uint8_t* fmt, ...)
{
    uint8_t    buf[BUFSIZE];
    va_list args;
    va_start(args, fmt);
    u8_vsnprintf(buf, sizeof(buf), fmt, args);
    va_end(args);
    fprintf(stderr, "%s: %s", PROGRAMNAME, buf);
    return code;
}

uint8_t*
substr(uint8_t* src, int start, int finish)
{
    int len = u8_strlen(src);
    if (finish > len)
        finish = len;
    int substr_len = finish-start;
    if (substr_len < 0)
        substr_len = 0;
    uint8_t* result = (uint8_t*) malloc(sizeof(uint8_t) * (substr_len+1));

    for (int i = start; i < finish && *(src+i) != '\0'; i++)
    {
        *result++ = *(src+i);
    }
    *result = '\0';

    return result-substr_len;
}

int
main(int argc, char** argv)
{
    char* arg;
    Command cmd = CMD_NONE;

    while ((arg = *argv++))
    {
        if (*arg++ == '-')
        {
            char c = *arg++;
            if (c == '-')
            {
                if (!strcmp(arg, "version"))
                {
                    cmd = CMD_VERSION;
                }
                else
                {
                    error(1, "Invalid argument: --%s\n", arg);
                    return usage();
                }
            }
            else
            {
                switch (c)
                {
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

    if (cmd == CMD_VERSION)
    {
        return version();
    }

    uint8_t line[BUFSIZE];

    static const uint8_t* table_symbols[][9] =
    {
        [TABLE_SYMBOLS_ASCII] = {
            // ascii
            (uint8_t*)"+", (uint8_t*)"-", (uint8_t*)"+",
            (uint8_t*)"|", (uint8_t*)" ", (uint8_t*)"|",
            (uint8_t*)"+", (uint8_t*)"-", (uint8_t*)"+",
        },
        [TABLE_SYMBOLS_SINGLE] = {
            // single
            (uint8_t*)"\u250c",  (uint8_t*)"\u2500",  (uint8_t*)"\u2510",
            (uint8_t*)"\u2502",  (uint8_t*)" ",  (uint8_t*)"\u2502",
            (uint8_t*)"\u2514",  (uint8_t*)"\u2500",  (uint8_t*)"\u2518",
        },
        [TABLE_SYMBOLS_DOUBLE] = {
            // double
            (uint8_t*)"\u2554",  (uint8_t*)"\u2550",  (uint8_t*)"\u2557",
            (uint8_t*)"\u2551",  (uint8_t*)" ",  (uint8_t*)"\u2551",
            (uint8_t*)"\u255a",  (uint8_t*)"\u2550",  (uint8_t*)"\u255d",
        }
    };
    int current_symbol_set = TABLE_SYMBOLS_DOUBLE;

    fprintf(stdout, "%s", table_symbols[current_symbol_set][0]);
    int col = 0;
    int cols = 80;
    while (col++ < cols-2)
        fprintf(stdout, "%s", table_symbols[current_symbol_set][1]);
    fprintf(stdout, "%s\n", table_symbols[current_symbol_set][2]);

    do
    {
        fgets(line, sizeof(line), stdin);
        if (!feof(stdin))
        {
            uint8_t* eol = u8_strchr(line, '\n');
            if (eol)
                *eol = '\0';
            fprintf(stdout, "%s", table_symbols[current_symbol_set][3]);
            /*uint8_t format[BUFSIZE];*/
            /*sprintf(format, "%%%ds", cols-2);*/
            uint8_t* cropped_line = substr(line, 0, cols-2);
            fprintf(stdout, "%s", cropped_line);
            /*for (int i = 0; i < cols-2-u8_mblen(cropped_line, BUFSIZE); i++)*/
            for (int i = 0; i < cols-2-u8_strwidth(cropped_line, "utf-8"); i++)
                /*for (int i = 0; i < cols-2-strlen(cropped_line); i++)*/
            {
                fprintf(stdout, " ");
            }
            fprintf(stdout, "%s\n", table_symbols[current_symbol_set][5]);
            /*fprintf(stdout, "u8_strlen(s) = %d\n", u8_strlen(cropped_line));*/
        }
    }
    while (!feof(stdin));
    fprintf(stdout, "%s", table_symbols[current_symbol_set][6]);
    col = 0;
    cols = 80;
    while (col++ < cols-2)
        fprintf(stdout, "%s", table_symbols[current_symbol_set][7]);
    fprintf(stdout, "%s\n", table_symbols[current_symbol_set][8]);

    /*error(2, "Invalid arguments\n");*/
    return 0;
}


