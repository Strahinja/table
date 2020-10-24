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
    printf("        -a\n");
    printf("        --ansi\n");
    printf("                    ANSI SGR processing in the input: add\n");
    printf("                        \\e[0m\\e[?25h\n");
    printf("                    to the end of each cell\n");
    printf("\n");
    printf("        -b\n");
    printf("        --border-mode\n");
    printf("                    Border mode: force single column\n");
    printf("\n");
    printf("        -c <cols>\n");
    printf("        --columns=<cols>\n");
    printf("                    Set maximum table width in columns (default 80)\n");
    printf("\n");
    printf("        -d <delim>\n");
    printf("        --delimiter=<delim>\n");
    printf("                    Set field delimiter (default \",\"). First line\n");
    printf("                    of the input determines the number of table\n");
    printf("                    columns. Special characters, like \";\", need\n");
    printf("                    to be quoted:\n");
    printf("                        $ table -d \";\"\n");
    printf("\n");
    printf("        -h\n");
    printf("        --help\n");
    printf("                    Print this usage information screen\n");
    printf("\n");
    printf("        -s <set>\n");
    printf("        --symbols=<set>\n");
    printf("                    Use table symbol set <set> for table lines,\n");
    printf("                    where <set> is one of the following\n");
    printf("                    (format: <border><inner border>):\n");
    printf("                        aa = ascii-ascii\n");
    printf("                        ss = single-single\n");
    printf("                        sd = single-double\n");
    printf("                        ds = double-single (default)\n");
    printf("                        dd = double-double\n");
    printf("\n");
    printf("        -v\n");
    printf("        --version\n");
    printf("                    Print program version and exit\n");
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

void
u32_print(char* name, uint32_t* src, size_t src_len);

/*
   A  B  C  0
   0  1  2  3
  len = 3 --^
  start = 1 (B)
  finish = len = 3 (0)
  substr_len = finish-start = 2 (BC)
  result = BC0
*/

char*
substr(char* src, int start, int finish)
{
    int len = strlen(src);
    if (finish > len)
        finish = len;
    int substr_len = finish-start;
    if (substr_len < 0)
        substr_len = 0;
    char* result = (char*) calloc(substr_len+1, sizeof(char));
    char* presult = result;

    for (int i = start; i < finish && *(src+i) != '\0'; i++)
    {
        *presult++ = *(src+i);
    }
    *presult = '\0';

    return result;
}

