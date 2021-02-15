#!/bin/sh

SRCDIR=.

lsb_release -a 2>/dev/null | $SRCDIR/table -n -t
