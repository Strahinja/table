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
    printf("        -c <cols>\n");
    printf("        --columns=<cols>\n");
    printf("                    Set maximum table width in columns (default 80)\n");
    printf("\n");
    printf("        -h\n");
    printf("        --help\n");
    printf("                    Prints this usage information screen\n");
    printf("\n");
    printf("        -s <set>\n");
    printf("        --symbols=<set>\n");
    printf("                    Use table symbol set <set>, where <set> is one\n");
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

uint32_t*
u32_substr(uint32_t* src, size_t src_len, size_t start, size_t finish,
           uint32_t* dest, size_t* dest_len)
{
    size_t normalized_finish = src_len;
    if (finish < normalized_finish)
        normalized_finish = finish;
    size_t normalized_start = 0;
    if (start >= normalized_start)
        normalized_start = start;
    if (normalized_start > normalized_finish)
        normalized_start = normalized_finish;
    size_t len = normalized_finish - normalized_start;

    dest = u32_cpy(dest, src + normalized_start, len);
    *dest_len = len;
    return dest;
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
set_columns(char* arg, size_t* cols)
{
    size_t c = strtol(arg, NULL, 10);
    if (errno == EINVAL || errno == ERANGE)
    {
        return error(1, (uint8_t*)"Invalid numeric value: %s\n", arg);
    }
    else
    {
        *cols = c;
    }
    return 0;
}

size_t
number_of_columns(const uint32_t* input, const size_t input_len,
                  uint32_t delimiter)
{
    size_t result = 1;

    for (size_t i = 0; i < input_len; i++)
    {
        if (input[i] == delimiter)
        {
            result++;
        }
    }
    return result;
}

uint32_t*
u32_tabs_to_spaces(const uint32_t* src, const size_t src_len, uint32_t* dest,
                   size_t* dest_len, size_t tab_length)
{
    size_t rune_column = 0;

    for (size_t i = 0; i < src_len; i++)
    {
        if (src[i] == (uint32_t)'\t')
        {
            do
            {
                dest[rune_column++] = (uint32_t)' ';
            }
            while (rune_column % tab_length != 0);
        }
        else
            dest[rune_column++] = src[i];
    }
    *dest_len = rune_column;
    return dest;
}

size_t
get_max_table_column_width(size_t rune_columns, size_t table_columns)
{
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
    size_t table_columns = 0;
    size_t max_table_column_width = MIN_TABLE_COL_WIDTH;
    size_t rune_columns = 80;
    size_t tab_length = 8;
    uint32_t delimiter = ',';

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
                    result = set_columns(arg, &rune_columns);
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
                result = set_columns(arg, &rune_columns);
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
    while (col++ < rune_columns-2)
        fprintf(stdout, "%s", table_symbols[current_symbol_set][1]);
    fprintf(stdout, "%s\n", table_symbols[current_symbol_set][2]);

    uint8_t* line = NULL;
    size_t line_len = 0;
    uint8_t* pline = NULL;

    uint32_t* unicode_line = NULL;
    size_t unicode_line_len = 0;
    uint32_t* punicode_line = NULL;

    uint8_t* buffer = NULL;
    size_t buffer_len = 0;
    uint8_t* pbuffer = NULL;

    uint32_t* unicode_buffer = NULL;
    size_t unicode_buffer_len = 0;
    uint32_t* punicode_buffer = NULL;

    do
    {
        if (line)
            line = (uint8_t*) realloc(line, sizeof(uint8_t) * BUFSIZE);
        else
            line = (uint8_t*) malloc(sizeof(uint8_t) * BUFSIZE);

        fgets(line, sizeof(uint8_t) * BUFSIZE, input);
        if (!feof(input))
        {
            uint8_t* eol = u8_strchr(line, (ucs4_t)'\n');
            if (eol)
                *eol = (uint8_t)'\0';

            if (unicode_line)
                unicode_line = (uint32_t*) realloc(unicode_line, sizeof(uint32_t) * BUFSIZE);
            else
                unicode_line = (uint32_t*) malloc(sizeof(uint32_t) * BUFSIZE);
            unicode_line_len = sizeof(uint32_t) * BUFSIZE;

            if (buffer)
                buffer = (uint8_t*) realloc(buffer, sizeof(uint8_t) * BUFSIZE);
            else
                buffer = (uint8_t*) malloc(sizeof(uint8_t) * BUFSIZE);
            buffer_len = sizeof(uint8_t) * BUFSIZE;

            if (unicode_buffer)
                unicode_buffer = (uint32_t*) realloc(unicode_buffer, sizeof(uint32_t) * BUFSIZE);
            else
                unicode_buffer = (uint32_t*) malloc(sizeof(uint32_t) * BUFSIZE);
            unicode_buffer_len = sizeof(uint32_t) * BUFSIZE;

            punicode_line = u8_to_u32(line, u8_strlen(line),
                                      unicode_line, &unicode_line_len);

            if (!table_columns)
            {
                table_columns = number_of_columns(punicode_line,
                                                  unicode_line_len, delimiter);
                max_table_column_width =
                    get_max_table_column_width(rune_columns, table_columns);
            }

            fprintf(stdout, "%s", table_symbols[current_symbol_set][3]);

            punicode_buffer = u32_tabs_to_spaces(punicode_line,
                                                 unicode_line_len,
                                                 unicode_buffer,
                                                 &unicode_buffer_len,
                                                 tab_length);

            punicode_line = u32_strcpy(unicode_line, unicode_buffer);

            // TODO: Make the print column-wise
            /*
             *            for (size_t rune_column = 0;
             *                    rune_column < unicode_line_len;
             *                    rune_column += max_table_column_width)
             *            {
             *                punicode_buffer = u32_substr(punicode_line,
             *                                             unicode_line_len,
             *                                             rune_column,
             *                                             rune_column +,
             *                                             unicode_buffer,
             *                                             &unicode_buffer_len);
             *                pbuffer = u32_to_u8(punicode_buffer, max_table_column_width,
             *                                    buffer, &buffer_len);
             *
             *                fprintf(stdout, "%s%s",
             *                        table_symbols[current_symbol_set][3],
             *                        pbuffer);
             *
             *                if (pbuffer)
             *                {
             *                    pbuffer[buffer_len] = (uint8_t)'\0';
             *                    fprintf(stdout, "%s", pbuffer);
             *                }
             *
             *                for (int i = 0; i < rune_columns-2-u8_width(buffer, buffer_len, "utf-8"); i++)
             *                {
             *                    fprintf(stdout, " ");
             *                }
             *
             *                fprintf(stdout, "%s\n", table_symbols[current_symbol_set][5]);
             *
             *            }
             */

            /*
             *            unicode_cropped_line = (uint32_t*) malloc(sizeof(uint32_t) * BUFSIZE);
             *            unicode_cropped_line_len = sizeof(sizeof(uint32_t) * BUFSIZE);
             *            punicode_cropped_line = u32_substr(pexpanded_line, expanded_line_len,
             *                                               0, rune_columns-2,
             *                                               unicode_cropped_line, &unicode_cropped_line_len);
             *
             *            if (cropped_line)
             *                cropped_line = (uint8_t*) realloc(cropped_line, sizeof(uint8_t) * BUFSIZE);
             *            else
             *                cropped_line = (uint8_t*) malloc(sizeof(uint8_t) * BUFSIZE);
             *            cropped_line_len = sizeof(uint8_t) * BUFSIZE;
             *            pcropped_line = u32_to_u8(punicode_cropped_line, unicode_cropped_line_len,
             *                                      cropped_line, &cropped_line_len);
             *
             *            if (pcropped_line)
             *            {
             *                pcropped_line[cropped_line_len] = (uint8_t)'\0';
             *                fprintf(stdout, "%s", pcropped_line);
             *            }
             *
             *            for (int i = 0; i < rune_columns-2-u8_width(cropped_line, cropped_line_len, "utf-8"); i++)
             *            {
             *                fprintf(stdout, " ");
             *            }
             *
             *            fprintf(stdout, "%s\n", table_symbols[current_symbol_set][5]);
             */
        }
    }
    while (!feof(input));

    fprintf(stdout, "%s", table_symbols[current_symbol_set][6]);
    col = 0;
    while (col++ < rune_columns-2)
        fprintf(stdout, "%s", table_symbols[current_symbol_set][7]);
    fprintf(stdout, "%s\n", table_symbols[current_symbol_set][8]);

    return 0;
}

