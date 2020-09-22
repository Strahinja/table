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
    printf("Usage: %s [options] [filename]\n", PROGRAMNAME);
    printf("    Where [options] is one or more of the following:\n");
    printf("        -c [cols]\n");
    printf("        --columns=[cols]\n");
    printf("                    Set maximum table width in columns (default 80)\n");
    printf("\n");
    printf("        -h\n");
    printf("        --help\n");
    printf("                    Prints this usage information screen\n");
    printf("\n");
    printf("        -s [set]\n");
    printf("        --symbols=[set]\n");
    printf("                    Use table symbol set [set], where [set] is one\n");
    printf("                    of the following:\n");
    printf("                        ascii | single | double (default)\n");
    printf("\n");
    printf("        -v\n");
    printf("        --version\n");
    printf("                    Prints program version and exits\n");
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

char*
substr(char* src, int start, int finish)
{
    int len = strlen(src);
    if (finish > len)
        finish = len;
    int substr_len = finish-start;
    if (substr_len < 0)
        substr_len = 0;
    char* result = (char*) malloc(sizeof(char) * (substr_len+1));

    for (int i = start; i < finish && *(src+i) != '\0'; i++)
    {
        *result++ = *(src+i);
    }
    *result = '\0';

    return result-substr_len;
}

uint8_t*
u8_substr(uint8_t* src, int start, int finish)
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
set_symbol_set(char* arg, int* current_symbol_set)
{
    if (!strcmp(arg, "ascii"))
    {
        *current_symbol_set = TABLE_SYMBOLS_ASCII;
    }
    else if (!strcmp(arg, "single"))
    {
        *current_symbol_set = TABLE_SYMBOLS_SINGLE;
    }
    else if (!strcmp(arg, "double"))
    {
        *current_symbol_set = TABLE_SYMBOLS_DOUBLE;
    }
    else
    {
        return error(1, (uint8_t*)"Invalid symbol set: %s\n", arg);
    }
    return 0;
}

int
set_columns(char* arg, int* cols)
{
    int c = strtol(arg, NULL, 10);
    if (errno == EINVAL || errno == ERANGE)
    {
        return error(1, "Invalid numeric value: %s\n", arg);
    }
    else
    {
        *cols = c;
    }
    return 0;
}

int
main(int argc, char** argv)
{
    char* arg;
    Command cmd = CMD_NONE;
    char* filename = NULL;
    int current_symbol_set = TABLE_SYMBOLS_DOUBLE;
    int cols = 80;

    while ((arg = *++argv))
    {
        if (*arg == '-')
        {
            arg++;
            char c = *arg++;
            int result;
            if (c == '-')
            {
                if (!strcmp(arg, "version"))
                {
                    cmd = CMD_VERSION;
                }
                else if (!strcmp(substr(arg, 0, strlen("columns=")), "columns="))
                {
                    arg += strlen("columns=");
                    result = set_columns(arg, &cols);
                    if (result)
                        return result;
                }
                else if (!strcmp(substr(arg, 0, strlen("symbols=")), "symbols="))
                {
                    arg += strlen("symbols=");
                    result = set_symbol_set(arg, &current_symbol_set);
                    if (result)
                        return result;

                }
                else if (!strcmp(arg, "help"))
                {
                    return usage();
                }
                else
                {
                    error(1, (uint8_t*)"Invalid argument: --%s\n", arg);
                    return usage();
                }
            }
            else
            {
                switch (c)
                {
                case 'c':
                    cmd = CMD_COLUMNS;
                    break;
                case 'h':
                    return usage();
                    break;
                case 's':
                    cmd = CMD_SYMBOLS;
                    break;
                case 'v':
                    cmd = CMD_VERSION;
                    break;
                default:
                    error(1, (uint8_t*)"Invalid argument: -%c\n", c);
                    return usage();
                }
            }
        }
        else
        {
            int result;
            if (cmd == CMD_SYMBOLS)
            {
                result = set_symbol_set(arg, &current_symbol_set);
                if (result)
                    return result;
                cmd = CMD_NONE;
            }
            else if (cmd == CMD_COLUMNS)
            {
                result = set_columns(arg, &cols);
                if (result)
                    return result;
                cmd = CMD_NONE;
            }
            else
            {
                filename = arg;
            }
        }
    }

    if (cmd == CMD_VERSION)
    {
        return version();
    }

    uint8_t line[BUFSIZE];

    FILE* input = NULL;
    if (filename)
    {
        input = fopen(filename, "r");
        if (!input)
        {
            return error(ENOENT, "File not found: %s\n", filename);
        }
    }
    else
        input = stdin;

    fprintf(stdout, "%s", table_symbols[current_symbol_set][0]);
    int col = 0;
    while (col++ < cols-2)
        fprintf(stdout, "%s", table_symbols[current_symbol_set][1]);
    fprintf(stdout, "%s\n", table_symbols[current_symbol_set][2]);

    do
    {
        fgets(line, sizeof(line), input);
        if (!feof(input))
        {
            uint8_t* eol = u8_strchr(line, '\n');
            if (eol)
                *eol = '\0';
            fprintf(stdout, "%s", table_symbols[current_symbol_set][3]);
            /*uint8_t format[BUFSIZE];*/
            /*sprintf(format, "%%%ds", cols-2);*/
            uint8_t* cropped_line = u8_substr(line, 0, cols-2);
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
    while (!feof(input));
    fprintf(stdout, "%s", table_symbols[current_symbol_set][6]);
    col = 0;
    while (col++ < cols-2)
        fprintf(stdout, "%s", table_symbols[current_symbol_set][7]);
    fprintf(stdout, "%s\n", table_symbols[current_symbol_set][8]);

    /*error(2, "Invalid arguments\n");*/
    return 0;
}


