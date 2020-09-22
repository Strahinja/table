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

#ifndef __DEFS_H
#define __DEFS_H

#define PROGRAMNAME "table"
#define VERSION "0.1"

#define BUFSIZE 4096

typedef enum
{
    CMD_NONE,
    CMD_VERSION
} Command;

enum
{
    TABLE_SYMBOLS_ASCII,
    TABLE_SYMBOLS_SINGLE,
    TABLE_SYMBOLS_DOUBLE
};

#endif

