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

#ifndef __DEFS_H
#define __DEFS_H

#define _POSIX_C_SOURCE 200809L

#include <errno.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistr.h>
#include <unistdio.h>
#include <uniwidth.h>

#define PROGRAMNAME "table"
#define VERSION     "0.2.1"

#define BUFSIZE       4096
#define SMALL_BUFSIZE 256

//#define ANSI_SGR_RESET "\e[0m\e[?25h"
#define ANSI_SGR_BOLD_ON  "\e[1m"
#define ANSI_SGR_BOLD_OFF "\e[0m"

typedef enum
{
    FALSE = 0,
    TRUE  = 1
} BOOL;

typedef unsigned char UBYTE;
typedef unsigned int  UINT;
typedef unsigned long ULONG;

typedef enum
{
    CMD_NONE,
    CMD_COLUMNS,
    CMD_DELIMITER,
    CMD_FORMAT,
    CMD_SYMBOLS,
    CMD_VERSION
} Command;

enum
{
    TABLE_SYMBOLS_ASCII,
    TABLE_SYMBOLS_SINGLE,
    TABLE_SYMBOLS_DOUBLE
};

enum
{
    TABLE_INNER_ASCII_ASCII,
    TABLE_INNER_SINGLE_SINGLE,
    TABLE_INNER_SINGLE_DOUBLE,
    TABLE_INNER_DOUBLE_SINGLE,
    TABLE_INNER_DOUBLE_DOUBLE
};

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
        (uint8_t*)"\u250c",  (uint8_t*)"\u2500",     (uint8_t*)"\u2510",
        (uint8_t*)"\u2502",  (uint8_t*)" ",          (uint8_t*)"\u2502",
        (uint8_t*)"\u2514",  (uint8_t*)"\u2500",     (uint8_t*)"\u2518",
    },
    [TABLE_SYMBOLS_DOUBLE] = {
        // double
        (uint8_t*)"\u2554",  (uint8_t*)"\u2550",  (uint8_t*)"\u2557",
        (uint8_t*)"\u2551",  (uint8_t*)" ",  (uint8_t*)"\u2551",
        (uint8_t*)"\u255a",  (uint8_t*)"\u2550",  (uint8_t*)"\u255d",
    }
};

static const uint8_t* table_inner_symbols[][3] =
{
    [TABLE_INNER_ASCII_ASCII] = {
        // ascii -> ascii
        (uint8_t*)"+",
        (uint8_t*)"|",
        (uint8_t*)"+",
    },

    [TABLE_INNER_SINGLE_SINGLE] = {
        // single -> single
        (uint8_t*)"\u252c",
        (uint8_t*)"\u2502",
        (uint8_t*)"\u2534",
    },

    [TABLE_INNER_SINGLE_DOUBLE] = {
        // single -> double
        (uint8_t*)"\u2565",
        (uint8_t*)"\u2551",
        (uint8_t*)"\u2568",
    },

    [TABLE_INNER_DOUBLE_SINGLE] = {
        // double -> single
        (uint8_t*)"\u2564",
        (uint8_t*)"\u2502",
        (uint8_t*)"\u2567",
    },

    [TABLE_INNER_DOUBLE_DOUBLE] = {
        // double -> double
        (uint8_t*)"\u2566",
        (uint8_t*)"\u2551",
        (uint8_t*)"\u2569",
    }
};

#endif