/*
      start = 1
      |     finish = 3
      v     v
   A  B  C  D  0
      ====
   0  1  2  3  4
  src_len = 4
  normalized_start = 1 (B)
  normalized_finish = 3 (0)
  len = normalized_finish-normalized_start = 2 (BC)
  dest = BC0
*/
uint32_t*
u32_substr(uint32_t* src, size_t src_len, size_t start, size_t finish,
           size_t* dest_len)
{
    size_t normalized_finish = src_len;
    uint32_t* dest = NULL;
    if (finish < normalized_finish)
        normalized_finish = finish;
    if (normalized_finish < 0)
        normalized_finish = 0;
    size_t normalized_start = 0;
    if (start > normalized_start)
        normalized_start = start;
    if (normalized_start > normalized_finish)
        normalized_start = normalized_finish;
    size_t len = normalized_finish - normalized_start;

    dest = (uint32_t*) calloc(len+1, sizeof(uint32_t));

    /*
     *    if (handle_ansi)
     *    {
     *        uint32_t *psrc = src + normalized_start;
     *        size_t i = 0;
     *        size_t chars_in_sgr = 0;
     *        size_t current_sgr_len = 0;
     *        BOOL in_sgr = FALSE;
     *
     *        while (*psrc && i < len)
     *        {
     *            if (*psrc == (uint32_t)'\e'
     *                    && i+1 < len
     *                    && *(psrc+1) == (uint32_t)'[')
     *            {
     *                in_sgr = TRUE;
     *                current_sgr_len = 0;
     *                psrc++;
     *                i++;
     *            }
     *            else if (in_sgr && (
     *                         *psrc == (uint32_t)'0'
     *                         || *psrc == (uint32_t)'0'
     *                         || *psrc == (uint32_t)'1'
     *                         || *psrc == (uint32_t)'2'
     *                         || *psrc == (uint32_t)'3'
     *                         || *psrc == (uint32_t)'4'
     *                         || *psrc == (uint32_t)'5'
     *                         || *psrc == (uint32_t)'6'
     *                         || *psrc == (uint32_t)'7'
     *                         || *psrc == (uint32_t)'8'
     *                         || *psrc == (uint32_t)'9'
     *                         || *psrc == (uint32_t)';'))
     *            {
     *                current_sgr_len++;
     *                chars_in_sgr++;
     *            }
     *            else if (in_sgr && current_sgr_len > 0 && *psrc == (uint32_t)'m')
     *            {
     *                in_sgr = FALSE;
     *                chars_in_sgr++;
     *            }
     *            else if (in_sgr)
     *            {
     *                in_sgr = FALSE;
     *                chars_in_sgr -= current_sgr_len;
     *                current_sgr_len = 0;
     *            }
     *            else
     *            {
     *                *dest++ = *psrc;
     *                i++;
     *                psrc++;
     *            }
     *        }
     *    }
     *    else
     */
    memcpy(dest, src + normalized_start, sizeof(uint32_t) * len);

    dest[len] = (uint32_t)'\0';
    *dest_len = len;

    return dest;
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

int
set_delimiter(char* arg, uint32_t* delimiter)
{
    size_t start = 0;
    size_t len = strlen(arg);
    size_t finish = len;

    if (arg[0] == '"')
        start = 1;
    if (arg[len-1] == '"')
        finish = len-2;

    char* stripped_arg = substr(arg, start, finish);
    uint32_t* delim = (uint32_t*) calloc(BUFSIZE, sizeof(uint32_t));
    size_t delim_len = BUFSIZE;
    u8_to_u32(stripped_arg, len, delim, &delim_len);
    *delimiter = delim[0];
    free(delim);

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
u32_tabs_to_spaces(const uint32_t* src, const size_t src_len,
                   size_t* dest_len, size_t tab_length)
{
    size_t rune_column = 0;
    uint32_t* dest = (uint32_t*) calloc(BUFSIZE, sizeof(uint32_t));

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
    dest[*dest_len] = (uint32_t)'\0';
    return dest;
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

size_t
get_max_last_table_column_width(size_t rune_columns, size_t table_columns)
{
    size_t result = get_max_table_column_width(rune_columns, table_columns);
    if (rune_columns > (table_columns-1) * (result+1) + 2)
        return rune_columns - (table_columns-1) * (result+1) - 2;
    return MIN_TABLE_COL_WIDTH;
}

void
u32_print(char* name, uint32_t* src, size_t src_len)
{
    fprintf(stdout, "%s [%lu] = \"", name, src_len);
    for (size_t k = 0; k < src_len; k++)
    {
        uint8_t charbuf[7];
        size_t charbuf_len = 7;
        u32_to_u8(src+k, 1, charbuf, &charbuf_len);
        charbuf[charbuf_len] = (uint8_t)'\0';
        fprintf(stdout, "%X='%s' ", src[k], charbuf);
    }
    fprintf(stdout, "\"\n");
}

/* TODO: Column alignment?
 *void
 *write_column(uint32_t* src, size_t src_len, size_t column_width,
 *             size_t written_columns, size_t rune_columns,
 *             uint8_t* start_symbol, uint8_t* finish_symbol)
 *{
 *    uint8_t* buffer = NULL;
 *    size_t buffer_len = BUFSIZE;
 *
 *    fprintf(stdout, "%s", start_symbol);
 *
 *    buffer = (uint8_t*) calloc(BUFSIZE, sizeof(uint8_t));
 *    u32_to_u8(src, src_len, buffer, &buffer_len);
 *    fprintf(stdout, "%s", buffer);
 *
 *    size_t end_column;
 *    if (rune_columns - written_columns > 2 * column_width)
 *        end_column = written_columns + column_width + 1;
 *    else
 *        end_column = rune_columns - 1;
 *
 *    for (size_t column = written_columns; column < end_column; column++)
 *        fprintf(stdout, " ");
 *
 *    fprintf(stdout, "%s", finish_symbol);
 *}
 */

// Returns the number of uint32_t characters that make up ANSI SGR codes and
// thus don't count towards the string rune column length
size_t
ansi_lookahead(uint32_t* src, size_t src_len)
{
    size_t i = 0;
    size_t chars_in_sgr = 0;
    size_t current_sgr_len = 0;
    BOOL in_sgr = FALSE;

    while (i < src_len)
    {
        if (src[i] == (uint32_t)'\e'
                && i+1 < src_len
                && src[i+1] == (uint32_t)'[')
        {
            in_sgr = TRUE;
            current_sgr_len = 0;
            i++;
        }
        else if (in_sgr && (src[i] == (uint32_t)'0'
                            || src[i] == (uint32_t)'1'
                            || src[i] == (uint32_t)'2'
                            || src[i] == (uint32_t)'3'
                            || src[i] == (uint32_t)'4'
                            || src[i] == (uint32_t)'5'
                            || src[i] == (uint32_t)'6'
                            || src[i] == (uint32_t)'7'
                            || src[i] == (uint32_t)'8'
                            || src[i] == (uint32_t)'9'
                            || src[i] == (uint32_t)';'))
        {
            current_sgr_len++;
            chars_in_sgr++;
        }
        else if (in_sgr && current_sgr_len > 0 && src[i] == (uint32_t)'m')
        {
            in_sgr = FALSE;
            chars_in_sgr++;
        }
        else if (in_sgr)
        {
            // Only count full SGRs, \e[ n [; n...] m
            in_sgr = FALSE;
            chars_in_sgr -= current_sgr_len;
            current_sgr_len = 0;
        }
        //else...
        i++;
    }
    return chars_in_sgr;
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
    size_t max_last_table_column_width = MIN_TABLE_COL_WIDTH;
    size_t rune_columns = 80;
    size_t tab_length = 8;
    uint32_t delimiter = ',';
    BOOL handle_ansi = FALSE;
    BOOL border_mode = FALSE;

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
                else if (!strcmp(substr(arg, 0, strlen("ansi")), "ansi"))
                {
                    arg += strlen("ansi");
                    handle_ansi = TRUE;
                }
                else if (!strcmp(substr(arg, 0, strlen("border-mode")),
                                 "border-mode"))
                {
                    arg += strlen("border-mode");
                    border_mode = TRUE;
                }
                else if (!strcmp(substr(arg, 0, strlen("columns=")), "columns="))
                {
                    arg += strlen("columns=");
                    result = set_columns(arg, &rune_columns);
                    if (result)
                        return result;
                }
                else if (!strcmp(substr(arg, 0, strlen("delimiter=")), "delimiter="))
                {
                    arg += strlen("delimiter=");
                    result = set_delimiter(arg, &delimiter);
                    if (result)
                        return result;
                }
                else if (!strcmp(substr(arg, 0, strlen("symbols=")), "symbols="))
                {
                    arg += strlen("symbols=");
                    result = set_symbol_set(arg, &current_symbol_set,
                                            &current_inner_symbol_set);
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
                case 'a':
                    handle_ansi = TRUE;
                    break;
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
                result = set_delimiter(arg, &delimiter);
                if (result)
                    return result;
            }
            else
            {
                filename = arg;
            }
            cmd = CMD_NONE;
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

    uint8_t* line = (uint8_t*) calloc(BUFSIZE, sizeof(uint8_t));
    size_t line_len = 0;

    uint32_t* unicode_line = NULL;
    size_t unicode_line_len = 0;

    uint8_t* buffer = NULL;
    size_t buffer_len = 0;

    uint32_t* unicode_buffer = NULL;
    size_t unicode_buffer_len = 0;

    size_t sgr_chars = 0;

    size_t col;

    do
    {
        fgets(line, sizeof(uint8_t) * BUFSIZE, input);
        line_len = u8_strlen(line);

        if (!feof(input))
        {
            uint8_t* eol = u8_strchr(line, (ucs4_t)'\n');
            if (eol)
                *eol = (uint8_t)'\0';

            if (unicode_buffer)
                unicode_buffer = (uint32_t*) realloc(unicode_buffer, sizeof(uint32_t) * BUFSIZE);
            else
                unicode_buffer = (uint32_t*) calloc(BUFSIZE, sizeof(uint32_t));
            unicode_buffer_len = BUFSIZE;

            unicode_line_len = BUFSIZE;
            unicode_line = u8_to_u32(line, line_len,
                                     NULL, &unicode_line_len);
            if (!unicode_line)
            {
                fprintf(stdout, "%s", table_symbols[current_symbol_set][3]);
                for (size_t i = 0; i < max_table_column_width; i++)
                {
                    fprintf(stdout, " ");
                }
                if (handle_ansi)
                    fprintf(stdout, "%s", ANSI_SGR_RESET);
                fprintf(stdout, "%s\n", table_symbols[current_symbol_set][5]);
                continue;
            }

            if (!table_columns)
            {
                // Sanity check
                if (rune_columns < MIN_TABLE_COL_WIDTH + 2)
                    rune_columns = MIN_TABLE_COL_WIDTH + 2;

                table_columns = border_mode
                                ? 1
                                : number_of_columns(unicode_line,
                                                    unicode_line_len, delimiter);
                max_table_column_width =
                    get_max_table_column_width(rune_columns, table_columns);
                max_last_table_column_width =
                    get_max_last_table_column_width(rune_columns, table_columns);

                // Sanity check
                if (rune_columns < (table_columns-1) * max_table_column_width
                        + max_last_table_column_width + table_columns + 1)
                {
                    rune_columns = (table_columns-1) * max_table_column_width
                                   + max_last_table_column_width + table_columns + 1;
                    max_table_column_width =
                        get_max_table_column_width(rune_columns, table_columns);
                    max_last_table_column_width =
                        get_max_last_table_column_width(rune_columns, table_columns);
                }

                fprintf(stdout, "%s", table_symbols[current_symbol_set][0]);
                col = 0;
                // |---|---|---|---|---|
                // 01  4   8  12  16  20
                //
                // max_w = 3
                //         8-1 = 7 % 3 = 1
                //         12-1 = 11 % 3 = 2
                //        8 % (max_w+1) = 0
                //        12 % (max_w+1) = 0
                size_t table_column = 0;
                while (col++ < rune_columns-2)
                    if (table_column < table_columns-1
                            && col % (max_table_column_width+1) == 0)
                    {
                        fprintf(stdout, "%s",
                                table_inner_symbols[current_inner_symbol_set][0]);
                        table_column++;
                    }
                    else
                        fprintf(stdout, "%s", table_symbols[current_symbol_set][1]);
                fprintf(stdout, "%s\n", table_symbols[current_symbol_set][2]);
            }

            if (unicode_buffer)
                free(unicode_buffer);
            unicode_buffer = u32_tabs_to_spaces(unicode_line,
                                                unicode_line_len,
                                                &unicode_buffer_len,
                                                tab_length);

            if (unicode_line)
                free(unicode_line);
            unicode_line = (uint32_t*) calloc(BUFSIZE, sizeof(uint32_t));
            u32_strcpy(unicode_line, unicode_buffer);
            unicode_line_len = unicode_buffer_len;

            size_t substr_start = 0;
            size_t substr_finish = 0;
            size_t rune_column;
            size_t column_width = max_table_column_width;
            size_t written_columns = 0;
            size_t current_table_column = 0;


            for (rune_column = 0;
                    rune_column < unicode_line_len;
                    rune_column++)
            {
                if (table_columns>1
                        && current_table_column < table_columns-1
                        && unicode_line[rune_column] == delimiter)
                {
                    column_width = max_table_column_width;

                    fprintf(stdout, "%s",
                            current_table_column == 0
                            ? table_symbols[current_symbol_set][3]
                            : table_inner_symbols[current_inner_symbol_set][1]
                           );
                    written_columns++;

                    substr_finish = rune_column;
                    if (unicode_buffer)
                        free(unicode_buffer);
                    unicode_buffer = u32_substr(unicode_line,
                                                unicode_line_len,
                                                substr_start,
                                                substr_finish,
                                                &unicode_buffer_len);
                    if (unicode_buffer_len < column_width)
                        column_width = unicode_buffer_len;

                    if (buffer)
                        free(buffer);
                    buffer = (uint8_t*) calloc(BUFSIZE, sizeof(uint8_t));
                    buffer_len = BUFSIZE;
                    u32_to_u8(unicode_buffer, column_width,
                              buffer, &buffer_len);

                    if (buffer && buffer_len>0)
                    {
                        fprintf(stdout, "%s", buffer);
                    }

                    if (handle_ansi)
                        sgr_chars = ansi_lookahead(unicode_buffer, column_width);
                    for (size_t i = 0; i + column_width <
                            max_table_column_width + sgr_chars; i++)
                    {
                        fprintf(stdout, " ");
                    }
                    if (handle_ansi)
                        fprintf(stdout, "%s", ANSI_SGR_RESET);

                    written_columns += max_table_column_width;
                    /*
                     *printf("wc = %lu, sc = %lu, bl = %lu\n",
                     *       written_columns,
                     *       sgr_chars,
                     *       buffer_len);
                     */

                    substr_start = rune_column+1;
                    current_table_column++;
                }
            }

            // Leftovers
            if (table_columns > 1)
            {
                column_width = max_table_column_width;
                // Not the last column
                if (current_table_column > table_columns-2)
                    column_width = max_last_table_column_width;

                fprintf(stdout, "%s",
                        current_table_column == 0
                        ? table_symbols[current_symbol_set][3]
                        : table_inner_symbols[current_inner_symbol_set][1]
                       );
                written_columns++;

                substr_finish = substr_start + column_width;
                if (unicode_buffer)
                    free(unicode_buffer);
                unicode_buffer = u32_substr(unicode_line,
                                            unicode_line_len,
                                            substr_start,
                                            substr_finish,
                                            &unicode_buffer_len);
                unicode_buffer_len = u32_strlen(unicode_buffer);
                if (unicode_buffer_len < column_width)
                    column_width = unicode_buffer_len;

                if (buffer)
                    free(buffer);
                buffer = (uint8_t*) calloc(BUFSIZE, sizeof(uint8_t));
                buffer_len = BUFSIZE;
                u32_to_u8(unicode_buffer, column_width,
                          buffer, &buffer_len);

                if (buffer && buffer_len>0)
                {
                    fprintf(stdout, "%s", buffer);
                    written_columns += column_width;
                }

                // Not the last column
                if (current_table_column+1 < table_columns)
                {
                    if (handle_ansi)
                        sgr_chars = ansi_lookahead(unicode_buffer, column_width);
                    for (size_t i = 0; i <
                            max_table_column_width-column_width+sgr_chars; i++)
                    {
                        fprintf(stdout, " ");
                    }
                    if (handle_ansi)
                        fprintf(stdout, "%s", ANSI_SGR_RESET);
                    written_columns += max_table_column_width - column_width;

                    for (size_t leftover_column = 0;
                            leftover_column < table_columns-current_table_column-1;
                            leftover_column++)
                    {
                        fprintf(stdout, "%s",
                                table_inner_symbols[current_inner_symbol_set][1]);

                        size_t table_column_width = max_table_column_width;
                        if (leftover_column == table_columns-current_table_column-2)
                            table_column_width = max_last_table_column_width;
                        for (size_t i = 0; i < table_column_width; i++)
                            fprintf(stdout, " ");
                    }
                }
                // Last one
                else
                {
                    for (size_t i = written_columns+1; i < rune_columns; i++)
                    {
                        fprintf(stdout, " ");
                    }
                    if (handle_ansi)
                        fprintf(stdout, "%s", ANSI_SGR_RESET);
                }

                written_columns = rune_columns;
            }
            // Handle a line with no delimiter
            else
            {
                column_width = max_table_column_width;

                fprintf(stdout, "%s", table_symbols[current_symbol_set][3]);

                substr_finish = unicode_line_len;
                if (unicode_buffer)
                    free(unicode_buffer);
                unicode_buffer = u32_substr(unicode_line,
                                            unicode_line_len,
                                            substr_start,
                                            substr_finish,
                                            &unicode_buffer_len);
                if (unicode_buffer_len < column_width + 1)
                    column_width = unicode_buffer_len-1;

                if (buffer)
                    free(buffer);
                buffer = (uint8_t*) calloc(BUFSIZE, sizeof(uint8_t));
                buffer_len = BUFSIZE;
                u32_to_u8(unicode_buffer, column_width,
                          buffer, &buffer_len);
                /*
                 *printf("\n");
                 *u32_print("unicode_buffer", unicode_buffer, column_width);
                 *printf("\n");
                 */

                if (buffer && buffer_len>0)
                {
                    fprintf(stdout, "%s", buffer);
                }

                if (handle_ansi)
                    sgr_chars = ansi_lookahead(unicode_buffer, column_width);
                for (size_t i = 0; i + column_width + table_columns + 1 <
                        rune_columns + sgr_chars; i++)
                {
                    fprintf(stdout, " ");
                }
                /*
                 *printf("cw = %lu, sc = %lu, bl = %lu\n",
                 *       column_width,
                 *       sgr_chars,
                 *       buffer_len);
                 */
            }
            if (handle_ansi)
                fprintf(stdout, "%s", ANSI_SGR_RESET);
            fprintf(stdout, "%s\n", table_symbols[current_symbol_set][5]);
        }
    }
    while (!feof(input));

    if (table_columns)
    {
        fprintf(stdout, "%s", table_symbols[current_symbol_set][6]);
        size_t table_column = 0;
        col = 0;
        while (col++ < rune_columns-2)
            if (table_column < table_columns-1
                    && col % (max_table_column_width+1) == 0)
            {
                fprintf(stdout, "%s",
                        table_inner_symbols[current_inner_symbol_set][2]);
                table_column++;
            }
            else
                fprintf(stdout, "%s", table_symbols[current_symbol_set][7]);
        fprintf(stdout, "%s\n", table_symbols[current_symbol_set][8]);
    }

    return 0;
}

