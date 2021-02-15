/*
 *    table - Command line utility to format and display CSV.
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

#define CHECKEXITNOMEM(ptr) { if (!ptr) exit(error(ENOMEM, \
                (uint8_t*)"Memory allocation failed (out of memory?)\n")); }

#define CALLOC(ptr, ptrtype, nmemb) { ptr = calloc(nmemb, sizeof(ptrtype)); \
    CHECKEXITNOMEM(ptr) }

#define REALLOC(ptr, ptrtype, newsize) { ptrtype* newptr = realloc(ptr, newsize); \
    CHECKEXITNOMEM(newptr) \
    ptr = newptr; }

#define REALLOCARRAY(ptr, membtype, newcount) \
    REALLOC(ptr, membtype, sizeof(membtype) * newcount)

#define WITHIN_COLUMN (current_rune_column < \
        (current_table_column+1) * max_table_column_width \
            + current_table_column)

#define WITHIN_SINGLE_COLUMN (current_rune_column < rune_columns-2)

int
version()
{
    printf("%s v%s\n", PROGRAMNAME, VERSION);
    return 0;
}

int
usage()
{
    printf("Usage: %s [-b|--border-mode] [-c <cols>|--columns=<cols>] [-d"
            " <delim>|--delimiter=<delim>] [-h|--help] [-n|--no-ansi] [-s"
            " <set>|--symbols=<set>] [-t|--expand-tabs] [-v|--version]\n",
                PROGRAMNAME);
    return 0;
}

int
error(int code, uint8_t* fmt, ...)
{
    uint8_t    buf[BUFSIZE];
    va_list args;
    va_start(args, fmt);
    u8_vsnprintf(buf, sizeof(buf), (const char*)fmt, args);
    va_end(args);
    fprintf(stderr, "%s: %s", PROGRAMNAME, buf);
    return code;
}

char*
substr(const char* src, int start, int finish)
{
    int len = strlen(src);
    if (finish > len)
        finish = len;
    int substr_len = finish-start;
    if (substr_len < 0)
        substr_len = 0;
    char* result = NULL;
    CALLOC(result, char, substr_len+1)
    char* presult = result;
    for (int i = start; i < finish && *(src+i) != '\0'; i++)
        *presult++ = *(src+i);
    *presult = 0;

    return result;
}

BOOL
startswith(const char* s, const char* what)
{
    if (!s || !what)
        return 0;

    char* subs = substr(s, 0, strlen(what));
    BOOL result = !strcmp(subs, what);

    free(subs);

    return result;
}

int
set_symbol_set(char* arg, int* current_symbol_set,
               int* current_inner_symbol_set)
{
    if (!strcmp(arg, "aa"))
    {
        *current_symbol_set = TABLE_SYMBOLS_ASCII;
        *current_inner_symbol_set = TABLE_INNER_ASCII_ASCII;
    }
    else if (!strcmp(arg, "ss"))
    {
        *current_symbol_set = TABLE_SYMBOLS_SINGLE;
        *current_inner_symbol_set = TABLE_INNER_SINGLE_SINGLE;
    }
    else if (!strcmp(arg, "sd"))
    {
        *current_symbol_set = TABLE_SYMBOLS_SINGLE;
        *current_inner_symbol_set = TABLE_INNER_SINGLE_DOUBLE;
    }
    else if (!strcmp(arg, "ds"))
    {
        *current_symbol_set = TABLE_SYMBOLS_DOUBLE;
        *current_inner_symbol_set = TABLE_INNER_DOUBLE_SINGLE;
    }
    else if (!strcmp(arg, "dd"))
    {
        *current_symbol_set = TABLE_SYMBOLS_DOUBLE;
        *current_inner_symbol_set = TABLE_INNER_DOUBLE_DOUBLE;
    }
    else
    {
        return error(1, (uint8_t*)"Invalid symbol set: %s\n", arg);
    }
    return 0;
}

int
set_columns(const char* arg, size_t* cols)
{
    size_t c = strtol(arg, NULL, 10);
    if (errno == EINVAL || errno == ERANGE)
        return error(1, (uint8_t*)"Invalid numeric value: %s\n", arg);
    else
        *cols = c;
    return 0;
}

int
set_delimiter(uint8_t* arg, ucs4_t* delimiter)
{
    size_t arg_len = u8_strlen(arg);
    size_t delimiter_len = 0;
    uint8_t* parg = arg;
    ucs4_t uch;

    while (parg && *parg)
    {
        delimiter_len = u8_mbtouc(&uch, parg, arg_len);
        if (uch != (ucs4_t)'"' && uch != (ucs4_t)'\'')
        {
            *delimiter = uch;
            return 0;
        }
        parg += delimiter_len;
    }

    return -1;
}

/* Return number of columns based on the input line */
size_t
number_of_columns(const uint8_t* input, ucs4_t delimiter)
{
    ucs4_t uch;
    size_t ch_len = 0;
    size_t result = 1;
    const uint8_t* pinput = NULL;
    size_t input_len = 0;

    if (!input)
        return result;

    input_len = u8_strlen(input);
    pinput = input;

    while (pinput && *pinput)
    {
        ch_len = u8_mbtouc(&uch, pinput, input_len);
        if (uch == delimiter)
            result++;
        pinput += ch_len;
    }

    return result;
}

