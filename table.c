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
                (uint8_t*)"Memory allocation failed (out of memory?)")); }

#define CALLOC(ptr, ptrtype, nmemb) { ptr = calloc(nmemb, sizeof(ptrtype)); \
    CHECKEXITNOMEM(ptr) }

#define REALLOC(ptr, ptrtype, newsize) { ptrtype* newptr = realloc(ptr, newsize); \
    CHECKEXITNOMEM(newptr) \
    ptr = newptr; }

#define REALLOCARRAY(ptr, membtype, newcount) \
    REALLOC(ptr, membtype, sizeof(membtype) * newcount)

#define CHECKCOPY(token, ptoken, token_size, parg) { \
    if (ptoken + 2 > token + token_size) \
    { \
        size_t old_size = token_size; \
        token_size += BUFSIZE; \
        REALLOC(token, char, token_size) \
        ptoken = token + old_size - 1; \
    } \
    *ptoken++ = *parg++; }

#define CHECKSET(format, pformat, format_size, num) { \
    if (pformat + 2 > format + format_size) \
    { \
        size_t old_size = format_size; \
        format_size += BUFSIZE; \
        REALLOC(format, ULONG, format_size) \
        pformat = format + old_size - 1; \
    } \
    *pformat++ = num; }

size_t colno                  = 0;
size_t lineno                 = 0;
int current_symbol_set        = TABLE_SYMBOLS_DOUBLE;
int current_inner_symbol_set  = TABLE_INNER_DOUBLE_SINGLE;
size_t current_table_column   = 0;
size_t current_rune_column    = 0;
size_t table_columns          = 0;
size_t rune_columns           = 80;
size_t tab_length             = 8;
ucs4_t delimiter              = ',';
ULONG* format                 = NULL;
size_t format_size            = 0;
BOOL handle_ansi              = TRUE;
BOOL border_mode              = FALSE;
BOOL expand_tabs              = FALSE;

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
            " <delim>|--delimiter=<delim>] [-f <format>|--format=<format>]"
            " [-h|--help] [-n|--no-ansi] [-s <set>|--symbols=<set>] "
            "[-t|--expand-tabs] [-v|--version]\n",
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
    fprintf(stderr, "%s: %s\n", PROGRAMNAME, buf);
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
        return error(1, (uint8_t*)"Invalid symbol set: %s", arg);
    }
    return 0;
}

int
set_columns(const char* arg, size_t* cols)
{
    size_t c = strtol(arg, NULL, 10);
    if (errno == EINVAL || errno == ERANGE)
        return error(1, (uint8_t*)"Invalid numeric value: %s", arg);
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

int
set_format(const char* arg, ULONG** format, size_t* format_size)
{
    const char* parg = arg;
    ULONG* pformat = NULL;
    char* token = NULL;
    size_t token_size = BUFSIZE;
    char* ptoken = NULL;
    ULONG num;

    CALLOC(token, char, token_size)
    ptoken = token;
    *format_size = SMALL_BUFSIZE;
    CALLOC(*format, ULONG, *format_size)
    pformat = *format;

    while (*parg)
    {
        if (!((*parg >= '0' && *parg <= '9') || *parg == ':'))
        {
            free(token);
            free(*format);
            *format = NULL;
            return 1;
        }
        if (*parg == ':')
        {
            if (parg == arg || !*(parg+1))
            {
                free(token);
                free(*format);
                *format = NULL;
                return 2;
            }
            *ptoken = 0;
            num = strtol(token, NULL, 10);
            if (errno)
            {
                perror("strtol");
                free(token);
                free(*format);
                *format = NULL;
                return 3;
            }
            CHECKSET(*format, pformat, *format_size, num)
            *token = 0;
            ptoken = token;
            parg++;
        }
        else
            CHECKCOPY(token, ptoken, token_size, parg)
    }
    if (*token)
    {
        *ptoken = 0;
        num = strtol(token, NULL, 10);
        if (errno)
        {
            perror("strtol");
            free(token);
            free(*format);
            *format = NULL;
            return 3;
        }
        CHECKSET(*format, pformat, *format_size, num)
    }

    free(token);

    return 0;
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

UINT round_div(UINT a, UINT b)
{
    return (a + (b/2)) / b;
}

BOOL
within_column(size_t column_start)
{
    return (current_rune_column <
            column_start + *(format+current_table_column));
}

int
main(int argc, char** argv)
{
    char* arg;
    Command cmd      = CMD_NONE;
    char* filename   = NULL;

    while ((arg = *++argv))
    {
        if (*arg == '-')
        {
            arg++;
            char c = *arg++;
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
                    if (set_columns(arg, &rune_columns))
                        return error(EINVAL, (uint8_t*)"Invalid argument: '%s'",
                                arg);
                }
                else if (startswith(arg, "delimiter="))
                {
                    arg += strlen("delimiter=");
                    if (set_delimiter((uint8_t*)arg, &delimiter))
                        return error(EINVAL, (uint8_t*)"Invalid argument: '%s'",
                                arg);
                }
                else if (startswith(arg, "expand-tabs"))
                {
                    arg += strlen("expand-tabs");
                    expand_tabs = TRUE;
                }
                else if (startswith(arg, "format="))
                {
                    arg += strlen("format=");
                    set_format(arg, &format, &format_size);
                }
                else if (startswith(arg, "no-ansi"))
                {
                    arg += strlen("no-ansi");
                    handle_ansi = FALSE;
                }
                else if (startswith(arg, "symbols="))
                {
                    arg += strlen("symbols=");
                    if (set_symbol_set(arg, &current_symbol_set,
                                &current_inner_symbol_set))
                        return error(EINVAL, (uint8_t*)"Invalid argument: '%s'",
                                arg);
                }
                else if (!strcmp(arg, "help"))
                    return usage();
                else
                {
                    error(EINVAL, (uint8_t*)"Invalid argument: --%s", arg);
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
                case 'f':
                    cmd = CMD_FORMAT;
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
                    error(EINVAL, (uint8_t*)"Invalid argument: -%c", c);
                    return usage();
                }
            }
        }
        else
        {
            if (cmd == CMD_COLUMNS)
            {
                if (set_columns(arg, &rune_columns))
                    return error(EINVAL, (uint8_t*)"Invalid argument: '%s'", arg);
            }
            else if (cmd == CMD_DELIMITER)
            {
                if (set_delimiter((uint8_t*)arg, &delimiter))
                    return error(EINVAL, (uint8_t*)"Invalid argument: '%s'", arg);
            }
            else if (cmd == CMD_FORMAT)
            {
                if (set_format(arg, &format, &format_size))
                    return error(EINVAL, (uint8_t*)"Invalid argument: '%s'", arg);
            }
            else if (cmd == CMD_SYMBOLS)
            {
                if (set_symbol_set(arg, &current_symbol_set,
                            &current_inner_symbol_set))
                    return error(EINVAL, (uint8_t*)"Invalid argument: '%s'", arg);
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
            return error(ENOENT, (uint8_t*)"File not found: %s", filename);
    }
    else
        input = stdin;

    uint8_t* line                  = NULL;
    size_t pline_len               = 0;
    uint8_t* pline                 = NULL;
    BOOL quote                     = FALSE;
    ucs4_t uch;
    size_t ch_len                  = 0;
    size_t output_lines            = 0;
    size_t column_start            = 0;

    CALLOC(line, uint8_t, BUFSIZE)

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

        /* Top border */
        if (lineno == 0)
        {
            if (!border_mode)
                table_columns = number_of_columns(line, delimiter);
            else
                table_columns = 1;

            /* Sanity check */
            if (rune_columns < table_columns+1)
                rune_columns = table_columns+1;

            if (format && !border_mode)
            {
                ULONG* pformat = format;
                ULONG format_sum = 0;
                while (*pformat)
                    format_sum += *pformat++;
                pformat = format;
                while (*pformat && format_sum)
                {
                    *pformat = round_div((rune_columns-table_columns-2) 
                            * (*pformat), format_sum);
                    pformat++;
                }
            }
            else
            {
                ULONG* pformat = NULL;
                format_size = SMALL_BUFSIZE;
                CALLOC(format, ULONG, format_size)
                pformat = format;
                for (size_t col = 0; col < table_columns; col++)
                    *pformat++ = (rune_columns-table_columns-2) / table_columns;
            }

            current_table_column = 0;
            column_start = 0;
            while (current_table_column < table_columns)
            {
                if (current_rune_column == 0)
                {
                    printf("%s", table_symbols[current_symbol_set][0]);
                    column_start++;
                }
                else if (!within_column(column_start))
                {
                    if (current_table_column == table_columns-1)
                    {
                        printf("%s", table_symbols[current_symbol_set][2]);
                        current_table_column++;
                    }
                    else
                    {
                        printf("%s", table_inner_symbols[current_inner_symbol_set][0]);
                        column_start++;
                        if (format)
                            column_start += *(format+current_table_column);
                        current_table_column++;
                    }
                }
                else
                    printf("%s", table_symbols[current_symbol_set][1]);
                current_rune_column++;
            }
            printf("\n");
            output_lines++;
        }

        /* Inner rows */
        current_table_column = 0;
        current_rune_column = 0;
        column_start = 0;

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
                    if (within_column(column_start))
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
                    if (within_column(column_start))
                    {
                        printf("%c", *pline++);
                        current_rune_column++;
                    }
                    colno++;
                }
                else if (uch == '\t' && expand_tabs)
                {
                    while (within_column(column_start))
                    {
                        printf(" ");
                        current_rune_column++;
                        if (current_rune_column % tab_length == 0)
                            break;
                    }
                    pline++;
                    colno++;
                }
                else if (uch == delimiter && !quote 
                        && current_table_column < table_columns-1)
                {
                    if (handle_ansi && lineno == 0)
                        printf("%s", ANSI_SGR_BOLD_OFF);

                    while (within_column(column_start))
                    {
                        printf(" ");
                        current_rune_column++;
                    }
                    column_start++;
                    if (format)
                        column_start += *(format+current_table_column);
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
                    if (within_column(column_start))
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
            while (within_column(column_start))
            {
                printf(" ");
                current_rune_column++;
            }
            printf("%s", table_inner_symbols[current_inner_symbol_set][1]);
            column_start++;
            column_start += *(format+current_table_column);
            current_table_column++;
            current_rune_column++;
        }
        while (within_column(column_start))
        {
            printf(" ");
            current_rune_column++;
        }

        printf("%s\n", table_symbols[current_symbol_set][5]);

        output_lines++;
        lineno++;
    }

    fclose(input);
    free(line);

    /* Bottom border */
    if (output_lines)
    {
        current_table_column = 0;
        current_rune_column = 0;
        column_start = 0;
        while (current_table_column < table_columns)
        {
            if (current_rune_column == 0)
            {
                printf("%s", table_symbols[current_symbol_set][6]);
                column_start++;
            }
            else if (!within_column(column_start))
            {
                if (current_table_column == table_columns-1)
                {
                    printf("%s", table_symbols[current_symbol_set][8]);
                    current_table_column++;
                }
                else
                {
                    printf("%s", table_inner_symbols[current_inner_symbol_set][2]);
                    column_start++;
                    if (format)
                        column_start += *(format+current_table_column);
                    current_table_column++;
                }
            }
            else
                printf("%s", table_symbols[current_symbol_set][7]);
            current_rune_column++;
        }
        printf("\n");
        output_lines++;
    }

    free(format);

    return 0;
}