// | A | B | C |
//  ^^^-^^^-^^^----This
size_t
get_max_table_column_width(size_t rune_columns, size_t table_columns)
{
    if (rune_columns < table_columns+1)
        return MIN_TABLE_COL_WIDTH;

    size_t result = (rune_columns-table_columns-1)/table_columns;

    return result < MIN_TABLE_COL_WIDTH
           ? MIN_TABLE_COL_WIDTH
           : result;
}

int
main(int argc, char** argv)
{
    char* arg;
    Command cmd = CMD_NONE;
    char* filename = NULL;
    int current_symbol_set = TABLE_SYMBOLS_DOUBLE;
    int current_inner_symbol_set = TABLE_INNER_DOUBLE_SINGLE;
    size_t table_columns = 0;
    size_t max_table_column_width = MIN_TABLE_COL_WIDTH;
    size_t rune_columns = 80;
    size_t tab_length = 8;
    ucs4_t delimiter = ',';
    BOOL handle_ansi = TRUE;
    BOOL border_mode = FALSE;
    BOOL expand_tabs = FALSE;

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
                    cmd = CMD_VERSION;
                else if (startswith(arg, "border-mode"))
                {
                    arg += strlen("border-mode");
                    border_mode = TRUE;
                }
                else if (startswith(arg, "columns="))
                {
                    arg += strlen("columns=");
                    result = set_columns(arg, &rune_columns);
                    if (result)
                        return result;
                }
                else if (startswith(arg, "delimiter="))
                {
                    arg += strlen("delimiter=");
                    result = set_delimiter((uint8_t*)arg, &delimiter);
                    if (result)
                        return result;
                }
                else if (startswith(arg, "expand-tabs"))
                {
                    arg += strlen("expand-tabs");
                    expand_tabs = TRUE;
                }
                else if (startswith(arg, "no-ansi"))
                {
                    arg += strlen("no-ansi");
                    handle_ansi = FALSE;
                }
                else if (startswith(arg, "symbols="))
                {
                    arg += strlen("symbols=");
                    result = set_symbol_set(arg, &current_symbol_set,
                                            &current_inner_symbol_set);
                    if (result)
                        return result;
                }
                else if (!strcmp(arg, "help"))
                    return usage();
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
                case 'b':
                    border_mode = TRUE;
                    break;
                case 'c':
                    cmd = CMD_COLUMNS;
                    break;
                case 'd':
                    cmd = CMD_DELIMITER;
                    break;
                case 'h':
                    return usage();
                    break;
                case 'n':
                    handle_ansi = FALSE;
                    break;
                case 's':
                    cmd = CMD_SYMBOLS;
                    break;
                case 't':
                    expand_tabs = TRUE;
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
                result = set_symbol_set(arg, &current_symbol_set,
                                        &current_inner_symbol_set);
                if (result)
                    return result;
            }
            else if (cmd == CMD_COLUMNS)
            {
                result = set_columns(arg, &rune_columns);
                if (result)
                    return result;
            }
            else if (cmd == CMD_DELIMITER)
            {
                result = set_delimiter((uint8_t*)arg, &delimiter);
                if (result)
                    return result;
            }
            else
                filename = arg;
            cmd = CMD_NONE;
        }
    }

    if (cmd == CMD_VERSION)
        return version();

    FILE* input = NULL;
    if (filename)
    {
        input = fopen(filename, "r");
        if (!input)
            return error(ENOENT, (uint8_t*)"File not found: %s\n", filename);
    }
    else
        input = stdin;

    uint8_t* line = NULL;
    CALLOC(line, uint8_t, BUFSIZE)
    size_t pline_len = 0;
    uint8_t* pline = NULL;
    BOOL quote = FALSE;
    size_t colno = 0;
    size_t lineno = 0;
    ucs4_t uch;
    size_t ch_len = 0;
    size_t current_table_column = 0;
    size_t current_rune_column = 0;

    while (!feof(input))
    {
        char* eol = NULL;
        if (!fgets((char*)line, sizeof(uint8_t) * BUFSIZE-1, input))
            break;

        eol = strchr((char*)line, '\n');
        if (eol)
            *eol = 0;

        if (!*line)
            continue;

        quote = FALSE;
        pline = line;
        colno = 0;
        current_rune_column = 0;

        if (lineno == 0)
        {
            if (!border_mode)
                table_columns = number_of_columns(line, delimiter);
            else
                table_columns = 1;
            max_table_column_width = get_max_table_column_width(rune_columns,
                    table_columns);
            if (rune_columns < table_columns * MIN_TABLE_COL_WIDTH
                    + table_columns + 1)
                rune_columns = table_columns * MIN_TABLE_COL_WIDTH 
                    + table_columns + 1;

            current_table_column = 0;
            for (size_t col = 0; col < rune_columns; col++)
            {
                if (col == 0)
                    printf("%s", table_symbols[current_symbol_set][0]);
                else if (col == rune_columns-1)
                    printf("%s", table_symbols[current_symbol_set][2]);
                else if (current_table_column < table_columns-1
                        && col > 1
                        && col % (max_table_column_width+1) == 0)
                {
                    current_table_column++;
                    printf("%s", table_inner_symbols[current_inner_symbol_set][0]);
                }
                else
                    printf("%s", table_symbols[current_symbol_set][1]);
            }
            printf("\n");
        }

        current_table_column = 0;

        printf("%s", table_symbols[current_symbol_set][3]);

        if (handle_ansi && lineno == 0)
            printf("%s", ANSI_SGR_BOLD_ON);

        while (pline && *pline)
        {
            if (*pline == '"')
            {
                quote = !quote;
                pline++;
                colno++;
            }
            else
            {
                uint8_t* pch_end;
                pline_len = u8_strlen(line);
                ch_len = u8_mbtouc(&uch, pline, pline_len);
                if (border_mode)
                {
                    pch_end = pline + ch_len;
                    if (WITHIN_SINGLE_COLUMN)
                    {
                        while (pline != pch_end)
                            printf("%c", *pline++);
                        current_rune_column++;
                    }
                    pline = pch_end;
                    colno += ch_len;
                }
                else if (ch_len <= 0)
                {
                    if (WITHIN_COLUMN)
                    {
                        printf("%c", *pline++);
                        current_rune_column++;
                    }
                    colno++;
                }
                else if (uch == '\t' && expand_tabs)
                {
                    while (WITHIN_COLUMN)
                    {
                        printf(" ");
                        current_rune_column++;
                        if (current_rune_column % tab_length == 0)
                            break;
                    }
                    pline++;
                    colno++;
                }
                else if (uch == delimiter && !quote)
                {
                    if (handle_ansi && lineno == 0)
                        printf("%s", ANSI_SGR_BOLD_OFF);

                    while (WITHIN_COLUMN)
                    {
                        printf(" ");
                        current_rune_column++;
                    }

                    if (current_table_column != table_columns-1)
                    {
                        printf("%s", table_inner_symbols[current_inner_symbol_set][1]);
                        current_rune_column++;
                        current_table_column++;
                    }

                    if (handle_ansi && lineno == 0)
                        printf("%s", ANSI_SGR_BOLD_ON);

                    pline += ch_len;
                    colno += ch_len;
                }
                else
                {
                    pch_end = pline + ch_len;
                    if (WITHIN_COLUMN)
                    {
                        while (pline != pch_end)
                            printf("%c", *pline++);
                        current_rune_column++;
                    }
                    pline = pch_end;
                    colno += ch_len;
                }
            }
        }

        if (handle_ansi && lineno == 0)
            printf("%s", ANSI_SGR_BOLD_OFF);

        while (current_table_column < table_columns-1)
        {
            while (WITHIN_COLUMN)
            {
                printf(" ");
                current_rune_column++;
            }
            printf("%s", table_inner_symbols[current_inner_symbol_set][1]);
            current_table_column++;
            current_rune_column++;
        }
        while (current_rune_column < rune_columns-2)
        {
            printf(" ");
            current_rune_column++;
        }

        printf("%s\n", table_symbols[current_symbol_set][5]);

        lineno++;
    }

    fclose(input);

    free(line);

    current_table_column = 0;
    for (size_t col = 0; col < rune_columns; col++)
    {
        if (col == 0)
            printf("%s", table_symbols[current_symbol_set][6]);
        else if (col == rune_columns-1)
            printf("%s", table_symbols[current_symbol_set][8]);
        else if (current_table_column < table_columns-1
                && col > 1
                && col % (max_table_column_width+1) == 0)
        {
            current_table_column++;
            printf("%s", table_inner_symbols[current_inner_symbol_set][2]);
        }
        else
            printf("%s", table_symbols[current_symbol_set][7]);
    }
    printf("\n");

    return 0;
}

